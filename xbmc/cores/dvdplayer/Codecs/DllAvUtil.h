#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "DynamicDll.h"

extern "C" {
#ifndef HAVE_MMX
#define HAVE_MMX
#endif
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#ifndef __GNUC__
#pragma warning(disable:4244)
#endif

#include "libavutil/avutil.h"
#include "libavutil/channel_layout.h"
#include "libavutil/crc.h"
#include "libavutil/opt.h"
#include "libavutil/fifo.h"
#include "libavutil/samplefmt.h"

}

#include "settings/Settings.h"

// callback used for logging
void ff_avutil_log(void* ptr, int level, const char* format, va_list va);

class DllAvUtilInterface
{
public:
  virtual ~DllAvUtilInterface() {}
  virtual void av_log_set_callback(void (*)(void*, int, const char*, va_list))=0;
  virtual void *av_malloc(unsigned int size)=0;
  virtual void *av_mallocz(unsigned int size)=0;
  virtual void *av_realloc(void *ptr, unsigned int size)=0;
  virtual void av_free(void *ptr)=0;
  virtual void av_freep(void *ptr)=0;
  virtual int64_t av_rescale_rnd(int64_t a, int64_t b, int64_t c, enum AVRounding)=0;
  virtual const AVCRC* av_crc_get_table(AVCRCId crc_id)=0;
  virtual uint32_t av_crc(const AVCRC *ctx, uint32_t crc, const uint8_t *buffer, size_t length)=0;
  virtual int av_opt_set(void *obj, const char *name, const char *val, int search_flags)=0;
  virtual char *av_strdup(const char *s)=0;
  virtual int av_get_bytes_per_sample(enum AVSampleFormat p1) = 0;
  virtual AVDictionaryEntry *av_dict_get(AVDictionary *m, const char *key, const AVDictionaryEntry *prev, int flags) = 0;
  virtual int av_dict_set(AVDictionary **pm, const char *key, const char *value, int flags)=0;
  virtual int av_samples_get_buffer_size (int *linesize, int nb_channels, int nb_samples, enum AVSampleFormat sample_fmt, int align) = 0;
  virtual int64_t av_get_default_channel_layout(int nb_channels)=0;
  virtual void av_force_cpu_flags(int mask)=0;
};

#if (defined USE_EXTERNAL_FFMPEG)

// Use direct layer
class DllAvUtilBase : public DllDynamic, DllAvUtilInterface
{
public:
  
  virtual ~DllAvUtilBase() {}
   virtual void av_log_set_callback(void (*foo)(void*, int, const char*, va_list)) { ::av_log_set_callback(foo); }
   virtual void *av_malloc(unsigned int size) { return ::av_malloc(size); }
   virtual void *av_mallocz(unsigned int size) { return ::av_mallocz(size); }
   virtual void *av_realloc(void *ptr, unsigned int size) { return ::av_realloc(ptr, size); } 
   virtual void av_free(void *ptr) { ::av_free(ptr); }
   virtual void av_freep(void *ptr) { ::av_freep(ptr); }
   virtual int64_t av_rescale_rnd(int64_t a, int64_t b, int64_t c, enum AVRounding d) { return ::av_rescale_rnd(a, b, c, d); }
   virtual const AVCRC* av_crc_get_table(AVCRCId crc_id) { return ::av_crc_get_table(crc_id); }
   virtual uint32_t av_crc(const AVCRC *ctx, uint32_t crc, const uint8_t *buffer, size_t length) { return ::av_crc(ctx, crc, buffer); }
   virtual int av_opt_set(void *obj, const char *name, const char *val, int search_flags) { return ::av_opt_set(obj, name, val, search_flags); }
  virtual int av_get_bytes_per_sample(enum AVSampleFormat p1)
    { return ::av_get_bytes_per_sample(p1); }
  virtual AVDictionaryEntry *av_dict_get(AVDictionary *m, const char *key, const AVDictionaryEntry *prev, int flags){ return ::av_dict_get(m, key, prev, flags); }
  virtual int av_dict_set(AVDictionary **pm, const char *key, const char *value, int flags) { return ::av_dict_set(pm, key, value, flags); }
  virtual int av_samples_get_buffer_size (int *linesize, int nb_channels, int nb_samples, enum AVSampleFormat sample_fmt, int align)
    { return ::av_samples_get_buffer_size(linesize, nb_channels, nb_samples, sample_fmt, align); }
  virtual int64_t av_get_default_channel_layout(int nb_channels) { return ::av_get_default_channel_layout(nb_channels); }

   // DLL faking.
   virtual bool ResolveExports() { return true; }
   virtual bool Load() {
     CLog::Log(LOGDEBUG, "DllAvUtilBase: Using libavutil system library");
     return true;
   }
   virtual void Unload() {}
};

#else

class DllAvUtilBase : public DllDynamic, DllAvUtilInterface
{
public:
  DllAvUtilBase() : DllDynamic( CSettings::Get().GetFFmpegDllFolder() + "avutil-52.dll") {}

  LOAD_SYMBOLS()

  DEFINE_METHOD1(void, av_log_set_callback, (void (*p1)(void*, int, const char*, va_list)))
  DEFINE_METHOD1(void*, av_malloc, (unsigned int p1))
  DEFINE_METHOD1(void*, av_mallocz, (unsigned int p1))
  DEFINE_METHOD2(void*, av_realloc, (void *p1, unsigned int p2))
  DEFINE_METHOD1(void, av_free, (void *p1))
  DEFINE_METHOD1(void, av_freep, (void *p1))
  DEFINE_METHOD4(int64_t, av_rescale_rnd, (int64_t p1, int64_t p2, int64_t p3, enum AVRounding p4));
  DEFINE_METHOD1(const AVCRC*, av_crc_get_table, (AVCRCId p1))
  DEFINE_METHOD4(uint32_t, av_crc, (const AVCRC *p1, uint32_t p2, const uint8_t *p3, size_t p4));
  DEFINE_METHOD4(int, av_opt_set, (void *p1, const char *p2, const char *p3, int p4));
  DEFINE_METHOD1(char*, av_strdup, (const char *p1))
  DEFINE_METHOD1(int, av_get_bytes_per_sample, (enum AVSampleFormat p1))
  DEFINE_METHOD4(AVDictionaryEntry *, av_dict_get, (AVDictionary *p1, const char *p2, const AVDictionaryEntry *p3, int p4))
  DEFINE_METHOD4(int, av_dict_set, (AVDictionary **p1, const char *p2, const char *p3, int p4));
  DEFINE_METHOD5(int, av_samples_get_buffer_size, (int *p1, int p2, int p3, enum AVSampleFormat p4, int p5))
  DEFINE_METHOD1(int64_t, av_get_default_channel_layout, (int p1))
  DEFINE_METHOD1(void, av_force_cpu_flags, (int p1)) 

  public:
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(av_log_set_callback)
    RESOLVE_METHOD(av_malloc)
    RESOLVE_METHOD(av_mallocz)
    RESOLVE_METHOD(av_realloc)
    RESOLVE_METHOD(av_free)
    RESOLVE_METHOD(av_freep)
    RESOLVE_METHOD(av_rescale_rnd)
    RESOLVE_METHOD(av_crc_get_table)
    RESOLVE_METHOD(av_crc)
    RESOLVE_METHOD(av_opt_set)
    RESOLVE_METHOD(av_strdup)
    RESOLVE_METHOD(av_get_bytes_per_sample)
    RESOLVE_METHOD(av_dict_get)
    RESOLVE_METHOD(av_dict_set)
    RESOLVE_METHOD(av_samples_get_buffer_size)
    RESOLVE_METHOD(av_get_default_channel_layout)
    RESOLVE_METHOD(av_force_cpu_flags)
  END_METHOD_RESOLVE()
};

#endif

class DllAvUtil : public DllAvUtilBase
{
public:
  virtual bool Load()
  {
    if( DllAvUtilBase::Load() )
    {
      DllAvUtilBase::av_log_set_callback(ff_avutil_log);
      return true;
    }
    return false;
  }
};
