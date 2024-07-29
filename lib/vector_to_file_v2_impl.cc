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
#include "vector_to_file_v2_impl.h"
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

vector_to_file_v2::sptr
vector_to_file_v2::make(size_t itemsize, const char* filename, bool append)
{
    return gnuradio::make_block_sptr<vector_to_file_v2_impl>(itemsize, filename, append);
}

vector_to_file_v2_impl::vector_to_file_v2_impl(size_t itemsize,
                                               const char* filename,
                                               bool append)
    : sync_block("vector_to_file_v2",
                 io_signature::make(1, 1, itemsize),
                 io_signature::make(0, 0, 0)),
      d_itemsize(itemsize),
      d_fp(0),
      d_new_fp(0),
      d_updated(false),
      d_is_binary(true),
      d_append(append),
      d_filename(filename)

{
    gr::configure_default_loggers(
        d_base_logger, d_base_debug_logger, "vector_to_file_v2");
    if (!open(filename))
        throw std::runtime_error("can't open file");
}

vector_to_file_v2_impl::~vector_to_file_v2_impl()
{
    d_base_debug_logger->debug("[destructor] Closing file");
    close();
    if (d_fp) {
        fclose(d_fp);
        d_fp = 0;
    }
}


bool vector_to_file_v2_impl::open(const char* filename)
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

void vector_to_file_v2_impl::close()
{
    gr::thread::scoped_lock guard(d_mutex); // hold mutex for duration of this function

    d_base_debug_logger->debug("Closing file");
    if (d_new_fp) {
        fclose(d_new_fp);
        d_new_fp = 0;
    }
    d_updated = true;
}

void vector_to_file_v2_impl::do_update()
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

void vector_to_file_v2_impl::set_unbuffered(bool unbuffered)
{
    d_base_debug_logger->debug("Setting to {:s}buffered state", unbuffered ? "un" : "");
    d_unbuffered = unbuffered;
}


int vector_to_file_v2_impl::work(int noutput_items,
                                 gr_vector_const_void_star& input_items,
                                 gr_vector_void_star& output_items)
{

    const short sizeof_input_type = 4;
    const float* magnitude_vec = static_cast<const float*>(input_items[0]);
    int string_size = (noutput_items / 8);

    std::vector<char> input_to_string(string_size);
    char* current_char = input_to_string.data();
    char next_char = 0;

    int first_zero_index = 0;

    for (first_zero_index = 0; first_zero_index < noutput_items; first_zero_index++) {
        if (magnitude_vec[first_zero_index] == 0) {
            break;
        }
    }


    for (int i = first_zero_index, char_count = 0;
         i < noutput_items && char_count < string_size;) {
        // bitlerin ters sırada yazılması lazım
        for (int j = 7; j >= 0; j--) {

            unsigned short bit = magnitude_vec[i];

            if (j == 7 && bit == 1) {
                j++;

                // for (first_zero_index = i; first_zero_index < noutput_items;
                //      first_zero_index++) {
                //     if (magnitude_vec[first_zero_index] == 0) {
                //         i = first_zero_index;
                //         continue;
                //     }
                // }
                i += 1;
                next_char = 0;
                continue;
            }
            i += 1;
            next_char |= (bit << j);
        }
        // string_vec.emplace_back(next_char);
        // std::cout << " " << std::hex << std::setw(2) << std::setfill('0')
        //           << (0xff & next_char);
        *current_char++ = next_char;
        next_char = 0;
        char_count++;
    }

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
    return noutput_items;
}

bool vector_to_file_v2_impl::stop()
{
    do_update();
    fflush(d_fp);
    return true;
}

} // namespace aobb
} /* namespace gr */
