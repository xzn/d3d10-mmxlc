# Mega Man X Legacy Collection `d3d10.dll` wrapper mod

Features:

- Let you use [slang-shaders](https://github.com/libretro/slang-shaders) with Capcom's Mega Man X Legacy Collection.
- Fixes scaling artifact due to nearest-neighbour upscaling.

Download from [here](https://github.com/xzn/d3d10-mmxlc/releases).

## Building from source

Using i686-w64-mingw32-gcc (cross compiling should work too):

```bash
# Download source
git clone https://github.com/xzn/d3d10-mmxlc.git
cd d3d10-mmxlc
git submodule update --init --recursive

# Create symlinks and patch files
make prep

# Build the dll
make -j$(nproc) dll
```

Some options to pass to make

```bash
# disable optimizations and prevents stripping
make o3=0 dll

# disable lto (keep -O3)
make lto=0 dll
```

## Install

Copy `dinput8.dll`, `interp-mod.ini`, and the `slang-shaders\` directory to your game folders, e.g.:

- `SteamLibrary\steamapps\common\Mega Man X Legacy Collection`
- `SteamLibrary\steamapps\common\Mega Man X Legacy Collection 2`

## Configuration

`interp-mod.ini` contains options to configure the mod.

```ini
; Log API calls to interp-mod.log,
; Should be left disabled. Really slow and for the most part isn't even useful for debugging.
; [logging]
; enabled=true
; hotkey_toggle=VK_CONTROL+O
; hotkey_frame=VK_CONTROL+P

; Change interpolation mode and set up custom shaders.
[graphics]
; Recommended, even if slang-shader is not used; get rid of nearest-neighbour upscaling artifact.
interp=true
; Slightly reduces aliasing in X7 and X8
; linear=true
; Not recommended, very slow - when using Type 1 filter, run the default 2x upscale shader multiple times until it covers the size of the screen.
; enhanced=true
; Set up custom shader for X1~X6, and 3d shader for X7~X8. Need Type 1 filter to be set in-game.
slang_shader=slang-shaders/xbrz/xbrz-freescale-multipass.slangp
slang_shader_3d=slang-shaders/anti-aliasing/smaa.slangp
; Custom render resolution for X7 and X8
render_3d_width=
render_3d_height=
; Custom display resolution (can be higher or lower than the games allow)
display_width=
display_height=
```

If all goes well you should now be able to start the game and see the overlay on top-left of the screen showing the status of the mod.

`interp-mod.ini` can be edited and have its options applied while the game is running.

## License

Source code for this mod, without its dependencies, is available under MIT. Dependencies such as `RetroArch` are released under GPL.

- `RetroArch` is needed only for `slang_shader` support.
- `SPIRV-Cross` and `glslang` are used for `slang_shader` support.
- `HLSLcc` is used for debugging.

Other dependencies are more or less required:

- `minhook` is used for intercepting calls to `d3d10.dll`.
- `imgui` is used for overlay display.
- `smhasher` is technically optional. Currently used for identifying the built-in Type 1 filter shader.
