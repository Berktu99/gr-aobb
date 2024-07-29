

#ifndef INCLUDED_AOBB_FILE_TO_VECTOR_H
#define INCLUDED_AOBB_FILE_TO_VECTOR_H

#include <gnuradio/aobb/api.h>
#include <gnuradio/sync_block.h>
#include <cstdint>

namespace gr {
namespace aobb {

/*!
 * \brief Read stream from file
 * \ingroup file_operators_blk
 */
class AOBB_API file_to_vector : virtual public sync_block
{
public:
    typedef std::shared_ptr<file_to_vector> sptr;

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
                     bool repeat = false,
                     uint64_t offset = 0,
                     uint64_t len = 0);

    /*!
     * \brief seek file to \p seek_point relative to \p whence
     *
     * \param seek_point      sample offset in file
     * \param whence  one of SEEK_SET, SEEK_CUR, SEEK_END (man fseek)
     */
    virtual bool seek(int64_t seek_point, int whence) = 0;

    /*!
     * \brief Opens a new file.
     *
     * \param filename        name of the file to source from
     * \param repeat  repeat file from start
     * \param offset  begin this many items into file
     * \param len     produce only items [offset, offset+len)
     */
    virtual void
    open(const char* filename, bool repeat, uint64_t offset = 0, uint64_t len = 0) = 0;

    /*!
     * \brief Close the file handle.
     */
    virtual void close() = 0;

    /*!
     * \brief Add a stream tag to the first sample of the file if true
     */
    virtual void set_begin_tag(pmt::pmt_t val) = 0;

    virtual void rewind() = 0;
    // TODO: add tag to vector?
    virtual void set_data(const std::vector<gr_complex>& data) = 0;
    virtual void set_repeat(bool repeat) = 0;
};

} // namespace aobb
} /* namespace gr */

#endif /* INCLUDED_BLOCKS_FILE_SOURCE_H */
