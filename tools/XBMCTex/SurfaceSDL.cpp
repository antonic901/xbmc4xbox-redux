// class CSurface
#include "SurfaceSDL.h"

#ifdef USE_SDL
#include <stdio.h>
#include <stdlib.h>
#include "xbox.h"

CGraphicsDevice g_device;

CSurface::CSurface()
{
  m_surface = NULL;
  m_width = 0;
  m_height = 0;
  m_bpp = 0;
}

CSurface::~CSurface()
{
  if (m_surface)
    SDL_FreeSurface(m_surface);
}

bool CSurface::Create(unsigned int width, unsigned int height, CSurface::FORMAT format)
{
  if (m_surface)
  {
    SDL_FreeSurface(m_surface);
    m_surface = NULL;
  }

  m_info.width = width;
  m_info.height = height;
  m_info.format = format;

  if (format == FMT_LIN_ARGB)
  { // round width to multiple of 64 pixels
    m_width = (width + 63) & ~63;
    m_height = height;
  }
  else
  { // round up to nearest power of 2
    m_width = PadPow2(width);
    m_height = PadPow2(height);
  }
  m_bpp = (format == FMT_PALETTED) ? 1 : 4;

  Uint32 rmask = 0x00ff0000;
  Uint32 gmask = 0x0000ff00;
  Uint32 bmask = 0x000000ff;
  Uint32 amask = 0xff000000;

  if (format == FMT_PALETTED)
  {
    m_surface = SDL_CreateRGBSurface(0, m_width, m_height, 8, 0, 0, 0, 0);
  }
  else
  {
    m_surface = SDL_CreateRGBSurface(0, m_width, m_height, 32, rmask, gmask, bmask, amask);
  }

  if (!m_surface)
    return false;

  Clear();
  return true;
}

void CSurface::Clear()
{
  CSurfaceRect rect;
  if (Lock(&rect))
  {
    BYTE *pixels = rect.pBits;
    for (unsigned int i = 0; i < m_height; i++)
    {
      memset(pixels, 0, rect.Pitch);
      pixels += rect.Pitch;
    }
    Unlock();
  }
}

bool CSurface::CreateFromFile(const char *Filename, FORMAT format)
{
  SDL_Surface *original = IMG_Load(Filename);
  if (!original)
    return false;

  bool isPalettized = (original->format->BitsPerPixel == 8 && original->format->palette != NULL);

  if (format == FMT_PALETTED && isPalettized)
  {
    // Keep original if format matches
    m_surface = original;
    original = NULL; // ownership transferred

    m_info.width = m_surface->w;
    m_info.height = m_surface->h;
    m_info.format = format;
    m_width = PadPow2(m_info.width);
    m_height = PadPow2(m_info.height);
    m_bpp = 1;

    ClampToEdge();  // simulate GL_CLAMP_TO_EDGE
    return true;
  }

  // Otherwise fall back to ARGB
  if (!Create(original->w, original->h, format))
  {
    SDL_FreeSurface(original);
    return false;
  }

  SDL_SetSurfaceBlendMode(original, SDL_BLENDMODE_NONE);
  int ret = SDL_BlitSurface(original, NULL, m_surface, NULL);

  // If original was palettized and we created an 8-bit surface manually
  if (original->format->palette && m_surface->format->palette)
  {
    SDL_SetPaletteColors(m_surface->format->palette,
                         original->format->palette->colors,
                         0,
                         original->format->palette->ncolors);
  }

  SDL_FreeSurface(original);
  ClampToEdge();
  return (ret == 0);
}

void CSurface::ClampToEdge()
{
  // fix up the last row and column to simulate clamp_to_edge
  if (!m_info.width || !m_info.height)
    return; // invalid texture

  CSurfaceRect rect;
  if (Lock(&rect))
  {
    for (unsigned int y = 0; y < m_info.height; y++)
    {
      BYTE *src = rect.pBits + y * rect.Pitch;
      for (unsigned int x = m_info.width; x < m_width; x++)
      {
        if (m_info.width >= 1)
          memcpy(src + x * m_bpp, src + (m_info.width - 1) * m_bpp, m_bpp);
      }
    }

    if (m_info.height >= 1)
    {
      BYTE *src = rect.pBits + (m_info.height - 1) * rect.Pitch;
      for (unsigned int y = m_info.height; y < m_height; y++)
      {
        BYTE *dest = rect.pBits + y * rect.Pitch;
        memcpy(dest, src, rect.Pitch);
      }
    }

    Unlock();
  }
}

bool CSurface::Lock(CSurfaceRect *rect)
{
  if (m_surface && rect)
  {
    if (SDL_LockSurface(m_surface) == 0)
    {
      rect->pBits = (BYTE *)m_surface->pixels;
      rect->Pitch = m_surface->pitch;
      return true;
    }
  }
  return false;
}

bool CSurface::Unlock()
{
  if (m_surface)
  {
    SDL_UnlockSurface(m_surface);
    return true;
  }
  return false;
}

CGraphicsDevice::CGraphicsDevice()
{
}

CGraphicsDevice::~CGraphicsDevice()
{
}

bool CGraphicsDevice::Create()
{
  putenv("SDL_VIDEODRIVER=dummy");

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
  {
    printf("SDL Initialization failed: %s\n", SDL_GetError());
    return false;
  }

  if (!(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF) & (IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF)))
  {
    printf("SDL_image Initialization failed: %s\n", IMG_GetError());
    SDL_Quit();
    return false;
  }

  return true;
}

bool CGraphicsDevice::CreateSurface(unsigned int width, unsigned int height, enum CSurface::FORMAT format, CSurface *surface)
{
  return false;
}
#endif

