smhasher_src := smhasher/src/MurmurHash3.cpp
smhasher_obj := $(smhasher_src:smhasher/src/%.cpp=obj/smhasher/%.o)
smhasher_dir := $(sort $(dir $(smhasher_obj)))
HLSLcc_src := $(wildcard HLSLcc/src/*.cpp)
HLSLcc_obj := $(HLSLcc_src:HLSLcc/src/%.cpp=obj/HLSLcc/%.o)
HLSLcc_dir := $(sort $(dir $(HLSLcc_obj)))
cbstring_src := $(wildcard HLSLcc/src/cbstring/*.c)
cbstring_obj := $(cbstring_src:HLSLcc/src/cbstring/%.c=obj/cbstring/%.o)
cbstring_dir := $(sort $(dir $(cbstring_obj)))
imgui_src := $(wildcard imgui/*.cpp) imgui/examples/imgui_impl_dx10.cpp
imgui_obj := $(imgui_src:imgui/%.cpp=obj/imgui/%.o)
imgui_dir := $(sort $(dir $(imgui_obj)))
minhook_src := $(wildcard minhook/src/*.c minhook/src/hde/*.c)
minhook_obj := $(minhook_src:minhook/src/%.c=obj/minhook/%.o)
minhook_dir := $(sort $(dir $(minhook_obj)))
spirv_src := $(wildcard SPIRV-Cross/spirv_*.cpp)
spirv_obj := $(spirv_src:SPIRV-Cross/%.cpp=obj/SPIRV-Cross/%.o)
spirv_dir := $(sort $(dir $(spirv_obj)))
glslang_src := $(wildcard glslang/glslang/glslang/GenericCodeGen/*.cpp) $(wildcard glslang/glslang/glslang/MachineIndependent/*.cpp) $(wildcard glslang/glslang/glslang/MachineIndependent/preprocessor/*.cpp) glslang/glslang/glslang/OSDependent/Windows/ossource.cpp $(wildcard glslang/glslang/hlsl/*.cpp) $(wildcard glslang/glslang/OGLCompilersDLL/*.cpp) $(wildcard glslang/glslang/SPIRV/*.cpp)
glslang_obj := $(glslang_src:glslang/glslang/%.cpp=obj/glslang/%.o)
glslang_dir := $(sort $(dir $(glslang_obj)))
retroarch_obj := \
frontend/frontend.o \
frontend/frontend_driver.o \
frontend/drivers/platform_null.o \
ui/ui_companion_driver.o \
ui/drivers/ui_null.o \
ui/drivers/null/ui_null_window.o \
ui/drivers/null/ui_null_browser_window.o \
ui/drivers/null/ui_null_msg_window.o \
ui/drivers/null/ui_null_application.o \
core_impl.o \
retroarch.o \
dirs.o \
paths.o \
command.o \
msg_hash.o \
intl/msg_hash_us.o \
libretro-common/queues/task_queue.o \
tasks/task_content.o \
tasks/task_save.o \
tasks/task_file_transfer.o \
tasks/task_image.o \
tasks/task_audio_mixer.o \
libretro-common/encodings/encoding_utf.o \
libretro-common/encodings/encoding_crc32.o \
libretro-common/compat/fopen_utf8.o \
libretro-common/lists/file_list.o \
libretro-common/lists/dir_list.o \
libretro-common/file/retro_dirent.o \
libretro-common/streams/stdin_stream.o \
libretro-common/streams/file_stream.o \
libretro-common/streams/file_stream_transforms.o \
libretro-common/streams/interface_stream.o \
libretro-common/streams/memory_stream.o \
libretro-common/vfs/vfs_implementation.o \
libretro-common/lists/string_list.o \
libretro-common/string/stdstring.o \
libretro-common/memmap/memalign.o \
setting_list.o \
list_special.o \
libretro-common/file/nbio/nbio_stdio.o \
libretro-common/file/nbio/nbio_linux.o \
libretro-common/file/nbio/nbio_unixmmap.o \
libretro-common/file/nbio/nbio_windowsmmap.o \
libretro-common/file/nbio/nbio_intf.o \
libretro-common/file/file_path.o \
file_path_special.o \
file_path_str.o \
libretro-common/hash/rhash.o \
audio/audio_driver.o \
libretro-common/audio/audio_mixer.o \
input/common/input_common.o \
input/input_driver.o \
input/input_mapper.o \
led/led_driver.o \
led/drivers/led_null.o \
gfx/video_coord_array.o \
gfx/video_display_server.o \
gfx/video_driver.o \
gfx/video_crt_switch.o \
camera/camera_driver.o \
wifi/wifi_driver.o \
location/location_driver.o \
driver.o \
configuration.o \
libretro-common/dynamic/dylib.o \
dynamic.o \
cores/dynamic_dummy.o \
libretro-common/queues/message_queue.o \
managers/core_manager.o \
managers/state_manager.o \
gfx/drivers_font_renderer/bitmapfont.o \
tasks/task_autodetect.o \
input/input_autodetect_builtin.o \
input/input_keymaps.o \
input/input_remapping.o \
libretro-common/queues/fifo_queue.o \
managers/core_option_manager.o \
libretro-common/compat/compat_fnmatch.o \
libretro-common/compat/compat_posix_string.o \
managers/cheat_manager.o \
core_info.o \
libretro-common/file/config_file.o \
libretro-common/file/config_file_userdata.o \
tasks/task_screenshot.o \
tasks/task_powerstate.o \
libretro-common/gfx/scaler/scaler.o \
gfx/drivers_shader/shader_null.o \
gfx/video_shader_parse.o \
libretro-common/gfx/scaler/pixconv.o \
libretro-common/gfx/scaler/scaler_int.o \
libretro-common/gfx/scaler/scaler_filter.o \
gfx/font_driver.o \
gfx/video_filter.o \
libretro-common/audio/resampler/audio_resampler.o \
libretro-common/audio/dsp_filter.o \
libretro-common/audio/resampler/drivers/sinc_resampler.o \
libretro-common/audio/resampler/drivers/nearest_resampler.o \
libretro-common/audio/resampler/drivers/null_resampler.o \
libretro-common/utils/md5.o \
location/drivers/nulllocation.o \
camera/drivers/nullcamera.o \
wifi/drivers/nullwifi.o \
gfx/drivers/nullgfx.o \
gfx/display_servers/dispserv_null.o \
audio/drivers/nullaudio.o \
input/drivers/nullinput.o \
input/drivers_hid/null_hid.o \
input/drivers_joypad/null_joypad.o \
playlist.o \
movie.o \
record/record_driver.o \
record/drivers/record_null.o \
libretro-common/features/features_cpu.o \
performance_counters.o \
verbosity.o \
midi/midi_driver.o \
midi/drivers/null_midi.o \
libretro-common/compat/compat_getopt.o \
libretro-common/compat/compat_strcasestr.o \
libretro-common/compat/compat_strl.o \
libretro-common/formats/image_texture.o \
libretro-common/formats/xml/rxml.o \
libretro-common/formats/tga/rtga.o \
libretro-common/formats/png/rpng.o \
libretro-common/formats/png/rpng_encode.o \
libretro-common/formats/jpeg/rjpeg.o \
libretro-common/formats/bmp/rbmp.o \
libretro-common/file/archive_file.o \
libretro-common/streams/trans_stream.o \
libretro-common/streams/trans_stream_pipe.o \
libretro-common/file/archive_file_zlib.o \
libretro-common/streams/trans_stream_zlib.o \
deps/libz/adler32.o \
deps/libz/compress.o \
deps/libz/libz-crc32.o \
deps/libz/deflate.o \
deps/libz/gzclose.o \
deps/libz/gzlib.o \
deps/libz/gzread.o \
deps/libz/gzwrite.o \
deps/libz/inffast.o \
deps/libz/inflate.o \
deps/libz/inftrees.o \
deps/libz/trees.o \
deps/libz/uncompr.o \
deps/libz/zutil.o \
tasks/task_decompress.o \
gfx/common/win32_common.o \
frontend/drivers/platform_win32.o \
gfx/drivers/gdi_gfx.o \
gfx/drivers_context/gdi_ctx.o \
gfx/drivers_font/gdi_font.o \
gfx/display_servers/dispserv_win32.o \
menu/drivers_display/menu_display_gdi.o \
input/drivers/winraw_input.o \
ui/drivers/ui_win32.o \
ui/drivers/win32/ui_win32_window.o \
ui/drivers/win32/ui_win32_browser_window.o \
ui/drivers/win32/ui_win32_msg_window.o \
ui/drivers/win32/ui_win32_application.o \
gfx/drivers_context/gfx_null_ctx.o \
gfx/video_state_tracker.o \
libretro-common/formats/bmp/rbmp_encode.o \
libretro-common/formats/json/jsonsax.o \
libretro-common/formats/json/jsonsax_full.o \
libretro-common/formats/image_transfer.o \
libretro-common/audio/conversion/s16_to_float.o \
libretro-common/audio/conversion/float_to_s16.o \
libretro-common/audio/audio_mix.o \
libretro-common/formats/wav/rwav.o \
gfx/drivers/d3d10.o \
gfx/common/d3d10_common.o \
gfx/drivers_font/d3d10_font.o \
gfx/common/d3dcompiler_common.o \
gfx/common/dxgi_common.o \
input/drivers/dinput.o \
input/drivers_joypad/dinput_joypad.o \
gfx/drivers_shader/slang_process.o \
gfx/drivers_shader/slang_preprocess.o \
gfx/drivers_shader/glslang_util.o \
gfx/drivers_shader/slang_reflection.o \
deps/glslang/glslang.o
retroarch_obj := $(addprefix obj/RetroArch/,$(retroarch_obj))
retroarch_dir := $(sort $(dir $(retroarch_obj)))
retroarch_flg := -DRARCH_INTERNAL -DHAVE_MAIN -DHAVE_D3D10 -DHAVE_DINPUT -DHAVE_SLANG -DHAVE_GLSLANG -DHAVE_SPIRV_CROSS -DENABLE_HLSL -DHAVE_RTGA -DHAVE_RPNG -DHAVE_RJPEG -DHAVE_RBMP -DHAVE_ZLIB -DWANT_ZLIB -DHAVE_COMPRESSION -IRetroArch/RetroArch/libretro-common/include -I. -Iglslang -ISPIRV-Cross -IRetroArch/RetroArch -IRetroArch/RetroArch/deps
retroarch_cc = gcc $(color_opt) -c -MMD -MP -o $@ $< $(o3_opt) $(lto_opt) $(retroarch_flg) -Wno-implicit-function-declaration
retroarch_cxx = g++ $(color_opt) -c -MMD -MP -o $@ $< -std=c++17 $(o3_opt) $(lto_opt) -D__STDC_CONSTANT_MACROS $(retroarch_flg)
src := $(wildcard src/*.cpp)
obj := $(src:src/%.cpp=obj/%.o)
dir := $(sort $(dir $(obj)))
cxx = g++ $(color_opt) -c -MMD -MP -o $@ $< -std=c++17 $(o3_opt) $(lto_opt)
obj_all := $(obj) $(smhasher_obj) $(HLSLcc_obj) $(cbstring_obj) $(imgui_obj) $(minhook_obj) $(spirv_obj) $(glslang_obj) $(retroarch_obj)
dir_all := $(sort $(dir $(obj_all)))
dep_all := $(obj_all:%.o=%.d)
dll := dinput8.dll

ifeq ($(color),1)
	color_opt := -fdiagnostics-color=always
else
	color_opt :=
endif

ifeq ($(o3),0)
	o3_opt := -g
	lto := 0
else
	o3_opt := -O3 -DNDEBUG -s
endif

ifeq ($(lto),0)
	lto_opt :=
else
	lto_opt := -flto
endif

$(dll): $(obj_all) dinput8.def
	g++ $(color_opt) -o $@ $+ $(o3_opt) $(lto_opt) -shared -static -Werror -Wno-odr -Wno-lto-type-mismatch -Wl,--enable-stdcall-fixup -ld3dcompiler_47 -luuid -lmsimg32 -lhid -lsetupapi -lgdi32 -lcomdlg32 -ldinput8 -lole32 -ldxguid

obj/%.o: src/%.cpp | $(dir)
	$(cxx) -Werror -Wall $(retroarch_flg) -IRetroArch/RetroArch/gfx/common

obj/smhasher/%.o: smhasher/src/%.cpp | $(smhasher_dir)
	$(cxx)

obj/HLSLcc/%.o: HLSLcc/src/%.cpp | $(HLSLcc_dir)
	$(cxx) -IHLSLcc -IHLSLcc/include -IHLSLcc/src/internal_includes -IHLSLcc/src/cbstring -IHLSLcc/src -Wno-deprecated-declarations

obj/cbstring/%.o: HLSLcc/src/cbstring/%.c | $(cbstring_dir)
	gcc $(color_opt) -c -MMD -MP -o $@ $< $(o3_opt) $(lto_opt) -IHLSLcc/src/cbstring

obj/imgui/%.o: imgui/%.cpp | $(imgui_dir)
	$(cxx) -Iimgui

obj/minhook/%.o: minhook/src/%.c | $(minhook_dir)
	gcc $(color_opt) -c -MMD -MP -o $@ $< -std=c11 -masm=intel $(o3_opt)

obj/SPIRV-Cross/%.o: SPIRV-Cross/%.cpp | $(spirv_dir)
	$(cxx)

obj/glslang/%.o: glslang/glslang/%.cpp | $(glslang_dir)
	$(cxx) -DENABLE_HLSL

obj/RetroArch/gfx/common/d3d10_common.o : RetroArch/gfx/common/d3d10_common.c | $(retroarch_dir)
	$(retroarch_cc) -IRetroArch/RetroArch/gfx/common

obj/RetroArch/%.o: RetroArch/%.c | $(retroarch_dir)
	$(retroarch_cc) -IRetroArch/RetroArch/gfx/common -IRetroArch/RetroArch/gfx/drivers -DHAVE_DYNAMIC

obj/RetroArch/%.o: RetroArch/RetroArch/%.c | $(retroarch_dir)
	$(retroarch_cc) -DHAVE_DYNAMIC

obj/RetroArch/%.o: RetroArch/RetroArch/%.cpp | $(retroarch_dir)
	$(retroarch_cxx)

obj/RetroArch/deps/glslang/glslang.o: glslang/glslang.cpp | $(retroarch_dir)
	$(retroarch_cxx) -IRetroArch/RetroArch/deps/glslang

$(dir_all):
	@mkdir -p $@

.PHONY: clean

clean:
	-find obj/ -type f -name '*.o' -delete
	-find obj/ -type f -name '*.d' -delete
	-find obj/ -type d -empty -delete
	-$(RM) *.dll

-include $(dep_all)
