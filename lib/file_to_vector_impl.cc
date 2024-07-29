/* -*- c++ -*- */
/*
 * Copyright 2012, 2018 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "file_to_vector_impl.h"
#include <gnuradio/io_signature.h>
#include <gnuradio/thread/thread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>
#include <bitset>
#include <cstdio>
#include <sstream>
#include <stdexcept>

#ifdef _MSC_VER
#define GR_FSEEK _fseeki64
#define GR_FTELL _ftelli64
#define GR_FSTAT _fstati64
#define GR_FILENO _fileno
#define GR_STAT _stati64
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#else
#define GR_FSEEK fseeko
#define GR_FTELL ftello
#define GR_FSTAT fstat
#define GR_FILENO fileno
#define GR_STAT stat
#endif

namespace gr {
namespace aobb {

file_to_vector::sptr file_to_vector::make(size_t itemsize,
                                          const char* filename,
                                          bool repeat,
                                          uint64_t start_offset_items,
                                          uint64_t length_items)
{
    return gnuradio::make_block_sptr<file_to_vector_impl>(
        itemsize, filename, repeat, start_offset_items, length_items);
}

file_to_vector_impl::file_to_vector_impl(size_t itemsize,
                                         const char* filename,
                                         bool repeat,
                                         uint64_t start_offset_items,
                                         uint64_t length_items)
    : sync_block("file_to_vector",
                 io_signature::make(0, 0, 0),
                 io_signature::make(1, 1, itemsize)),
      d_itemsize(itemsize),
      d_start_offset_items(start_offset_items),
      d_length_items(length_items),
      d_fp(0),
      d_new_fp(0),
      d_repeat(repeat),
      d_updated(false),
      d_file_begin(true),
      d_repeat_cnt(0),
      d_add_begin_tag(pmt::PMT_NIL),
      vector_data()

{
    open(filename, repeat, start_offset_items, length_items);
    do_update();

    std::stringstream str;
    str << name() << unique_id();
    _id = pmt::string_to_symbol(str.str());
}

file_to_vector_impl::~file_to_vector_impl()
{
    if (d_fp)
        fclose((FILE*)d_fp);
    if (d_new_fp)
        fclose((FILE*)d_new_fp);
}

void file_to_vector_impl::set_data(const std::vector<gr_complex>& _vector_data)
{
    vector_data = _vector_data;
    rewind();
}

bool file_to_vector_impl::seek(int64_t seek_point, int whence)
{
    if (d_seekable) {
        seek_point += d_start_offset_items;

        switch (whence) {
        case SEEK_SET:
            break;
        case SEEK_CUR:
            seek_point += (d_length_items - d_items_remaining);
            break;
        case SEEK_END:
            seek_point = d_length_items - seek_point;
            break;
        default:
            d_logger->warn("bad seek mode {:d}", whence);
            return 0;
        }

        if ((seek_point < (int64_t)d_start_offset_items) ||
            (seek_point > (int64_t)(d_start_offset_items + d_length_items - 1))) {
            d_logger->warn("bad seek point {:d}", seek_point);
            return 0;
        }
        d_items_remaining = d_length_items - (seek_point - d_start_offset_items);
        return GR_FSEEK((FILE*)d_fp, seek_point * d_itemsize, SEEK_SET) == 0;
    } else {
        d_logger->warn("file not seekable");
        return 0;
    }
}


void file_to_vector_impl::open(const char* filename,
                               bool repeat,
                               uint64_t start_offset_items,
                               uint64_t length_items)
{
    // obtain exclusive access for duration of this function
    gr::thread::scoped_lock lock(fp_mutex);

    if (d_new_fp) {
        fclose(d_new_fp);
        d_new_fp = 0;
    }

    if ((d_new_fp = fopen(filename, "rb")) == NULL) {
        d_logger->error("[fopen] {:s}: {:s}", filename, strerror(errno));
        throw std::runtime_error("can't open file");
    }

    struct GR_STAT st;

    if (GR_FSTAT(GR_FILENO(d_new_fp), &st)) {
        d_logger->error("[fstat] {:s}: {:s}", filename, strerror(errno));
        throw std::runtime_error("can't fstat file");
    }
    if (S_ISREG(st.st_mode)) {
        d_seekable = true;
    } else {
        d_seekable = false;
    }

    uint64_t file_size;

    if (d_seekable) {
        // Check to ensure the file will be consumed according to item size
        if (GR_FSEEK(d_new_fp, 0, SEEK_END) == -1) {
            throw std::runtime_error("can't fseek()");
        }
        file_size = GR_FTELL(d_new_fp);

        // Make sure there will be at least one item available
        if ((file_size / d_itemsize) < (start_offset_items + 1)) {
            if (start_offset_items) {
                d_logger->warn("file is too small for start offset: {:d} < {:d}",
                               file_size - 1,
                               start_offset_items * d_itemsize);
            } else {
                d_logger->warn("file is too small ({:d})", file_size);
            }
            fclose(d_new_fp);
            throw std::runtime_error("file is too small");
        }
    } else {
        file_size = INT64_MAX;
    }

    uint64_t items_available = (file_size - start_offset_items);

    // If length is not specified, use the remainder of the file. Check alignment at end.
    if (length_items == 0) {
        length_items = items_available;
        if (d_seekable && (file_size % d_itemsize)) {
            d_logger->warn("file size is not a multiple of item size ({:d} ≠ N·{:d})",
                           file_size,
                           d_itemsize);
        }
    }

    // Check specified length. Warn and use available items instead of throwing an
    // exception.
    if (length_items > items_available) {
        d_logger->warn(
            "file too short, will read fewer than requested items: {:d} > {:d}",
            length_items,
            items_available);
        length_items = items_available;
    }

    // Rewind to start offset
    if (d_seekable) {
        auto start_offset = start_offset_items * d_itemsize;
#ifdef _POSIX_C_SOURCE
#if _POSIX_C_SOURCE >= 200112L
        // If supported, tell the OS that we'll be accessing the file sequentially
        // and that it would be a good idea to start prefetching it
        auto fd = fileno(d_new_fp);
        static const std::map<int, const std::string> fadv_errstrings = {
            { EBADF, "bad file descriptor" },
            { EINVAL, "invalid advise" },
            { ESPIPE, "tried to act as if a pipe or similar was a file" }
        };
        if (file_size && file_size != INT64_MAX) {
            if (auto ret = posix_fadvise(
                    fd, start_offset, file_size - start_offset, POSIX_FADV_SEQUENTIAL)) {
                d_logger->warn("failed to advise to read sequentially, {:s}",
                               fadv_errstrings.at(ret));
            }
            if (auto ret = posix_fadvise(
                    fd, start_offset, file_size - start_offset, POSIX_FADV_WILLNEED)) {
                d_logger->warn("failed to advise we'll need file contents soon, {:s}",
                               fadv_errstrings.at(ret));
            }
        }
#endif
#endif
        if (GR_FSEEK(d_new_fp, start_offset, SEEK_SET) == -1) {
            throw std::runtime_error("can't fseek()");
        }
    }

    d_updated = true;
    d_repeat = repeat;
    d_start_offset_items = start_offset_items;
    d_length_items = length_items;
    d_items_remaining = length_items;
}

void file_to_vector_impl::close()
{
    // obtain exclusive access for duration of this function
    gr::thread::scoped_lock lock(fp_mutex);

    if (d_new_fp != NULL) {
        fclose(d_new_fp);
        d_new_fp = NULL;
    }
    d_updated = true;
}

void file_to_vector_impl::do_update()
{
    if (d_updated) {
        gr::thread::scoped_lock lock(fp_mutex); // hold while in scope

        if (d_fp)
            fclose(d_fp);

        d_fp = d_new_fp; // install new file pointer
        d_new_fp = 0;
        d_updated = false;
        d_file_begin = true;
    }
}

void file_to_vector_impl::set_begin_tag(pmt::pmt_t val) { d_add_begin_tag = val; }


int file_to_vector_impl::work(int noutput_items,
                              gr_vector_const_void_star& input_items,
                              gr_vector_void_star& output_items)
{
    // static gr::logger log("file_to_vector_impl::work");

    char read_file_item_size = 1;

    gr_complex* optr = (gr_complex*)output_items[0];

    // TODO: Bu tam bölüneyebilir.
    uint64_t size = noutput_items / sizeof(gr_complex);
    char* read_file = new char[size];
    char* ptr_beginning = read_file; // Separate pointer for iteration

    do_update(); // update d_fp is reqd
    if (d_fp == NULL)
        throw std::runtime_error("work with file not open");

    gr::thread::scoped_lock lock(fp_mutex); // hold for the rest of this function

    // No items remaining - all done
    if (d_items_remaining == 0) {
        return WORK_DONE;
    }

    while (size) {

        // Add stream tag whenever the file starts again
        if (d_file_begin && d_add_begin_tag != pmt::PMT_NIL) {
            add_item_tag(0,
                         nitems_written(0) + noutput_items - size,
                         d_add_begin_tag,
                         pmt::from_long(d_repeat_cnt),
                         _id);
            d_file_begin = false;
        }

        uint64_t nitems_to_read = std::min(size, d_items_remaining);

        size_t nitems_read =
            fread(read_file, read_file_item_size, nitems_to_read, (FILE*)d_fp);
        if (nitems_to_read != nitems_read) {
            // Size of non-seekable files is unknown. EOF is normal.
            if (!d_seekable && feof((FILE*)d_fp)) {
                size -= nitems_read;
                d_items_remaining = 0;
                break;
            }

            throw std::runtime_error("fread error");
        }

        size -= nitems_read;
        d_items_remaining -= nitems_read;
        read_file += nitems_read * read_file_item_size;

        // Ran out of items ("EOF")
        if (d_items_remaining == 0) {

            // Repeat: rewind and request tag
            if (d_repeat && d_seekable) {
                if (GR_FSEEK(d_fp,
                             d_start_offset_items * read_file_item_size,
                             SEEK_SET) == -1) {
                    throw std::runtime_error("can't fseek()");
                }
                d_items_remaining = d_length_items;
                if (d_add_begin_tag != pmt::PMT_NIL) {
                    d_file_begin = true;
                    d_repeat_cnt++;
                }
            }

            // No repeat: return
            else {
                break;
            }
        }
    }

    // log.warn("out of while loop: read file: ");
    // fprintf(stderr, "read_file = %s\n", ptr_beginning);
    // convert to vector and feed it to output

    uint64_t index = 0;
    std::vector<gr_complex> bits_vector(0);
    while (index < (noutput_items / sizeof(gr_complex))) {
        // fprintf(stderr, "c = %c   ", ptr_beginning[index]);
        std::bitset<8> bits(ptr_beginning[index]);

        for (int i = 7; i >= 0; --i) {
            bits_vector.push_back(gr_complex(bits[i], 0));
        }
        index++;
    }

    // log.warn("bits_vector: ");
    // fprintf(stderr, "bits_vector[0].real() = %d\n", bits_vector[0].real());

    // Buraya kadar geldiğinde elimizde tam olarak gnuradio'nun beklediği
    // boyutta bir bits_vector olmalı


    // memcpy((void*)optr, (const void*)&bits_vector[0], noutput_items *
    // sizeof(gr_complex));

    // fprintf(stdout, "output_items.size() %d\n= ", (int)output_items.size());
    // fprintf(stdout, "Noutput_items       %d\n= ", (int)noutput_items);
    // fprintf(stdout, "bits_vector.size()  %d\n= ", (int)bits_vector.size());

    for (long unsigned int i = 0; i < bits_vector.size(); i++) {
        optr[i] = bits_vector[i];
        // fprintf(stdout,
        //         "bits_vector[%d] = %f + %fi\n",
        //         i,
        //         bits_vector[i].real(),
        //         bits_vector[i].imag());
    }


    // index = 0;
    // for (int i = 0; i < static_cast<int>(noutput_items); i++) {
    //     // fprintf(stderr, "bits_vector[%d] = %d\n", index, bits_vector[index]);
    //     // TODO: use memcpy instead
    //     optr[i] = bits_vector[index++];
    //     fprintf(stderr, "optr[%d] = %d\n", index, optr[index]);
    //     if (index >= bits_vector.size()) {
    //         // Loop back the read file if needed
    //         index = 0;
    //     }
    // }


    // log.warn("output is done: ");
    // fprintf(stderr, bits_vector);

    delete[] ptr_beginning;
    // bits_vector.clear();
    // fprintf(stdout, "if it comes here than there is no error?\n");

    // log.warn("read_file deleted: ");
    // fprintf(stderr, bits_vector);

    // Iterate through each character in the string


    // return (noutput_items - size);
    return (bits_vector.size());
}

} // namespace aobb
} /* namespace gr */
