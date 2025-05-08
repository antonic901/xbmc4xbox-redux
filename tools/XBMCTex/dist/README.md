# XBMCTex distribution

This folder contains the XBMCTex executable together with its runtime dependencies.

## Contents

- `XBMCTex.exe` – The compiled application, built with MSVC 2003.
- `SDL2.dll` – Runtime library for SDL2 (Simple DirectMedia Layer).
- `SDL2_image.dll` – Image loading library that extends SDL2.
- `libtiff-5.dll` – TIFF image format support library used by SDL2_image.

## DLL Sources

These DLLs were downloaded from the official SDL2 project:

- **SDL2.dll** from [SDL2-2.32.6-win32-x86.zip](https://github.com/libsdl-org/SDL/releases/tag/release-2.32.6)
- **SDL2_image.dll** and **libtiff-5.dll** from [SDL2_image-2.8.8-win32-x86.zip](https://github.com/libsdl-org/SDL_image/releases/tag/release-2.8.8)

## License Information

All libraries included are open-source and permissively licensed:

### SDL2

- License: [zlib License](https://github.com/libsdl-org/SDL/blob/main/LICENSE.txt)
- Copyright © Sam Lantinga
- Redistribution and use in source and binary forms, with or without modification, are permitted provided that the conditions of the zlib license are met.

### SDL2_image

- License: [zlib License](https://github.com/libsdl-org/SDL_image/blob/main/LICENSE.txt)
- Copyright © Sam Lantinga

### libtiff

- License: LibTIFF License ([link](http://www.simplesystems.org/libtiff/project/license.html))
- May also incorporate components under the zlib and IJG (Independent JPEG Group) licenses.

All necessary license files are located in the `licenses/` directory.

## Attribution

This distribution includes unmodified DLLs as provided by the official SDL project. No modifications have been made to these libraries. This application is not affiliated with or endorsed by the SDL2 project.

---

