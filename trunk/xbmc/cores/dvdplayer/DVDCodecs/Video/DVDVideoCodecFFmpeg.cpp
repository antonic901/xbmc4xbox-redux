/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 */

#include "system.h"
#include "utils/log.h"
#include "DVDVideoCodecFFmpeg.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDStreamInfo.h"
#include "DVDClock.h"
#include "DVDCodecs/DVDCodecs.h"
#include "utils/Win32Exception.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"

#define RINT(x) ((x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5)))


CDVDVideoCodecFFmpeg::CDVDVideoCodecFFmpeg() : CDVDVideoCodec()
{
  m_pCodecContext = NULL;
  m_pConvertFrame = NULL;
  m_pFrame = NULL;

  m_iPictureWidth = 0;
  m_iPictureHeight = 0;

  m_iScreenWidth = 0;
  m_iScreenHeight = 0;
  m_dts = DVD_NOPTS_VALUE;
}

CDVDVideoCodecFFmpeg::~CDVDVideoCodecFFmpeg()
{
  Dispose();
}

bool CDVDVideoCodecFFmpeg::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  AVCodec* pCodec;

  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load()) return false;

  m_dllAvCodec.avcodec_register_all();

  pCodec = m_dllAvCodec.avcodec_find_decoder(hints.codec);
  if(pCodec == NULL)
  {
    CLog::Log(LOGDEBUG,"CDVDVideoCodecFFmpeg::Open() Unable to find codec %d", hints.codec);
    return false;
  }

  CLog::Log(LOGNOTICE,"CDVDVideoCodecFFmpeg::Open() Using codec: %s",pCodec->long_name ? pCodec->long_name : pCodec->name);

  m_pCodecContext = m_dllAvCodec.avcodec_alloc_context3(pCodec);

  m_pCodecContext->opaque = (void*)this;
  m_pCodecContext->debug_mv = 0;
  m_pCodecContext->debug = 0;
  m_pCodecContext->workaround_bugs = FF_BUG_AUTODETECT;
  m_pCodecContext->codec_tag = hints.codec_tag;

  if (pCodec->capabilities & CODEC_CAP_DR1)
    m_pCodecContext->flags |= CODEC_FLAG_EMU_EDGE;

  // allow non spec compliant speedup tricks
  if (g_guiSettings.GetBool("videoplayer.fast"))
    m_pCodecContext->flags2 |= CODEC_FLAG2_FAST;

  // if we don't do this, then some codecs seem to fail.
  m_pCodecContext->coded_height = hints.height;
  m_pCodecContext->coded_width = hints.width;
  m_pCodecContext->bits_per_coded_sample = hints.bitsperpixel;

  if( hints.extradata && hints.extrasize > 0 )
  {
    m_pCodecContext->extradata_size = hints.extrasize;
    m_pCodecContext->extradata = (uint8_t*)m_dllAvUtil.av_mallocz(hints.extrasize + FF_INPUT_BUFFER_PADDING_SIZE);
    memcpy(m_pCodecContext->extradata, hints.extradata, hints.extrasize);
  }

  // set acceleration
  m_pCodecContext->dsp_mask = AV_CPU_FLAG_FORCE | AV_CPU_FLAG_MMX | AV_CPU_FLAG_MMX2 | AV_CPU_FLAG_SSE;

  AVDiscard discardVals[] = {AVDISCARD_DEFAULT, AVDISCARD_NONREF, AVDISCARD_BIDIR, AVDISCARD_NONKEY, AVDISCARD_ALL};
  AVDiscard avDiscard = discardVals[g_guiSettings.GetInt("videoplayer.skiploopfilter")];
  if (avDiscard != AVDISCARD_DEFAULT)
    m_pCodecContext->skip_loop_filter = avDiscard;

  // set any special options
  for(std::vector<CDVDCodecOption>::iterator it = options.m_keys.begin(); it != options.m_keys.end(); it++)
  {
    m_dllAvUtil.av_opt_set(m_pCodecContext, it->m_name.c_str(), it->m_value.c_str(), 0);
  }
  
  if (m_dllAvCodec.avcodec_open2(m_pCodecContext, pCodec, NULL) < 0)
  {
    CLog::Log(LOGDEBUG,"CDVDVideoCodecFFmpeg::Open() Unable to open codec");
    return false;
  }

  m_pFrame = m_dllAvCodec.avcodec_alloc_frame();
  if (!m_pFrame) return false;

  if(pCodec->name)
    m_name = CStdString("ff-") + pCodec->name;
  else
    m_name = "ffmpeg";

  return true;
}

void CDVDVideoCodecFFmpeg::Dispose()
{
  if (m_pFrame) m_dllAvUtil.av_free(m_pFrame);
  m_pFrame = NULL;

  if (m_pConvertFrame)

  {
    m_dllAvCodec.avpicture_free(m_pConvertFrame);
    m_dllAvUtil.av_free(m_pConvertFrame);
  }
  m_pConvertFrame = NULL;

  if (m_pCodecContext)
  {
    if (m_pCodecContext->codec) m_dllAvCodec.avcodec_close(m_pCodecContext);
    if (m_pCodecContext->extradata)
    {
      m_dllAvUtil.av_free(m_pCodecContext->extradata);
      m_pCodecContext->extradata = NULL;
      m_pCodecContext->extradata_size = 0;
    }
    m_dllAvUtil.av_free(m_pCodecContext);
    m_pCodecContext = NULL;
  }

  m_dllAvCodec.Unload();
  m_dllAvUtil.Unload();
}

void CDVDVideoCodecFFmpeg::SetDropState(bool bDrop)
{
  if( m_pCodecContext )
  {
    if( bDrop )
    {
      m_pCodecContext->skip_frame = AVDISCARD_NONREF;
      m_pCodecContext->skip_idct = AVDISCARD_NONREF;
    }
    else
    {
      m_pCodecContext->skip_frame = AVDISCARD_DEFAULT;
      m_pCodecContext->skip_idct = AVDISCARD_DEFAULT;
    }
  }
}

union pts_union
{
  double  pts_d;
  int64_t pts_i;
};

static int64_t pts_dtoi(double pts)
{
  pts_union u;
  u.pts_d = pts;
  return u.pts_i;
}

static double pts_itod(int64_t pts)
{
  pts_union u;
  u.pts_i = pts;
  return u.pts_d;
}

int CDVDVideoCodecFFmpeg::Decode(BYTE* pData, int iSize, double dts, double pts)
{
  int iGotPicture = 0, len = 0;

  if (!m_pCodecContext)
    return VC_ERROR;

  m_dts = dts;
  m_pCodecContext->reordered_opaque = pts_dtoi(pts);

  AVPacket avpkt;
  m_dllAvCodec.av_init_packet(&avpkt);
  avpkt.data = pData;
  avpkt.size = iSize;
  /* We lie, but this flag is only used by pngdec.c.
   * Setting it correctly would allow CorePNG decoding. */
  avpkt.flags = AV_PKT_FLAG_KEY;
  len = m_dllAvCodec.avcodec_decode_video2(m_pCodecContext, m_pFrame, &iGotPicture, &avpkt);

  if (len < 0)
  {
    CLog::Log(LOGERROR, "%s - avcodec_decode_video returned failure", __FUNCTION__);
    return VC_ERROR;
  }

  if (!iGotPicture)
    return VC_BUFFER;

  if(m_pCodecContext->pix_fmt != PIX_FMT_YUV420P
  && m_pCodecContext->pix_fmt != PIX_FMT_YUVJ420P)
  {
    if (!m_dllSwScale.IsLoaded())
    {
      if(!m_dllSwScale.Load())
        return VC_ERROR;

      m_dllSwScale.sws_rgb2rgb_init(SWS_CPU_CAPS_MMX2);    
    }

    if (!m_pConvertFrame)
    {
      // Allocate an AVFrame structure
      m_pConvertFrame = (AVPicture*)m_dllAvUtil.av_mallocz(sizeof(AVPicture));
      // Due to a bug in swsscale we need to allocate one extra line of data
      if(m_dllAvCodec.avpicture_alloc( m_pConvertFrame
                                     , PIX_FMT_YUV420P
                                     , m_pCodecContext->width
                                     , m_pCodecContext->height+1) < 0)
      {
        m_dllAvUtil.av_free(m_pConvertFrame);
        m_pConvertFrame = NULL;
        return VC_ERROR;
      }
    }

    // convert the picture
    struct SwsContext *context = m_dllSwScale.sws_getContext(m_pCodecContext->width, m_pCodecContext->height,
                                         m_pCodecContext->pix_fmt, m_pCodecContext->width, m_pCodecContext->height,
                                         PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);

    if(context == NULL)
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::Decode - unable to obtain sws context for w:%i, h:%i, pixfmt: %i", m_pCodecContext->width, m_pCodecContext->height, m_pCodecContext->pix_fmt);
      return VC_ERROR;
    }

    m_dllSwScale.sws_scale(context
                          , m_pFrame->data
                          , m_pFrame->linesize
                          , 0
                          , m_pCodecContext->height
                          , m_pConvertFrame->data
                          , m_pConvertFrame->linesize);

    m_dllSwScale.sws_freeContext(context);
  }
  else
  {
    // no need to convert, just free any existing convert buffers
    if (m_pConvertFrame)
    {
      m_dllAvCodec.avpicture_free(m_pConvertFrame);
      m_dllAvUtil.av_free(m_pConvertFrame);
      m_pConvertFrame = NULL;
    }
  }

  int result = VC_PICTURE | VC_BUFFER;

  if(result & VC_FLUSHED)
    Reset();

  return result;
}

void CDVDVideoCodecFFmpeg::Reset()
{
  m_dllAvCodec.avcodec_flush_buffers(m_pCodecContext);

  if (m_pConvertFrame)
  {
    delete[] m_pConvertFrame->data[0];
    m_dllAvUtil.av_free(m_pConvertFrame);
    m_pConvertFrame = NULL;
  }
}

bool CDVDVideoCodecFFmpeg::GetPictureCommon(DVDVideoPicture* pDvdVideoPicture)
{
  GetVideoAspect(m_pCodecContext, pDvdVideoPicture->iDisplayWidth, pDvdVideoPicture->iDisplayHeight);

  if(m_pCodecContext->coded_width  && m_pCodecContext->coded_width  < m_pCodecContext->width
                                   && m_pCodecContext->coded_width  > m_pCodecContext->width  - 10)
    pDvdVideoPicture->iWidth = m_pCodecContext->coded_width;
  else
    pDvdVideoPicture->iWidth = m_pCodecContext->width;

  if(m_pCodecContext->coded_height && m_pCodecContext->coded_height < m_pCodecContext->height
                                   && m_pCodecContext->coded_height > m_pCodecContext->height - 10)
    pDvdVideoPicture->iHeight = m_pCodecContext->coded_height;
  else
    pDvdVideoPicture->iHeight = m_pCodecContext->height;

  pDvdVideoPicture->pts = DVD_NOPTS_VALUE;

  if (!m_pFrame)
    return false;

  pDvdVideoPicture->iRepeatPicture = 0.5 * m_pFrame->repeat_pict;
  pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED;
  pDvdVideoPicture->iFlags |= m_pFrame->interlaced_frame ? DVP_FLAG_INTERLACED : 0;
  pDvdVideoPicture->iFlags |= m_pFrame->top_field_first ? DVP_FLAG_TOP_FIELD_FIRST: 0;
  if(m_pCodecContext->pix_fmt == PIX_FMT_YUVJ420P)
    pDvdVideoPicture->color_range = 1;

  pDvdVideoPicture->qscale_table = m_pFrame->qscale_table;
  pDvdVideoPicture->qscale_stride = m_pFrame->qstride;

  switch (m_pFrame->qscale_type) {
  case FF_QSCALE_TYPE_MPEG1:
    pDvdVideoPicture->qscale_type = DVP_QSCALE_MPEG1;
    break;
  case FF_QSCALE_TYPE_MPEG2:
    pDvdVideoPicture->qscale_type = DVP_QSCALE_MPEG2;
    break;
  case FF_QSCALE_TYPE_H264:
    pDvdVideoPicture->qscale_type = DVP_QSCALE_H264;
    break;
  default:
    pDvdVideoPicture->qscale_type = DVP_QSCALE_UNKNOWN;
  }

  pDvdVideoPicture->dts = m_dts;
  m_dts = DVD_NOPTS_VALUE;
  if (m_pFrame->reordered_opaque)
    pDvdVideoPicture->pts = pts_itod(m_pFrame->reordered_opaque);
  else
    pDvdVideoPicture->pts = DVD_NOPTS_VALUE;

  return true;
}

bool CDVDVideoCodecFFmpeg::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  if(!GetPictureCommon(pDvdVideoPicture))
    return false;

  if(m_pConvertFrame)
  {
    for (int i = 0; i < 4; i++)
      pDvdVideoPicture->data[i]      = m_pConvertFrame->data[i];
    for (int i = 0; i < 4; i++)
      pDvdVideoPicture->iLineSize[i] = m_pConvertFrame->linesize[i];
  }
  else
  {
    for (int i = 0; i < 4; i++)
      pDvdVideoPicture->data[i]      = m_pFrame->data[i];
    for (int i = 0; i < 4; i++)
      pDvdVideoPicture->iLineSize[i] = m_pFrame->linesize[i];
  }

  pDvdVideoPicture->iFlags |= pDvdVideoPicture->data[0] ? 0 : DVP_FLAG_DROPPED;
  pDvdVideoPicture->format = DVDVideoPicture::FMT_YUV420P;

  return true;
}

/*
 * Calculate the height and width this video should be displayed in
 */
void CDVDVideoCodecFFmpeg::GetVideoAspect(AVCodecContext* pCodecContext, unsigned int& iWidth, unsigned int& iHeight)
{
  double aspect_ratio;

  /* XXX: use variable in the frame */
  if (pCodecContext->sample_aspect_ratio.num == 0) aspect_ratio = 0;
  else aspect_ratio = av_q2d(pCodecContext->sample_aspect_ratio) * pCodecContext->width / pCodecContext->height;

  if (aspect_ratio <= 0.0) aspect_ratio = (float)pCodecContext->width / (float)pCodecContext->height;

  /* XXX: we suppose the screen has a 1.0 pixel ratio */ // CDVDVideo will compensate it.
  iHeight = pCodecContext->height;
  iWidth = ((int)RINT(pCodecContext->height * aspect_ratio)) & -3;
  if (iWidth > (unsigned int)pCodecContext->width)
  {
    iWidth = pCodecContext->width;
    iHeight = ((int)RINT(pCodecContext->width / aspect_ratio)) & -3;
  }
}
