/*
*      Copyright (C) 2005-2011 Team XBMC
*      http://www.xbmc.org
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*  Parts of this code taken from Guido Vollbeding <http://sylvana.net/jpegcrop/exif_orientation.html>
*
*/

#include "include.h"
#include "lib/libexif/libexif.h"
#include "GUISettings.h"
#include "Settings.h"
#include "../xbmc/FileSystem/File.h"
#include "utils/log.h"
#include "JpegIO.h"
#include "XBTF.h"
#include "utils/log.h"

#include <setjmp.h>

#define EXIF_TAG_ORIENTATION    0x0112

struct my_error_mgr
{
  struct jpeg_error_mgr pub;    // "public" fields
  jmp_buf setjmp_buffer;        // for return to caller
};

CJpegIO::CJpegIO()
{
  m_width  = 0;
  m_height = 0;
  m_originalwidth = 0;
  m_originalheight = 0;
  m_orientation = 0;
  m_inputBuffSize = 0;
  m_inputBuff = NULL;
  m_texturePath = "";
}

CJpegIO::~CJpegIO()
{
  Close();
}

void CJpegIO::Close()
{
  delete [] m_inputBuff;
}

bool CJpegIO::Open(const CStdString &texturePath, unsigned int minx, unsigned int miny, bool read)
{
  m_texturePath = texturePath;
  unsigned int imgsize = 0;

  XFILE::CFile file;
  if (file.Open(m_texturePath.c_str(), 0))
  {
    imgsize = (unsigned int)file.GetLength();
    m_inputBuff = new unsigned char[imgsize];
    m_inputBuffSize = file.Read(m_inputBuff, imgsize);
    file.Close();

    if ((imgsize != m_inputBuffSize) || (m_inputBuffSize == 0))
      return false;
  }
  else
    return false;

  if (!read)
    return true;

  if (Read(m_inputBuff, m_inputBuffSize, minx, miny))
    return true;
  return false;
}

bool CJpegIO::Read(unsigned char* buffer, unsigned int bufSize, unsigned int minx, unsigned int miny)
{
  struct my_error_mgr jerr;
  m_cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = jpeg_error_exit;

  if (buffer == NULL || !bufSize )
    return false;

  jpeg_create_decompress(&m_cinfo);
  tb_jpeg_mem_src(&m_cinfo, buffer, bufSize);

  if (setjmp(jerr.setjmp_buffer))
  {
    tb_jpeg_destroy_decompress(&m_cinfo);
    return false;
  }
  else
  {
    tb_jpeg_save_markers(&m_cinfo, JPEG_APP0 + 1, 0xFFFF);
    tb_jpeg_read_header(&m_cinfo, true);
    m_originalwidth = m_cinfo.image_width;
    m_originalheight = m_cinfo.image_height;

    /*  libjpeg can scale the image for us if it is too big. It must be in the format
    num/denom, where (for our purposes) that is [1-8]/8 where 8/8 is the unscaled image.
    The only way to know how big a resulting image will be is to try a ratio and
    test its resulting size.
    If the res is greater than the one desired, use that one since there's no need
    to decode a bigger one just to squish it back down. If the res is greater than
    the gpu can hold, use the previous one.*/
    if (minx == 0 || miny == 0)
    {
      minx = g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iWidth;
      miny = g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iHeight;
    }

    /* override minx/miny values based on image aspect and area of requested minx/miny 
    so that tall/wide images come out larger (geometric mean) */
    unsigned int rminx = minx;
    unsigned int rminy = miny;
    unsigned int area = minx * miny;
    float aspect = ((float) m_originalwidth) / ((float) m_originalheight);
    minx = (unsigned int)sqrt(area * aspect);
    miny = (unsigned int)sqrt(area / aspect);
    CLog::Log(LOGDEBUG, "JpegIO::Read - Requested minx x miny %u x %u - using minx x miny %u x %u", rminx, rminy, minx, miny);

    m_cinfo.scale_denom = 8;
    m_cinfo.out_color_space = JCS_RGB;
    unsigned int maxtexsize = 4096;
    for (m_cinfo.scale_num = 1; m_cinfo.scale_num <= 8; m_cinfo.scale_num++)
    {
      tb_jpeg_calc_output_dimensions(&m_cinfo);
      if ((m_cinfo.output_width > maxtexsize) || (m_cinfo.output_height > maxtexsize))
      {
        m_cinfo.scale_num--;
        break;
      }
      if (m_cinfo.output_width >= minx || m_cinfo.output_height >= miny)
        break;
    }
    tb_jpeg_calc_output_dimensions(&m_cinfo);
    m_width  = m_cinfo.output_width;
    m_height = m_cinfo.output_height;
    CLog::Log(LOGDEBUG, "JpegIO::Read - Using scale_num of %i, %u x %u", m_cinfo.scale_num, m_width, m_height);

    if (m_cinfo.marker_list)
      m_orientation = GetExifOrientation(m_cinfo.marker_list->data, m_cinfo.marker_list->data_length);
    return true;
  }
}

bool CJpegIO::Decode(const unsigned char *pixels, unsigned int pitch, unsigned int format)
{
  unsigned char *dst = (unsigned char*)pixels;

  struct my_error_mgr jerr;
  m_cinfo.err = tb_jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = jpeg_error_exit;

  if (setjmp(jerr.setjmp_buffer))
  {
    tb_jpeg_destroy_decompress(&m_cinfo);
    return false;
  }
  else
  {
    tb_jpeg_start_decompress(&m_cinfo);

    if (format == XB_FMT_RGB8)
    {
      while (m_cinfo.output_scanline < m_height)
      {
        tb_jpeg_read_scanlines(&m_cinfo, &dst, 1);
        dst += pitch;
      }
    }
    else if (format == XB_FMT_A8R8G8B8)
    {
      unsigned char* row = new unsigned char[m_width * 3];
      while (m_cinfo.output_scanline < m_height)
      {
        tb_jpeg_read_scanlines(&m_cinfo, &row, 1);
        unsigned char *src2 = row;
        unsigned char *dst2 = dst;
        for (unsigned int x = 0; x < m_width; x++, src2 += 3)
        {
          *dst2++ = src2[2];
          *dst2++ = src2[1];
          *dst2++ = src2[0];
          *dst2++ = 0xff;
        }
        dst += pitch;
      }
      delete[] row;
    }
    else
    {
      CLog::Log(LOGWARNING, "JpegIO: Incorrect output format specified");
      tb_jpeg_destroy_decompress(&m_cinfo);
      return false;
    }
    tb_jpeg_finish_decompress(&m_cinfo);
  }
  tb_jpeg_destroy_decompress(&m_cinfo);
  return true;
}

bool CJpegIO::CreateThumbnail(const CStdString& sourceFile, const CStdString& destFile, int minx, int miny, bool rotateExif)
{
  //Copy sourceFile to buffer, pass to CreateThumbnailFromMemory for decode+re-encode
  if (!Open(sourceFile, minx, miny, false))
    return false;

  return CreateThumbnailFromMemory(m_inputBuff, m_inputBuffSize, destFile, minx, miny);
}

bool CJpegIO::CreateThumbnailFromMemory(unsigned char* buffer, unsigned int bufSize, const CStdString& destFile, unsigned int minx, unsigned int miny)
{
  //Decode a jpeg residing in buffer, pass to CreateThumbnailFromSurface for re-encode
  unsigned int pitch = 0;
  unsigned char *sourceBuf = NULL;

  if (!Read(buffer, bufSize, minx, miny))
    return false;
  pitch = Width() * 3;
  sourceBuf = new unsigned char [Height() * pitch];

  if (!Decode(sourceBuf,pitch,XB_FMT_RGB8))
  {
    delete [] sourceBuf;
    return false;
  }
  if (!CreateThumbnailFromSurface(sourceBuf, Width(), Height() , XB_FMT_RGB8, pitch, destFile))
  {
    delete [] sourceBuf;
    return false;
  }
  delete [] sourceBuf;
  return true;
}

bool CJpegIO::CreateThumbnailFromSurface(unsigned char* buffer, unsigned int width, unsigned int height, unsigned int format, unsigned int pitch, const CStdString& destFile)
{
  //Encode raw data from buffer, save to destFile
  struct jpeg_compress_struct cinfo;
  struct my_error_mgr jerr;
  JSAMPROW row_pointer[1];
  long unsigned int outBufSize = width * height;
  unsigned char* result;
  unsigned char* src = buffer;
  unsigned char* rgbbuf, *src2, *dst2;

  if (buffer == NULL)
  {
    CLog::Log(LOGERROR, "JpegIO::CreateThumbnailFromSurface no buffer");
    return false;
  }

  result = (unsigned char*) malloc(outBufSize); //Initial buffer. Grows as-needed.
  if (result == NULL)
  {
    CLog::Log(LOGERROR, "JpegIO::CreateThumbnailFromSurface error allocating memory for image buffer");
    return false;
  }

  if(format == XB_FMT_RGB8)
  {
    rgbbuf = buffer;
  }
  else if(format == XB_FMT_A8R8G8B8)
  {
    // create a copy for bgra -> rgb.
    rgbbuf = new unsigned char [(width * height * 3)];
    unsigned char* dst = rgbbuf;
    for (unsigned int y = 0; y < height; y++)
    {
      dst2 = dst;
      src2 = src;
      for (unsigned int x = 0; x < width; x++, src2 += 4)
      {
        *dst2++ = src2[2];
        *dst2++ = src2[1];
        *dst2++ = src2[0];
      }
      dst += width * 3;
      src += pitch;
    }
  }
  else
  {
    CLog::Log(LOGWARNING, "JpegIO::CreateThumbnailFromSurface Unsupported format");
    return false;
  }

  cinfo.err = tb_jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = jpeg_error_exit;
  jpeg_create_compress(&cinfo);

  if (setjmp(jerr.setjmp_buffer))
  {
    tb_jpeg_destroy_compress(&cinfo);
    delete [] result;
    if(format != XB_FMT_RGB8)
      delete [] rgbbuf;
    return false;
  }
  else
  {
    tb_jpeg_mem_dest(&cinfo, &result, &outBufSize);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    tb_jpeg_set_defaults(&cinfo);
    tb_jpeg_set_quality(&cinfo, 75, TRUE);
    tb_jpeg_start_compress(&cinfo, TRUE);

    while (cinfo.next_scanline < cinfo.image_height)
    {
      row_pointer[0] = &rgbbuf[cinfo.next_scanline * width * 3];
      tb_jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    tb_jpeg_finish_compress(&cinfo);
    tb_jpeg_destroy_compress(&cinfo);
  }
  if(format != XB_FMT_RGB8)
    delete [] rgbbuf;

  XFILE::CFile file;
  if (file.OpenForWrite(destFile, true))
  {
    file.Write(result, outBufSize);
    file.Close();
    free(result);
    return true;
  }
  free(result);
  return false;
}

// override libjpeg's error function to avoid an exit() call
void CJpegIO::jpeg_error_exit(j_common_ptr cinfo)
{
  CStdString msg;
  msg.Format("Error %i: %s",cinfo->err->msg_code, cinfo->err->jpeg_message_table[cinfo->err->msg_code]);
  CLog::Log(LOGWARNING, "JpegIO: %s", msg.c_str());

  my_error_mgr *myerr = (my_error_mgr*)cinfo->err;
  longjmp(myerr->setjmp_buffer, 1);
}

unsigned int CJpegIO::GetExifOrientation(unsigned char* exif_data, unsigned int exif_data_size)
{
  unsigned int offset = 0;
  unsigned int numberOfTags = 0;
  unsigned int tagNumber = 0;
  bool isMotorola = false;
  unsigned const char ExifHeader[] = "Exif\0\0";
  unsigned int orientation = 0;

  // read exif head, check for "Exif"
  //   next we want to read to current offset + length
  //   check if buffer is big enough
  if (exif_data_size && memcmp(exif_data, ExifHeader, 6) == 0)
  {
    //read exif body
    exif_data += 6;
  }
  else
  {
    return 0;
  }

  // Discover byte order
  if (exif_data[0] == 'I' && exif_data[1] == 'I')
    isMotorola = false;
  else if (exif_data[0] == 'M' && exif_data[1] == 'M')
    isMotorola = true;
  else
    return 0;

  // Check Tag Mark
  if (isMotorola)
  {
    if (exif_data[2] != 0 || exif_data[3] != 0x2A)
      return 0;
  }
  else
  {
    if (exif_data[3] != 0 || exif_data[2] != 0x2A)
      return 0;
  }

  // Get first IFD offset (offset to IFD0)
  if (isMotorola)
  {
    if (exif_data[4] != 0 || exif_data[5] != 0)
      return 0;
    offset = exif_data[6];
    offset <<= 8;
    offset += exif_data[7];
  }
  else
  {
    if (exif_data[7] != 0 || exif_data[6] != 0)
      return 0;
    offset = exif_data[5];
    offset <<= 8;
    offset += exif_data[4];
  }

  if (offset > exif_data_size - 2)
    return 0; // check end of data segment

  // Get the number of directory entries contained in this IFD
  if (isMotorola)
  {
    numberOfTags = exif_data[offset];
    numberOfTags <<= 8;
    numberOfTags += exif_data[offset+1];
  }
  else
  {
    numberOfTags = exif_data[offset+1];
    numberOfTags <<= 8;
    numberOfTags += exif_data[offset];
  }

  if (numberOfTags == 0)
    return 0;
  offset += 2;

  // Search for Orientation Tag in IFD0 - hey almost there! :D
  while(1)//hopefully this jpeg has correct exif data...
  {
    if (offset > exif_data_size - 12)
      return 0; // check end of data segment

    // Get Tag number
    if (isMotorola)
    {
      tagNumber = exif_data[offset];
      tagNumber <<= 8;
      tagNumber += exif_data[offset+1];
    }
    else
    {
      tagNumber = exif_data[offset+1];
      tagNumber <<= 8;
      tagNumber += exif_data[offset];
    }

    if (tagNumber == EXIF_TAG_ORIENTATION)
      break; //found orientation tag

    if ( --numberOfTags == 0)
      return 0;//no orientation found
    offset += 12;//jump to next tag
  }

  // Get the Orientation value
  if (isMotorola)
  {
    if (exif_data[offset+8] != 0)
      return 0;
    orientation = exif_data[offset+9];
  }
  else
  {
    if (exif_data[offset+9] != 0)
      return 0;
    orientation = exif_data[offset+8];
  }
  if (orientation > 8)
  {
    orientation = 0;
    return 0;
  }

  return orientation;//done
}