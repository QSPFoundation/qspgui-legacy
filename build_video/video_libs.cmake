# Third-party libraries for the built-in WebM decoder:
#   nestegg (WebM demuxer), libvpx (VP8/VP9) and libopus (Opus).
# Vorbis is decoded by stb_vorbis, which the sound engine already contains.
#
# Everything is compiled as portable C without assembly, so the same rules work
# for MinGW cross builds, MSVC, universal macOS binaries and Linux.

set(VIDEO_LIBVPX_TAG v1.17.0)
set(VIDEO_OPUS_VERSION 1.6.1)
set(VIDEO_OPUS_SHA256 6ffcb593207be92584df15b32466ed64bbec99109f007c82205f0194572411a1)
set(VIDEO_NESTEGG_COMMIT 767aab25013acefbdcc6d68a2a7a2c9081303e3f)

FetchContent_Declare(video-libvpx
	GIT_REPOSITORY https://github.com/webmproject/libvpx.git
	GIT_TAG ${VIDEO_LIBVPX_TAG}
	GIT_SHALLOW true
	GIT_PROGRESS true
	SUBBUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/libvpx-download"
	SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/libvpx-src"
	SOURCE_SUBDIR no-cmake-build # only fetch, the library is built below
)
FetchContent_Declare(video-nestegg
	GIT_REPOSITORY https://github.com/kinetiknz/nestegg.git
	GIT_TAG ${VIDEO_NESTEGG_COMMIT}
	GIT_PROGRESS true
	SUBBUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/nestegg-download"
	SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/nestegg-src"
	SOURCE_SUBDIR no-cmake-build # only fetch, the library is built below
)
if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
	set(VIDEO_OPUS_TIMESTAMP DOWNLOAD_EXTRACT_TIMESTAMP true)
else()
	set(VIDEO_OPUS_TIMESTAMP)
endif()
FetchContent_Declare(video-opus
	URL https://downloads.xiph.org/releases/opus/opus-${VIDEO_OPUS_VERSION}.tar.gz
	URL_HASH SHA256=${VIDEO_OPUS_SHA256}
	${VIDEO_OPUS_TIMESTAMP}
	SUBBUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/opus-download"
	SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/opus-src"
	BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/opus-build"
)
FetchContent_MakeAvailable(video-libvpx video-nestegg)

if ("${CMAKE_C_COMPILER_ID}" MATCHES "GNU|Clang")
	set(VIDEO_LIBS_C_OPTIONS -fno-fast-math -w)
else()
	set(VIDEO_LIBS_C_OPTIONS)
endif()

# Opus

set(OPUS_BUILD_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(OPUS_INSTALL_PKG_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
set(OPUS_INSTALL_CMAKE_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
if (MSVC)
	# Opus switches itself to the DLL runtime unless told otherwise, which breaks linking with /MT
	if (DEFINED CMAKE_MSVC_RUNTIME_LIBRARY AND NOT CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "DLL")
		set(OPUS_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
	else()
		set(OPUS_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)
	endif()
endif()
list(LENGTH CMAKE_OSX_ARCHITECTURES VIDEO_OSX_ARCH_COUNT)
if (VIDEO_OSX_ARCH_COUNT GREATER 1)
	set(OPUS_DISABLE_INTRINSICS ON CACHE BOOL "" FORCE)
endif()
FetchContent_MakeAvailable(video-opus)
target_compile_options(opus PRIVATE ${VIDEO_LIBS_C_OPTIONS})

# nestegg

add_library(qsp_nestegg STATIC "${video-nestegg_SOURCE_DIR}/src/nestegg.c")
target_include_directories(qsp_nestegg PUBLIC "${video-nestegg_SOURCE_DIR}/include")
target_compile_options(qsp_nestegg PRIVATE ${VIDEO_LIBS_C_OPTIONS})

# libvpx (decoders only, generic C)

set(VIDEO_LIBVPX_SOURCES
	vp8/common/alloccommon.c vp8/common/blockd.c vp8/common/dequantize.c vp8/common/entropy.c
	vp8/common/entropymode.c vp8/common/entropymv.c vp8/common/extend.c vp8/common/filter.c
	vp8/common/findnearmv.c vp8/common/generic/systemdependent.c vp8/common/idct_blk.c
	vp8/common/idctllm.c vp8/common/loopfilter_filters.c vp8/common/mbpitch.c vp8/common/modecont.c
	vp8/common/quant_common.c vp8/common/reconinter.c vp8/common/reconintra.c
	vp8/common/reconintra4x4.c vp8/common/rtcd.c vp8/common/setupintrarecon.c
	vp8/common/swapyv12buffer.c vp8/common/treecoder.c vp8/common/vp8_loopfilter.c
	vp8/decoder/dboolhuff.c vp8/decoder/decodeframe.c vp8/decoder/decodemv.c
	vp8/decoder/detokenize.c vp8/decoder/onyxd_if.c vp8/decoder/threading.c vp8/vp8_dx_iface.c
	vp9/common/vp9_alloccommon.c vp9/common/vp9_blockd.c vp9/common/vp9_common_data.c
	vp9/common/vp9_entropy.c vp9/common/vp9_entropymode.c vp9/common/vp9_entropymv.c
	vp9/common/vp9_filter.c vp9/common/vp9_frame_buffers.c vp9/common/vp9_idct.c
	vp9/common/vp9_loopfilter.c vp9/common/vp9_mvref_common.c vp9/common/vp9_pred_common.c
	vp9/common/vp9_quant_common.c vp9/common/vp9_reconinter.c vp9/common/vp9_reconintra.c
	vp9/common/vp9_rtcd.c vp9/common/vp9_scale.c vp9/common/vp9_scan.c vp9/common/vp9_seg_common.c
	vp9/common/vp9_thread_common.c vp9/common/vp9_tile_common.c vp9/decoder/vp9_decodeframe.c
	vp9/decoder/vp9_decodemv.c vp9/decoder/vp9_decoder.c vp9/decoder/vp9_detokenize.c
	vp9/decoder/vp9_dsubexp.c vp9/decoder/vp9_job_queue.c vp9/vp9_dx_iface.c vp9/vp9_iface_common.c
	vpx/src/vpx_codec.c vpx/src/vpx_decoder.c vpx/src/vpx_encoder.c vpx/src/vpx_image.c
	vpx_dsp/bitreader.c vpx_dsp/bitreader_buffer.c vpx_dsp/intrapred.c vpx_dsp/inv_txfm.c
	vpx_dsp/loopfilter.c vpx_dsp/prob.c vpx_dsp/skin_detection.c vpx_dsp/vpx_convolve.c
	vpx_dsp/vpx_dsp_rtcd.c vpx_mem/vpx_mem.c vpx_scale/generic/gen_scalers.c
	vpx_scale/generic/vpx_scale.c vpx_scale/generic/yv12config.c vpx_scale/generic/yv12extend.c
	vpx_scale/vpx_scale_rtcd.c vpx_util/vpx_thread.c vpx_util/vpx_write_yuv_frame.c
)
list(TRANSFORM VIDEO_LIBVPX_SOURCES PREPEND "${video-libvpx_SOURCE_DIR}/")

include(CheckIncludeFile)
include(CheckSymbolExists)
if (WIN32)
	# libvpx uses native Win32 threads when pthread.h is absent; keep it that way for MinGW too
	set(HAVE_PTHREAD_H OFF)
	set(HAVE_PTHREAD_SETNAME_NP OFF)
else()
	check_include_file(pthread.h HAVE_PTHREAD_H)
	set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
	set(CMAKE_REQUIRED_LIBRARIES pthread)
	# macOS has a one-argument pthread_setname_np that libvpx can't use
	if (NOT APPLE)
		check_symbol_exists(pthread_setname_np pthread.h HAVE_PTHREAD_SETNAME_NP)
	endif()
	unset(CMAKE_REQUIRED_DEFINITIONS)
	unset(CMAKE_REQUIRED_LIBRARIES)
endif()
check_include_file(unistd.h HAVE_UNISTD_H)
if (MSVC)
	set(CONFIG_GCC OFF)
	set(CONFIG_MSVS ON)
else()
	set(CONFIG_GCC ON)
	set(CONFIG_MSVS OFF)
endif()
set(VIDEO_LIBVPX_CONFIG_DIR "${CMAKE_CURRENT_BINARY_DIR}/libvpx-config")
configure_file("${CMAKE_CURRENT_LIST_DIR}/libvpx/vpx_config.h.in" "${VIDEO_LIBVPX_CONFIG_DIR}/vpx_config.h")

add_library(qsp_vpx STATIC ${VIDEO_LIBVPX_SOURCES} "${CMAKE_CURRENT_LIST_DIR}/libvpx/vpx_config.c")
target_include_directories(qsp_vpx
	PRIVATE "${VIDEO_LIBVPX_CONFIG_DIR}" "${CMAKE_CURRENT_LIST_DIR}/libvpx"
	PUBLIC "${video-libvpx_SOURCE_DIR}")
target_compile_options(qsp_vpx PRIVATE ${VIDEO_LIBS_C_OPTIONS})
if (WIN32)
	# The Win32 threading layer of libvpx needs SRW locks (Vista+) and forgets to include errno.h
	target_compile_definitions(qsp_vpx PRIVATE _WIN32_WINNT=0x0600)
	if (MSVC)
		target_compile_options(qsp_vpx PRIVATE /FIerrno.h)
	else()
		target_compile_options(qsp_vpx PRIVATE -include errno.h)
	endif()
endif()
if (HAVE_PTHREAD_H)
	find_package(Threads REQUIRED)
	target_link_libraries(qsp_vpx PUBLIC Threads::Threads)
endif()
if (HAVE_PTHREAD_SETNAME_NP)
	target_compile_definitions(qsp_vpx PRIVATE _GNU_SOURCE)
endif()
