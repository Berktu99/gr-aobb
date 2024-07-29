#ifndef DORT_ISLEM_H
#define DORT_ISLEM_H

#include <gnuradio/aobb/api.h>
#include <gnuradio/sync_block.h>
#include <cstdint>

namespace gr {
namespace aobb {

// TODO: Bu enum gnuradio'da nasıl expose edilebilir?
enum class dort_islem_type { add, sub, mul, div };


template <typename T>
class AOBB_API dort_islem : virtual public gr::sync_block
{
private:
    /* data */
public:
    typedef std::shared_ptr<dort_islem<T>> sptr;
    // Vector desteklenmiyor, zaten daha vector ile ne yaptığını anlamadım
    static sptr
    make(const bool add = 1, const bool sub = 0, const bool mul = 0, const bool div = 0);

    virtual void set_dort_islem_type(const bool add,
                                     const bool sub,
                                     const bool mul,
                                     const bool div) = 0;
};

typedef dort_islem<std::int16_t> dort_islem_ss;
typedef dort_islem<std::int32_t> dort_islem_ii;
typedef dort_islem<gr_complex> dort_islem_cc;
typedef dort_islem<float> dort_islem_ff;

} // namespace aobb
} // namespace gr

#endif
