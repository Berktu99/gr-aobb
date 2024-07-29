

#ifndef DORT_ISLEM_IMPL_H
#define DORT_ISLEM_IMPL_H

#include <gnuradio/aobb/dort_islem.h>

namespace gr {
namespace aobb {

template <class T>
class AOBB_API dort_islem_impl : public dort_islem<T>
{
private:
    // std::string m_dort_islem_type;
    bool m_add;
    bool m_sub;
    bool m_mul;
    bool m_div;

public:
    dort_islem_impl(const bool add, const bool sub, const bool mul, const bool div);
    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items) override;
    void set_dort_islem_type(const bool add,
                             const bool sub,
                             const bool mul,
                             const bool div) override;
};

} // namespace aobb
} // namespace gr


#endif // DORT_ISLEM_IMPL_H