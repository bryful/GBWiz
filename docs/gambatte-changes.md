# Gambatteの変更記録

上流はlibretro/gambatte-libretroの7809bb68ee96c3e6c74e9d9c4c8bb09aa7c6801dです。変更者はbryful。GBWiz向けの変更は2026年10月以降に実装し、2026-11-02に公開用の変更表記を各ファイルへ追加しました。既存のGPLv2・著作権表示を維持します。

- `libgambatte/include/gambatte.h`: Expose optional CPU write filtering, CPU registers and mapped ROM access.
- `libgambatte/src/cpu.cpp`: Apply optional write filters while preserving emulated bus timing.
- `libgambatte/src/cpu.h`: Store the optional write filter callback and its context.
- `libgambatte/src/gambatte-memory.h`: Expose a read-only mapped ROM byte accessor.
- `libgambatte/src/gambatte.cpp`: Implement public observers/accessors and initialize save-state working data.
- `libgambatte/src/video_libretro.cpp`: Expand RGB32 color components from 5 bits to 8 bits.
