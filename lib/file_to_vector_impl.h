

#ifndef INCLUDED_AOBB_FILE_TO_VECTOR_IMPL_H
#define INCLUDED_AOBB_FILE_TO_VECTOR_IMPL_H

#include <gnuradio/aobb/file_to_vector.h>
#include <gnuradio/thread/thread.h>

namespace gr {
namespace aobb {

class AOBB_API file_to_vector_impl : public file_to_vector
{
private:
    const size_t d_itemsize;
    uint64_t d_start_offset_items;
    uint64_t d_length_items;
    uint64_t d_items_remaining;
    FILE* d_fp;
    FILE* d_new_fp;
    bool d_repeat;
    bool d_updated;
    bool d_file_begin;
    bool d_seekable;
    long d_repeat_cnt;
    pmt::pmt_t d_add_begin_tag;

    gr::thread::mutex fp_mutex;
    unsigned int vector_offset;

    pmt::pmt_t _id;

    std::vector<gr_complex> vector_data;

    void do_update();

public:
    file_to_vector_impl(size_t itemsize,
                        const char* filename,
                        bool repeat,
                        uint64_t offset,
                        uint64_t len);
    ~file_to_vector_impl() override;

    bool seek(int64_t seek_point, int whence) override;
    void open(const char* filename, bool repeat, uint64_t offset, uint64_t len) override;
    void close() override;

    void rewind() override { vector_offset = 0; }
    void set_data(const std::vector<gr_complex>& data) override;
    void set_repeat(bool repeat) override { d_repeat = repeat; };

    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items) override;

    void set_begin_tag(pmt::pmt_t val) override;
};

} // namespace aobb
} /* namespace gr */

#endif /* INCLUDED_AOBB_FILE_SOURCE_IMPL_H */
