/*
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef XBMC_TAGLIB_CALLBACK_STREAM_H
#define XBMC_TAGLIB_CALLBACK_STREAM_H

#include "taglib_xbmc.h"
#include <tiostream.h>
#include <tbytevector.h>
#include <algorithm>
#include <stdexcept>

namespace XbmcTagLib
{
// This adapter is constructed and destroyed inside libtag. Only byte buffers
// and C function pointers cross into XBMC's VFS.
class CallbackStream : public TagLib::IOStream
{
public:
  explicit CallbackStream(const XbmcTagLibRequest& request) : m_request(request)
  {
    if (length() <= 0)
      throw std::runtime_error("empty or unavailable audio stream");
    seek(0);
  }
  virtual TagLib::FileName name() const { return m_request.filename; }
  virtual TagLib::ByteVector readBlock(unsigned long requested)
  {
    // Never allocate according to an unchecked length from a corrupt tag.
    const long remaining = length() - tell();
    const unsigned int size = static_cast<unsigned int>(
        std::min(requested, static_cast<unsigned long>(std::max(0L, remaining))));
    TagLib::ByteVector result(size);
    unsigned int done = 0;
    while (done < size)
    {
      int count = m_request.io.read(m_request.io.context, result.data() + done, size - done);
      if (count < 0 || static_cast<unsigned int>(count) > size - done)
        throw std::runtime_error("audio stream read failed");
      if (count == 0) break;
      done += count;
    }
    result.resize(done);
    return result;
  }
  virtual bool readOnly() const { return true; }
  virtual bool isOpen() const { return true; }
  virtual void seek(long offset, Position position = Beginning)
  {
    Checked(m_request.io.seek(m_request.io.context, offset,
        position == Beginning ? 0 : position == Current ? 1 : 2));
  }
  virtual long tell() const
  { return Checked(m_request.io.seek(m_request.io.context, 0, 1)); }
  virtual long length()
  { return Checked(m_request.io.length(m_request.io.context)); }
  virtual void clear() {}
  virtual void writeBlock(const TagLib::ByteVector&) { ReadOnly(); }
  virtual void insert(const TagLib::ByteVector&, unsigned long, unsigned long) { ReadOnly(); }
  virtual void removeBlock(unsigned long, unsigned long) { ReadOnly(); }
  virtual void truncate(long) { ReadOnly(); }
private:
  static long Checked(XbmcTagLibOffset value)
  {
    // TagLib 1.x uses signed 32-bit long on Windows, even on a 64-bit host.
    if (value < 0 || value > 0x7fffffff)
      throw std::runtime_error("audio stream I/O failed or exceeds TagLib 1.x 2 GiB limit");
    return static_cast<long>(value);
  }
  static void ReadOnly() { throw std::runtime_error("audio stream is read-only"); }
  const XbmcTagLibRequest& m_request;
};
}
#endif
