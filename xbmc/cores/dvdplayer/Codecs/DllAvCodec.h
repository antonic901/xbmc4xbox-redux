
#pragma once
#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "DynamicDll.h"
#include "DllAvUtil.h"

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

#include "libavcodec/avcodec.h"
#include "libavcodec/audioconvert.h"
}

#include "threads/SingleLock.h"
#include "settings/Settings.h"

class DllAvCodecInterface
{
public:
  virtual ~DllAvCodecInterface() {}
  virtual void avcodec_register_all(void)=0;
  virtual void avcodec_flush_buffers(AVCodecContext *avctx)=0;
  virtual int avcodec_open2_dont_call(AVCodecContext *avctx, AVCodec *codec, AVDictionary **options)=0;
  virtual AVCodec *avcodec_find_decoder(enum CodecID id)=0;
  virtual int avcodec_close_dont_call(AVCodecContext *avctx)=0;
  virtual AVFrame *avcodec_alloc_frame(void)=0;
  virtual int avpicture_fill(AVPicture *picture, uint8_t *ptr, int pix_fmt, int width, int height)=0;
  virtual int avcodec_decode_video2(AVCodecContext *avctx, AVFrame *picture, int *got_picture_ptr, AVPacket *avpkt)=0;
  virtual int avcodec_decode_audio4(AVCodecContext *avctx, AVFrame *frame, int *got_frame_ptr, AVPacket *avpkt)=0;
  virtual int avcodec_decode_subtitle2(AVCodecContext *avctx, AVSubtitle *sub, int *got_sub_ptr, AVPacket *avpkt)=0;
  virtual int avpicture_get_size(PixelFormat pix_fmt, int width, int height)=0;
  virtual AVCodecContext *avcodec_alloc_context3(AVCodec *codec)=0;
  virtual void avcodec_string(char *buf, int buf_size, AVCodecContext *enc, int encode)=0;
  virtual void avcodec_get_context_defaults3(AVCodecContext *s, AVCodec *codec)=0;
  virtual AVCodecParserContext *av_parser_init(int codec_id)=0;
  virtual int av_parser_parse2(AVCodecParserContext *s,AVCodecContext *avctx, uint8_t **poutbuf, int *poutbuf_size,
                    const uint8_t *buf, int buf_size,
                    int64_t pts, int64_t dts, int64_t pos)=0;
  virtual void av_parser_close(AVCodecParserContext *s)=0;
  virtual void avpicture_free(AVPicture *picture)=0;
  virtual int avpicture_alloc(AVPicture *picture, PixelFormat pix_fmt, int width, int height)=0;
  virtual int avcodec_default_get_buffer(AVCodecContext *s, AVFrame *pic)=0;
  virtual void avcodec_default_release_buffer(AVCodecContext *s, AVFrame *pic)=0;
  virtual AVCodec *av_codec_next(AVCodec *c)=0;
  virtual AVAudioConvert *av_audio_convert_alloc(enum AVSampleFormat out_fmt, int out_channels,
                                                 enum AVSampleFormat in_fmt , int in_channels,
                                                 const float *matrix      , int flags)=0;
  virtual void av_audio_convert_free(AVAudioConvert *ctx)=0;
  virtual int av_audio_convert(AVAudioConvert *ctx,
                                     void * const out[6], const int out_stride[6],
                               const void * const  in[6], const int  in_stride[6], int len)=0;
  virtual int av_dup_packet(AVPacket *pkt)=0;
  virtual int av_init_packet(AVPacket *pkt)=0;
  virtual void av_free_packet(AVPacket *pkt)=0;
};

#if (defined USE_EXTERNAL_FFMPEG)

// Use direct layer
class DllAvCodec : public DllDynamic, DllAvCodecInterface
{
public:
  static CCriticalSection m_critSection;
  
  virtual ~DllAvCodec() {}
  virtual void avcodec_register_all() { ::avcodec_register_all(); }
  virtual void avcodec_flush_buffers(AVCodecContext *avctx) { ::avcodec_flush_buffers(avctx); }
  virtual int avcodec_open2(AVCodecContext *avctx, AVCodec *codec, AVDictionary **options) 
  { 
    CSingleLock lock(DllAvCodec::m_critSection);
    return ::avcodec_open2(avctx, codec, options);
  }
  virtual int avcodec_open2_dont_call(AVCodecContext *avctx, AVCodec *codec, AVDictionary **options) { *(int *)0x0 = 0; return 0; } 
  virtual int avcodec_close_dont_call(AVCodecContext *avctx) { *(int *)0x0 = 0; return 0; } 
  virtual AVCodec *avcodec_find_decoder(enum CodecID id) { return ::avcodec_find_decoder(id); }
  virtual int avcodec_close(AVCodecContext *avctx) 
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return ::avcodec_close(avctx); 
  }
  virtual AVFrame *avcodec_alloc_frame() { return ::avcodec_alloc_frame(); }
  virtual int avpicture_fill(AVPicture *picture, uint8_t *ptr, int pix_fmt, int width, int height) { return ::avpicture_fill(picture, ptr, pix_fmt, width, height); }
  virtual int avcodec_decode_video2(AVCodecContext *avctx, AVFrame *picture, int *got_picture_ptr, AVPacket *avpkt) { return ::avcodec_decode_video2(avctx, picture, got_picture_ptr, avpkt); }
  virtual int avcodec_decode_audio4(AVCodecContext *avctx, AVFrame *frame, int *got_frame_ptr, AVPacket *avpkt) { return ::avcodec_decode_audio4(avctx, frame, got_frame_ptr, avpkt); }
  virtual int avcodec_decode_subtitle2(AVCodecContext *avctx, AVSubtitle *sub, int *got_sub_ptr, AVPacket *avpkt) { return ::avcodec_decode_subtitle2(avctx, sub, got_sub_ptr, avpkt); }
  virtual int avpicture_get_size(PixelFormat pix_fmt, int width, int height) { return ::avpicture_get_size(pix_fmt, width, height); }
  virtual AVCodecContext *avcodec_alloc_context3(AVCodec *codec) { return ::avcodec_alloc_context3(codec); }
  virtual void avcodec_string(char *buf, int buf_size, AVCodecContext *enc, int encode) { ::avcodec_string(buf, buf_size, enc, encode); }
  virtual void avcodec_get_context_defaults3(AVCodecContext *s, AVCodec *codec) { ::avcodec_get_context_defaults3(s, codec); }
  
  virtual AVCodecParserContext *av_parser_init(int codec_id) { return ::av_parser_init(codec_id); }
  virtual int av_parser_parse2(AVCodecParserContext *s,AVCodecContext *avctx, uint8_t **poutbuf, int *poutbuf_size,
                    const uint8_t *buf, int buf_size,
                    int64_t pts, int64_t dts, int64_t pos)
  {
    return ::av_parser_parse2(s, avctx, poutbuf, poutbuf_size, buf, buf_size, pts, dts, pos);
  }
  virtual void av_parser_close(AVCodecParserContext *s) { ::av_parser_close(s); }
  
  virtual void avpicture_free(AVPicture *picture) { ::avpicture_free(picture); }
  virtual int avpicture_alloc(AVPicture *picture, PixelFormat pix_fmt, int width, int height) { return ::avpicture_alloc(picture, pix_fmt, width, height); }
  virtual int avcodec_default_get_buffer(AVCodecContext *s, AVFrame *pic) { return ::avcodec_default_get_buffer(s, pic); }
  virtual void avcodec_default_release_buffer(AVCodecContext *s, AVFrame *pic) { ::avcodec_default_release_buffer(s, pic); }
  virtual AVCodec *av_codec_next(AVCodec *c) { return ::av_codec_next(c); }
  virtual AVAudioConvert *av_audio_convert_alloc(enum AVSampleFormat out_fmt, int out_channels,
                                                 enum AVSampleFormat in_fmt , int in_channels,
                                                 const float *matrix      , int flags) 
          { return ::av_audio_convert_alloc(out_fmt, out_channels, in_fmt, in_channels, matrix, flags); }
  virtual void av_audio_convert_free(AVAudioConvert *ctx)
          { ::av_audio_convert_free(ctx); }

  virtual int av_audio_convert(AVAudioConvert *ctx,
                                     void * const out[6], const int out_stride[6],
                               const void * const  in[6], const int  in_stride[6], int len)
          { return ::av_audio_convert(ctx, out, out_stride, in, in_stride, len); }

  virtual void av_packet_free(AVPacket *pkt) { ::av_free_packet(pkt); )
  virtual int av_dup_packet(AVPacket *pkt) { return ::av_dup_packet(pkt); }
  virtual int av_init_packet(AVPacket *pkt) { return ::av_init_packet(pkt); }

  // DLL faking.
  virtual bool ResolveExports() { return true; }
  virtual bool Load() {
    CLog::Log(LOGDEBUG, "DllAvCodec: Using libavcodec system library");
    return true;
  }
  virtual void Unload() {}
};
#else
class DllAvCodec : public DllDynamic, DllAvCodecInterface
{
public:
  DllAvCodec() : DllDynamic( CSettings::Get().GetFFmpegDllFolder() + "avcodec-54.dll") {}

  DEFINE_FUNC_ALIGNED1(void, __cdecl, avcodec_flush_buffers, AVCodecContext*)
  DEFINE_FUNC_ALIGNED3(int, __cdecl, avcodec_open2_dont_call, AVCodecContext*, AVCodec *, AVDictionary **)
  DEFINE_FUNC_ALIGNED4(int, __cdecl, avcodec_decode_video2, AVCodecContext*, AVFrame*, int*, AVPacket*)
  DEFINE_FUNC_ALIGNED4(int, __cdecl, avcodec_decode_audio4, AVCodecContext*, AVFrame*, int*, AVPacket*)
  DEFINE_FUNC_ALIGNED4(int, __cdecl, avcodec_decode_subtitle2, AVCodecContext*, AVSubtitle*, int*, AVPacket*)
  DEFINE_FUNC_ALIGNED1(AVCodecContext*, __cdecl, avcodec_alloc_context3, AVCodec *)
  DEFINE_FUNC_ALIGNED1(AVCodecParserContext*, __cdecl, av_parser_init, int)
  DEFINE_FUNC_ALIGNED9(int, __cdecl, av_parser_parse2, AVCodecParserContext*,AVCodecContext*, uint8_t**, int*, const uint8_t*, int, int64_t, int64_t, int64_t)

  LOAD_SYMBOLS();

  DEFINE_METHOD0(void, avcodec_register_all_dont_call)
  DEFINE_METHOD1(AVCodec*, avcodec_find_decoder, (enum CodecID p1))
  DEFINE_METHOD1(int, avcodec_close_dont_call, (AVCodecContext *p1))
  DEFINE_METHOD0(AVFrame*, avcodec_alloc_frame)
  DEFINE_METHOD5(int, avpicture_fill, (AVPicture *p1, uint8_t *p2, int p3, int p4, int p5))
  DEFINE_METHOD3(int, avpicture_get_size, (PixelFormat p1, int p2, int p3))
  DEFINE_METHOD4(void, avcodec_string, (char *p1, int p2, AVCodecContext *p3, int p4))
  DEFINE_METHOD2(void, avcodec_get_context_defaults3, (AVCodecContext *p1, AVCodec *p2))
  DEFINE_METHOD1(void, av_parser_close, (AVCodecParserContext *p1))
  DEFINE_METHOD1(void, avpicture_free, (AVPicture *p1))
  DEFINE_METHOD4(int, avpicture_alloc, (AVPicture *p1, PixelFormat p2, int p3, int p4))
  DEFINE_METHOD2(int, avcodec_default_get_buffer, (AVCodecContext *p1, AVFrame *p2))
  DEFINE_METHOD2(void, avcodec_default_release_buffer, (AVCodecContext *p1, AVFrame *p2))
  DEFINE_METHOD1(AVCodec*, av_codec_next, (AVCodec *p1))
  DEFINE_METHOD6(AVAudioConvert*, av_audio_convert_alloc, (enum AVSampleFormat p1, int p2,
                                                           enum AVSampleFormat p3, int p4,
                                                           const float *p5, int p6))
  DEFINE_METHOD1(void, av_audio_convert_free, (AVAudioConvert *p1))
  DEFINE_METHOD6(int,  av_audio_convert,      (AVAudioConvert *p1,
                                                     void * const p2[6], const int p3[6],
                                               const void * const p4[6], const int p5[6], int p6))
  DEFINE_METHOD1(int, av_dup_packet, (AVPacket *p1))
  DEFINE_METHOD1(int, av_init_packet, (AVPacket *p1))
  DEFINE_METHOD1(void, av_free_packet,        (AVPacket *p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(avcodec_flush_buffers)
    RESOLVE_METHOD_RENAME(avcodec_open2,avcodec_open2_dont_call)
    RESOLVE_METHOD_RENAME(avcodec_close,avcodec_close_dont_call)
    RESOLVE_METHOD(avcodec_find_decoder)
    RESOLVE_METHOD(avcodec_alloc_frame)
    RESOLVE_METHOD_RENAME(avcodec_register_all, avcodec_register_all_dont_call)
    RESOLVE_METHOD(avpicture_fill)
    RESOLVE_METHOD(avcodec_decode_video2)
    RESOLVE_METHOD(avcodec_decode_audio4)
    RESOLVE_METHOD(avcodec_decode_subtitle2)
    RESOLVE_METHOD(avpicture_get_size)
    RESOLVE_METHOD(avcodec_alloc_context3)
    RESOLVE_METHOD(avcodec_string)
    RESOLVE_METHOD(avcodec_get_context_defaults3)
    RESOLVE_METHOD(av_parser_init)
    RESOLVE_METHOD(av_parser_parse2)
    RESOLVE_METHOD(av_parser_close)
    RESOLVE_METHOD(avpicture_free)
    RESOLVE_METHOD(avpicture_alloc)
    RESOLVE_METHOD(avcodec_default_get_buffer)
    RESOLVE_METHOD(avcodec_default_release_buffer)
    RESOLVE_METHOD(av_codec_next)
    RESOLVE_METHOD(av_audio_convert_alloc)
    RESOLVE_METHOD(av_audio_convert_free)
    RESOLVE_METHOD(av_audio_convert)
    RESOLVE_METHOD(av_dup_packet)
    RESOLVE_METHOD(av_free_packet)
    RESOLVE_METHOD(av_init_packet)
  END_METHOD_RESOLVE()

  /* dependency of libavcodec */
  DllAvUtil m_dllAvUtil;

public:
    static CCriticalSection m_critSection;
    int avcodec_open2(AVCodecContext *avctx, AVCodec *codec, AVDictionary **options)
    {
      CSingleLock lock(DllAvCodec::m_critSection);
      return avcodec_open2_dont_call(avctx,codec, options);
    }
    int avcodec_close(AVCodecContext *avctx)
    {
      CSingleLock lock(DllAvCodec::m_critSection);
      return avcodec_close_dont_call(avctx);
    }
    void avcodec_register_all()
    {
      CSingleLock lock(DllAvCodec::m_critSection);
      avcodec_register_all_dont_call();
    }
    virtual bool Load()
    {
      if (!m_dllAvUtil.Load())
        return false;
      return DllDynamic::Load();
    }
};

#endif
