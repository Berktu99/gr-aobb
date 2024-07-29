

#ifndef INCLUDED_AOBB_VECTOR_TO_FILE_V2_IMPL_H
#define INCLUDED_AOBB_VECTOR_TO_FILE_V2_IMPL_H

#include <gnuradio/aobb/vector_to_file_v2.h>
#include <gnuradio/thread/thread.h>

namespace gr {
namespace aobb {

class AOBB_API vector_to_file_v2_impl : public vector_to_file_v2
{
private:
    const size_t d_itemsize;
    FILE* d_fp;
    FILE* d_new_fp;
    bool d_updated;
    bool d_is_binary;
    bool d_append;
    float d_threshold;
    std::string d_filename;
    gr::thread::mutex d_mutex;
    bool d_unbuffered;
    gr::logger_ptr d_base_logger;
    gr::logger_ptr d_base_debug_logger;

    void do_update();

public:
    vector_to_file_v2_impl(size_t itemsize, const char* filename, bool append = false);
    ~vector_to_file_v2_impl() override;

    bool open(const char* filename) override;
    bool stop() override;
    void close() override;

    void set_unbuffered(bool unbuffered) override;


    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items) override;
};

} // namespace aobb
} /* namespace gr */

#endif /* INCLUDED_AOBB_VECTOR_TO_FILE_V2_IMPL_H */
