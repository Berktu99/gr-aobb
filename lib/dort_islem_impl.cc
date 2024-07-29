
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dort_islem_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace aobb {


template <class T>
typename dort_islem<T>::sptr
dort_islem<T>::make(const bool add, const bool sub, const bool mul, const bool div)
{
    return gnuradio::make_block_sptr<dort_islem_impl<T>>(add, sub, mul, div);
}

template <class T>
dort_islem_impl<T>::dort_islem_impl(const bool add,
                                    const bool sub,
                                    const bool mul,
                                    const bool div)
    : gr::sync_block("dort_islem",
                     io_signature::make(1, -1, sizeof(T)),
                     io_signature::make(1, add + sub + mul + div, sizeof(T))),
      m_add(add),
      m_sub(sub),
      m_mul(mul),
      m_div(div)
{
    fprintf(
        stdout, "add = %d, sub = %d, mul = %d, div = %d\n", m_add, m_sub, m_mul, m_div);
}

template <class T>
void dort_islem_impl<T>::set_dort_islem_type(const bool add,
                                             const bool sub,
                                             const bool mul,
                                             const bool div)
{
    m_add = add;
    m_sub = sub;
    m_mul = mul;
    m_div = div;
}


template <class T>
int dort_islem_impl<T>::work(int noutput_items,
                             gr_vector_const_void_star& input_items,
                             gr_vector_void_star& output_items)
{

    if (m_add) {
        T* result_add = static_cast<T*>(output_items[0]);
        memcpy(result_add, input_items[0], noutput_items * sizeof(T));

        for (size_t i = 1; i < input_items.size(); i++) {
            for (int elem = 0; elem < noutput_items; elem++) {
                result_add[elem] += (static_cast<const T*>(input_items[i]))[elem];
            }
        }
    }

    if (m_sub) {
        T* result_sub = static_cast<T*>(output_items[1]);
        memcpy(result_sub, input_items[0], noutput_items * sizeof(T));

        for (size_t i = 1; i < input_items.size(); i++) {
            for (int elem = 0; elem < noutput_items; elem++) {
                result_sub[elem] -= (static_cast<const T*>(input_items[i]))[elem];
            }
        }
    }

    if (m_mul) {
        T* result_mul = static_cast<T*>(output_items[2]);
        memcpy(result_mul, input_items[0], noutput_items * sizeof(T));

        for (size_t i = 1; i < input_items.size(); i++) {
            for (int elem = 0; elem < noutput_items; elem++) {
                result_mul[elem] *= (static_cast<const T*>(input_items[i]))[elem];
            }
        }
    }

    if (m_div) {
        T* result_mul = static_cast<T*>(output_items[3]);
        memcpy(result_mul, input_items[0], noutput_items * sizeof(T));

        for (size_t i = 1; i < input_items.size(); i++) {
            for (int elem = 0; elem < noutput_items; elem++) {
                result_mul[elem] /= (static_cast<const T*>(input_items[i]))[elem];
            }
        }
    }

    if (m_add == 0 && m_sub == 0 && m_mul == 0 && m_div == 0) {
        std::memset(static_cast<T*>(output_items[0]), 0, noutput_items * sizeof(T));
        std::memset(static_cast<T*>(output_items[1]), 0, noutput_items * sizeof(T));
        std::memset(static_cast<T*>(output_items[2]), 0, noutput_items * sizeof(T));
        std::memset(static_cast<T*>(output_items[3]), 0, noutput_items * sizeof(T));
    }


    return noutput_items;
}

template class dort_islem<std::int16_t>;
template class dort_islem<std::int32_t>;
template class dort_islem<gr_complex>;
template class dort_islem<float>;

} // namespace aobb
} // namespace gr
