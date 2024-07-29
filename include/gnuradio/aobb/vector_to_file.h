

#ifndef INCLUDED_AOBB_VECTOR_TO_FILE_H
#define INCLUDED_AOBB_VECTOR_TO_FILE_H

#include <gnuradio/aobb/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace aobb {

/*!
 * \brief Write stream to file
 * \ingroup file_operators_blk
 */
class AOBB_API vector_to_file : virtual public sync_block
{
public:
    typedef std::shared_ptr<vector_to_file> sptr;

    /*!
     * \brief Create a file to vector.
     *
     * Opens \p filename as a source of items into a flowgraph. The
     * data is expected to be in binary format, item after item. The
     * \p itemsize of the block determines the conversion from bits
     * to items. The first \p offset items (default 0) will be
     * skipped.
     *
     * If \p repeat is turned on, the file will repeat the file after
     * it's reached the end.
     *
     * If \p len is non-zero, only items (offset, offset+len) will
     * be produced.
     *
     * \param itemsize        the size of each item in the file, in bytes
     * \param filename        name of the file to source from
     * \param repeat  repeat file from start
     * \param offset  begin this many items into file
     * \param len     produce only items (offset, offset+len)
     */
    static sptr make(size_t itemsize,
                     const char* filename,
                     bool append = false,
                     float threshold = 0.5,
                     unsigned int samples_per_symbol = 100);


    /*!
     * \brief Opens a new file.
     *
     * \param filename        name of the file to source from
     * \param repeat  repeat file from start
     * \param offset  begin this many items into file
     * \param len     produce only items [offset, offset+len)
     */
    virtual bool open(const char* filename) = 0;

    /*!
     * \brief Close the file handle.
     */
    virtual void close() = 0;

    /*!
     * \brief if we've had an update, do it now.
     */
    virtual void do_update() = 0;

    /*!
     * \brief turn on unbuffered writes for slower outputs
     */
    virtual void set_unbuffered(bool unbuffered) = 0;
};

} // namespace aobb
} /* namespace gr */

#endif /* INCLUDED_AOBB_VECTOR_TO_FILE_H */
