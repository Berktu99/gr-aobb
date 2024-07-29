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
#include "vector_to_file_impl.h"
#include <gnuradio/io_signature.h>
#include <gnuradio/logger.h>
#include <gnuradio/thread/thread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <volk/volk.h>
#include <algorithm>
#include <bitset>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>

// win32 (mingw/msvc) specific
#ifdef HAVE_IO_H
#include <io.h>
#endif
#include <iomanip>
#ifdef O_BINARY
#define OUR_O_BINARY O_BINARY
#else
#define OUR_O_BINARY 0
#endif

// should be handled via configure
#ifdef O_LARGEFILE
#define OUR_O_LARGEFILE O_LARGEFILE
#else
#define OUR_O_LARGEFILE 0
#endif


namespace gr {
namespace aobb {

vector_to_file::sptr vector_to_file::make(size_t itemsize,
                                          const char* filename,
                                          bool append,
                                          float threshold,
                                          unsigned int samples_per_symbol)
{
    return gnuradio::make_block_sptr<vector_to_file_impl>(
        itemsize, filename, append, threshold, samples_per_symbol);
}

vector_to_file_impl::vector_to_file_impl(size_t itemsize,
                                         const char* filename,
                                         bool append,
                                         float threshold,
                                         unsigned int samples_per_symbol)
    : sync_block("vector_to_file",
                 io_signature::make(1, 1, itemsize),
                 io_signature::make(0, 0, 0)),
      d_itemsize(itemsize),
      d_fp(0),
      d_new_fp(0),
      d_updated(false),
      d_is_binary(true),
      d_append(append),
      d_threshold(threshold),
      d_samples_per_symbol(samples_per_symbol),
      d_filename(filename)

{
    std::cout << "dthreshold:           " << d_threshold << std::endl;
    std::cout << "d_samples_per_symbol: " << d_samples_per_symbol << std::endl;
    gr::configure_default_loggers(d_base_logger, d_base_debug_logger, "vector_to_file");
    if (!open(filename))
        throw std::runtime_error("can't open file");
}

vector_to_file_impl::~vector_to_file_impl()
{
    d_base_debug_logger->debug("[destructor] Closing file");
    close();
    if (d_fp) {
        fclose(d_fp);
        d_fp = 0;
    }
}


bool vector_to_file_impl::open(const char* filename)
{
    // obtain exclusive access for duration of this function
    gr::thread::scoped_lock guard(d_mutex); // hold mutex for duration of this function
    d_base_debug_logger->debug("opening file {:s}", filename);

    // we use the open system call to get access to the O_LARGEFILE flag.
    int fd;
    int flags;

    if (d_append) {
        flags = O_WRONLY | O_CREAT | O_APPEND | OUR_O_LARGEFILE | OUR_O_BINARY;
    } else {
        flags = O_WRONLY | O_CREAT | O_TRUNC | OUR_O_LARGEFILE | OUR_O_BINARY;
    }
    if ((fd = ::open(filename, flags, 0664)) < 0) {
        d_base_logger->error("[::open] {:s}: {:s}", filename, strerror(errno));
        return false;
    }
    if (d_new_fp) { // if we've already got a new one open, close it
        fclose(d_new_fp);
        d_new_fp = 0;
    }

    if ((d_new_fp = fdopen(fd, d_is_binary ? "wb" : "w")) == NULL) {
        d_base_logger->error("[fdopen] {:s}: {:s}", filename, strerror(errno));
        ::close(fd); // don't leak file descriptor if fdopen fails.
    }

    d_updated = true;
    return d_new_fp != 0;
}

void vector_to_file_impl::close()
{
    gr::thread::scoped_lock guard(d_mutex); // hold mutex for duration of this function

    d_base_debug_logger->debug("Closing file");
    if (d_new_fp) {
        fclose(d_new_fp);
        d_new_fp = 0;
    }
    d_updated = true;
}

void vector_to_file_impl::do_update()
{
    if (d_updated) {
        gr::thread::scoped_lock guard(d_mutex); // hold mutex for duration of this block
        d_base_debug_logger->debug("updating");
        if (d_fp)
            fclose(d_fp);
        d_fp = d_new_fp; // install new file pointer
        d_new_fp = 0;
        d_updated = false;
    }
}

void vector_to_file_impl::set_unbuffered(bool unbuffered)
{
    d_base_debug_logger->debug("Setting to {:s}buffered state", unbuffered ? "un" : "");
    d_unbuffered = unbuffered;
}


int vector_to_file_impl::work(int noutput_items,
                              gr_vector_const_void_star& input_items,
                              gr_vector_void_star& output_items)
{


    // // COMPLEX TO MAGNITUDE
    const gr_complex* in = (const gr_complex*)input_items[0];
    std::vector<float> magnitude_vec(noutput_items);
    std::vector<short> binary_vec;

    // turned out to be faster than aligned/unaligned switching
    volk_32fc_magnitude_32f_u(magnitude_vec.data(), in, noutput_items);


    // MAGNITUDE THRESHOLD TO BINARY
    for (int i = 0; i < noutput_items; i++) {
        if (magnitude_vec[i] >= d_threshold) {
            magnitude_vec[i] = 1;
        } else {
            magnitude_vec[i] = 0;
        }
    }


    // // // BINARY TO STRING
    // // TODO: burada hata olablir, bellek kullanımını test etmek lazım
    int last_used_bit_index = 0;
    int string_size = (noutput_items / 8) / d_samples_per_symbol;

    std::vector<char> input_to_string(string_size);
    char* current_char = input_to_string.data();
    char next_char = 0;

    int first_zero_index = 0;

    for (first_zero_index = 0; first_zero_index < noutput_items; first_zero_index++) {
        if (magnitude_vec[first_zero_index] == 0) {
            break;
        }
    }

    int idx = first_zero_index;
    float avg = 0;
    unsigned short bit;
    for (int char_count = 0; idx < noutput_items && char_count < string_size;) {
        // TODO: maybe averagve the next "d_samples_per_synmbol" ?


        // bitlerin ters sırada yazılması lazım
        for (int j = 7; j >= 0; j--) {

            unsigned short bit = magnitude_vec[idx];

            if (j == 7 && bit == 1) {
                // sadece bir önlem, bir char 1 ile başlayamaz, sample rate eğer senkron
                // değilse diye konuldu.

                j++;

                // for (first_zero_index = i; first_zero_index < noutput_items;
                //      first_zero_index++) {
                //     if (magnitude_vec[first_zero_index] == 0) {
                //         i = first_zero_index;
                //         continue;
                //     }
                // }
                idx += 1;
                next_char = 0;
                continue;
            }
            idx += d_samples_per_symbol;

            // if (idx > noutput_items) {
            //     idx = noutput_items - (7 - j + 1) * d_samples_per_symbol;
            //     goto end;
            // }

            next_char |= (bit << j);
        }

        *current_char++ = next_char;
        next_char = 0;
        char_count++;
    }

    // end:
    last_used_bit_index = idx;
    // Now, input_to_string contains the result.
    const char* inbuf = input_to_string.data();
    int nwritten = 0;

    do_update(); // update d_fp is reqd

    if (!d_fp) {
        fprintf(stderr, "file_sink write failed with error %d\n", fileno(d_fp));
        return noutput_items / d_itemsize; // drop output on the floor
    }


    while (nwritten < string_size) {
        const int count = fwrite(inbuf, 1, string_size - nwritten, d_fp);
        if (count == 0) {
            if (ferror(d_fp)) {
                std::stringstream s;
                s << "file_sink write failed with error " << fileno(d_fp) << std::endl;
                throw std::runtime_error(s.str());
            } else { // is EOF
                break;
            }
        }
        nwritten += count;
        inbuf += count * 1;
    }

    if (d_unbuffered)
        fflush(d_fp);


    // return nwritten;
    // return string_size;
    // return noutput_items;
    return last_used_bit_index;
}

bool vector_to_file_impl::stop()
{
    do_update();
    fflush(d_fp);
    return true;
}

} // namespace aobb
} /* namespace gr */
