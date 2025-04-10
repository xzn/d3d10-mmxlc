ifeq ($(OS),Windows_NT)
	cross_prefix :=
else
	cross_prefix := i686-w64-mingw32-
endif
cc := $(cross_prefix)gcc
cxx := $(cross_prefix)g++
smhasher_src := smhasher/MurmurHash3.cpp
smhasher_obj := $(smhasher_src:smhasher/%.cpp=obj/smhasher/%.o)
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
retroarch_fun = $(shell make -sf RetroArch/RetroArch/Makefile.common $$'--eval=_print-var:\n\t@echo $$($1)' _print-var $(retroarch_def))
retroarch_def := GIT_VERSION= HAVE_LIBRETRODB=0 OS=Win32 HAVE_LANGEXTRA=1 HAVE_D3D10=1 HAVE_SLANG=1 HAVE_GLSLANG=1 HAVE_BUILTINZLIB=1 HAVE_RTGA=1 HAVE_RPNG=1 HAVE_RJPEG=1 HAVE_RBMP=1 HAVE_VIDEO_LAYOUT=1
retroarch_obj := $(call retroarch_fun,OBJ)
retroarch_obj := $(addprefix obj/RetroArch/,$(retroarch_obj))
retroarch_dir := $(sort $(dir $(retroarch_obj)))
retroarch_flg := -DRARCH_INTERNAL -DHAVE_MAIN $(call retroarch_fun,DEFINES) -DENABLE_HLSL -DHAVE_SPIRV_CROSS -IRetroArch/RetroArch/libretro-common/include -IRetroArch/RetroArch/libretro-common/include/compat/zlib -I. -isystem glslang -isystem glslang/glslang -isystem SPIRV-Cross -IRetroArch/RetroArch -IRetroArch/RetroArch/deps
retroarch_cc = $(cc) $(color_opt) -c -MMD -MP -o $@ $< $(dbg_opt) $(lto_opt) $(retroarch_flg) -Werror=implicit-function-declaration
retroarch_cxx = $(cxx) $(color_opt) -c -MMD -MP -o $@ $< -std=c++17 $(dbg_opt) $(lto_opt) -D__STDC_CONSTANT_MACROS $(retroarch_flg)
src := $(wildcard src/*.cpp)
obj := $(src:src/%.cpp=obj/%.o)
dir := $(sort $(dir $(obj)))
cxx_all = $(cxx) $(color_opt) -c -MMD -MP -o $@ $< -std=c++17 $(dbg_opt) $(lto_opt)
obj_all := $(obj) $(smhasher_obj) $(HLSLcc_obj) $(cbstring_obj) $(imgui_obj) $(minhook_obj) $(spirv_obj) $(glslang_obj) $(retroarch_obj)
dir_all := $(sort $(dir $(obj_all)))
dep_all := $(obj_all:%.o=%.d)
dll := dinput8.dll

ifeq ($(color),1)
	color_opt := -fdiagnostics-color=always
else
	color_opt :=
endif

ifeq ($(dbg),1)
	dbg_opt := -Og -g
	# dbg_opt += -g3 -ggdb -ggdb3
	# dbg_opt += -fno-inline -fno-early-inlining
	# dbg_opt += -fvar-tracking -gvariable-location-views -ginternal-reset-location-views -ginline-points -D_GLIBCXX_DEBUG
	# dbg_opt += -fno-merge-debug-strings -fno-eliminate-unused-debug-types -fno-eliminate-unused-debug-symbols
	lto := 0
	dll_dbg := $(dll:%.dll=%.dbg)
else
	dbg_opt := -O2 -DNDEBUG -s
endif

ifeq ($(lto),1)
	lto_opt := -flto
else
	lto_opt :=
endif

retroarch_ln := RetroArch/gfx/common/d3d10_common.c RetroArch/gfx/common/d3dcompiler_common.c RetroArch/gfx/drivers_display/gfx_display_d3d10.c RetroArch/gfx/drivers_font/d3d10_font.c
retroarch_base_ln := RetroArch/gfx/drivers/d3d10_base.c
retroarch_hdr := RetroArch/gfx/common/d3d10_common.h RetroArch/gfx/common/d3dcompiler_common.h
retroarch_hdr_src := $(retroarch_hdr:RetroArch/%.h=RetroArch/RetroArch/%.h)
retroarch_hdr_sen := obj/RetroArch/.retroarch_hdr_sen
retroarch_mod := RetroArch/gfx/drivers_shader/slang_reflection.cpp
retroarch_mod_src := $(retroarch_mod:RetroArch/%.cpp=RetroArch/RetroArch/%.cpp)
retroarch_mod_sen := obj/RetroArch/.retroarch_mod_sen

prep_src := $(glslang_ln) $(retroarch_ln) $(retroarch_hdr) $(retroarch_hdr_sen) $(retroarch_base_ln) $(retroarch_mod) $(retroarch_mod_sen)
prep: $(prep_src)
dll: $(dll) $(dll_dbg)

$(dll_dbg): $(dll)
	$(cross_prefix)objcopy --only-keep-debug $< $@

$(dll): $(obj_all) dinput8.def
	$(cxx) $(color_opt) -o $@ $+ $(dbg_opt) $(lto_opt) -shared -static -Werror -Wno-odr -Wno-lto-type-mismatch -Wl,--enable-stdcall-fixup -ld3dcompiler_47 -luuid -lmsimg32 -lhid -lsetupapi -lgdi32 -lcomdlg32 -ldinput8 -lole32 -ldxguid -lshlwapi -ld3d10

$(retroarch_ln): RetroArch/%: RetroArch/RetroArch/%
	ln -sr $< $@

$(retroarch_base_ln): RetroArch/%_base.c: RetroArch/RetroArch/%.c
	ln -sr $< $@

$(retroarch_hdr): $(retroarch_hdr_sen)
$(retroarch_hdr_sen): RetroArch/gfx/common/common.diff | $(retroarch_hdr_src) $(retroarch_dir)
	cp $(retroarch_hdr_src) $(<D)
	patch -d $(<D) -p 0 -i $(<F)
	touch $@

$(retroarch_mod): $(retroarch_mod_sen)
$(retroarch_mod_sen): RetroArch/gfx/drivers_shader/slang_reflection.diff | $(retroarch_mod_src) $(retroarch_dir)
	cp $(retroarch_mod_src) $(<D)
	patch -d $(<D) -p 0 -i $(<F)
	touch $@

obj/%.o: src/%.cpp | $(dir)
	$(cxx_all) -Werror -Wall $(retroarch_flg) -IRetroArch/RetroArch/gfx/common

obj/smhasher/%.o: smhasher/%.cpp | $(smhasher_dir)
	$(cxx_all)

obj/HLSLcc/%.o: HLSLcc/src/%.cpp | $(HLSLcc_dir)
	$(cxx_all) -IHLSLcc -IHLSLcc/include -IHLSLcc/src/internal_includes -IHLSLcc/src/cbstring -IHLSLcc/src -Wno-deprecated-declarations

obj/cbstring/%.o: HLSLcc/src/cbstring/%.c | $(cbstring_dir)
	$(cc) $(color_opt) -c -MMD -MP -o $@ $< $(dbg_opt) $(lto_opt) -IHLSLcc/src/cbstring

obj/imgui/%.o: imgui/%.cpp | $(imgui_dir)
	$(cxx_all) -Iimgui

obj/minhook/%.o: minhook/src/%.c | $(minhook_dir)
	$(cc) $(color_opt) -c -MMD -MP -o $@ $< -std=c11 -masm=intel $(dbg_opt)

obj/SPIRV-Cross/%.o: SPIRV-Cross/%.cpp | $(spirv_dir)
	$(cxx_all)

obj/glslang/%.o: glslang/glslang/%.cpp | $(glslang_dir)
	$(cxx_all) -DENABLE_HLSL -Iglslang/glslang

obj/RetroArch/gfx/common/d3d10_common.o : RetroArch/gfx/common/d3d10_common.c | $(retroarch_dir)
	$(retroarch_cc) -IRetroArch/RetroArch/gfx/common

obj/RetroArch/%.o: RetroArch/%.c | $(retroarch_dir)
	$(retroarch_cc) -IRetroArch/RetroArch/gfx/common -IRetroArch/RetroArch/gfx/drivers -DHAVE_DYNAMIC

obj/RetroArch/%.o: RetroArch/RetroArch/%.c | $(retroarch_dir)
	$(retroarch_cc) -DHAVE_DYNAMIC

obj/RetroArch/%.o: RetroArch/%.cpp | $(retroarch_dir)
	$(retroarch_cxx) -IRetroArch/RetroArch/gfx/drivers_shader

obj/RetroArch/%.o: RetroArch/RetroArch/%.cpp | $(retroarch_dir)
	$(retroarch_cxx)

obj/RetroArch/%.o: RetroArch/RetroArch/%.rc | $(retroarch_dir)
	$(cross_prefix)windres -o $@ $<

$(dir_all):
	@mkdir -p $@

.PHONY: prep dll clean retroarch_hdr

clean:
	-$(RM) *.dll *.dbg $(prep_src)
	-find obj/ -type f -name '*.o' -delete
	-find obj/ -type f -name '*.d' -delete
	-find obj/ -type d -empty -delete

-include $(dep_all)
