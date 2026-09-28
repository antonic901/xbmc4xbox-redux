/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "rfft.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace
{
double filter(const kiss_fft_cpx& data, size_t size, bool windowed)
{
  return static_cast<double>(sqrt(data.r * data.r + data.i * data.i)) * 2.0 / size *
         (windowed ? sqrt(8.0 / 3.0) : 1.0);
}
}

RFFT::RFFT(int size, bool windowed) :
  m_size(size), m_windowed(windowed)
{
  m_cfg = kiss_fftr_alloc(m_size,0,NULL,NULL);
}

RFFT::~RFFT()
{
  // we don' use kiss_fftr_free here because
  // its hardcoded to free and doesn't pay attention
  // to SIMD (which might be used during kiss_fftr_alloc
  //in the C'tor).
  KISS_FFT_FREE(m_cfg);
}

void RFFT::calc(const float* input, float* output)
{
  // temporary buffers
  std::vector<kiss_fft_scalar> linput(m_size), rinput(m_size);
  std::vector<kiss_fft_cpx> loutput(m_size), routput(m_size);

  for (size_t i=0;i<m_size;++i)
  {
    linput[i] = input[2*i];
    rinput[i] = input[2*i+1];
  }

  if (m_windowed)
  {
    hann(linput);
    hann(rinput);
  }

  // transform channels
  kiss_fftr(m_cfg, &linput[0], &loutput[0]);
  kiss_fftr(m_cfg, &rinput[0], &routput[0]);

  // interleave while taking magnitudes and normalizing
  for (size_t i=0;i<m_size/2;++i)
  {
    output[2*i] = filter(loutput[i], m_size, m_windowed);
    output[2*i+1] = filter(routput[i], m_size, m_windowed);
  }
}

void RFFT::hann(std::vector<kiss_fft_scalar>& data)
{
  for (size_t i=0;i<data.size();++i)
    data[i] *= 0.5f * (1.0f - cos(2.0f * static_cast<float>(M_PI) * i / (data.size() - 1)));
}
