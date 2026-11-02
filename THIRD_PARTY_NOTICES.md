# Third-party licenses and GBWiz licensing

GBWiz original code, documentation, and original assets are licensed under the
MIT License in LICENSE. This grant does not relicense third-party code, third-party
assets, or third-party portions of modified files.

## Gambatte — GPL-2.0-only

- Upstream: https://github.com/libretro/gambatte-libretro
- Imported revision: 7809bb68ee96c3e6c74e9d9c4c8bb09aa7c6801d
- License: third_party/gambatte/COPYING
- Original copyright notices and component-specific notices remain in their files.

GBWiz links Gambatte into its Windows and Android applications. Distribution of
these combined applications must satisfy GPL version 2, including the applicable
corresponding-source requirements. The MIT license for GBWiz's original portions
is retained; it does not make Gambatte or the combined application MIT-only.

GBWiz modifications to Gambatte are listed in docs/gambatte-changes.md and marked
in the modified files. Those files remain subject to their existing GPLv2 terms.

## SDL2 — zlib

- Upstream: https://github.com/libsdl-org/SDL
- Version: 2.32.10, official Visual C++ development package
- License and copyright: third_party/SDL2-2.32.10/LICENSE.txt
- The imported SDL2 files have not been modified by GBWiz.

## blipper — MIT

- Included through Gambatte under third_party/gambatte/libgambatte/libretro/
- Copyright (c) 2013 Hans-Kristian Arntzen
- License text: the header of blipper.c, also copied to blipper-LICENSE.txt in releases

Other imported files retain their own notices, including Gradle wrapper and
libretro-common files. LICENSE does not override those notices.

## Corresponding source for 0.1.2

- Tagged source: https://github.com/bryful/GBWiz/tree/v0.1.2
- Source archive: https://github.com/bryful/GBWiz/archive/refs/tags/v0.1.2.zip
- Build instructions: README.md and android/README.md

The v0.1.2 tag includes the application source, modified Gambatte source, vendored
Windows SDL2 headers/libraries, and build/package scripts. ROMs, private signing
keys, signing credentials, and the non-distributed local gameplay fixtures are
not required to build the application and are not distributed.
