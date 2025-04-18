cmake_minimum_required(VERSION 3.7)
include(${CMAKE_CURRENT_SOURCE_DIR}/Misc/cmake/ExtractVersion.cmake)
project(ironwail VERSION ${IRONWAIL_VERSION})

cmake_policy(SET CMP0072 NEW)   #OpenGL_GL_PREFERENCE to GLVND

if (WIN32)
	list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Windows/cmake-modules)
endif()

# Gyro Space and Play header to handle additional gyro_turning_axis modes
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/src/client/input/header/GyroSpace")
    execute_process(
        COMMAND git submodule update --init --recursive
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()	

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(CURL)

find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
    pkg_check_modules(PC_MPG123 IMPORTED_TARGET libmpg123)
    pkg_check_modules(PC_MAD IMPORTED_TARGET mad)
    pkg_check_modules(PC_OPUSFILE IMPORTED_TARGET opusfile)
    pkg_check_modules(PC_FLAC IMPORTED_TARGET flac)
    pkg_check_modules(PC_VORBISFILE IMPORTED_TARGET vorbisfile)
    pkg_check_modules(PC_TREMOR IMPORTED_TARGET vorbisidec)
    pkg_check_modules(PC_MIKMOD IMPORTED_TARGET libmikmod)
    pkg_check_modules(PC_MODPLUG IMPORTED_TARGET libmodplug)
    pkg_check_modules(PC_XMP IMPORTED_TARGET libxmp)
endif()

if (WIN32)
	find_package(BundledCODECS)
endif()

file(GLOB IWAIL_SRC CONFIGURE_DEPENDS "Quake/*.c" "Quake/*.h")

list(REMOVE_ITEM IWAIL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Quake/lodepng.h)
list(REMOVE_ITEM IWAIL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Quake/lodepng.c)

if (UNIX)
    list(REMOVE_ITEM IWAIL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Quake/pl_win.c)
    list(REMOVE_ITEM IWAIL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Quake/net_wipx.c)
    list(REMOVE_ITEM IWAIL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Quake/net_wipx.h)
    list(REMOVE_ITEM IWAIL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Quake/net_wins.c)
    list(REMOVE_ITEM IWAIL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Quake/net_wins.h)
    list(REMOVE_ITEM IWAIL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Quake/net_win.c)
    list(REMOVE_ITEM IWAIL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Quake/net_win.h)
    list(REMOVE_ITEM IWAIL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Quake/sys_sdl_win.c)
elseif (WIN32)
    list(REMOVE_ITEM IWAIL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Quake/pl_linux.c)
    list(REMOVE_ITEM IWAIL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Quake/net_bsd.c)
    list(REMOVE_ITEM IWAIL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Quake/net_bsd.h)
    list(REMOVE_ITEM IWAIL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Quake/net_udp.c)
    list(REMOVE_ITEM IWAIL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Quake/net_udp.h)
    list(REMOVE_ITEM IWAIL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Quake/sys_sdl_unix.c)
    list(APPEND IWAIL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Windows/QuakeSpasm.rc)
endif()

if (PC_MPG123_FOUND)
    list(REMOVE_ITEM IWAIL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Quake/snd_mp3.c)
elseif (PC_MAD_FOUND)
    list(REMOVE_ITEM IWAIL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Quake/snd_mpg123.c)
endif()

add_executable(ironwail ${IWAIL_SRC})
if (UNIX)
	target_link_libraries(ironwail PRIVATE m)
	set(ENABLE_USERDIRS TRUE CACHE STRING "Enable user directories")
elseif (WIN32)
	target_link_libraries(ironwail PRIVATE ws2_32 Winmm)
	if (MINGW)
		#When the GNU linker is used together with SDL2_Main,
		#it is very particular about the library ordering
		#on the link command line
		target_link_libraries(ironwail PRIVATE mingw32)
		if (SDL2_MAIN_LIBRARY)
			target_link_libraries(ironwail PRIVATE SDL2::main)
		endif()
	endif()
endif()
target_link_libraries(ironwail PRIVATE SDL2::SDL2)

if (CURL_FOUND)
	target_link_libraries(ironwail PRIVATE CURL::libcurl)
else()
	target_compile_definitions(ironwail PRIVATE WITHOUT_CURL)
endif()

if (OpenGL::OpenGL)
	target_link_libraries(ironwail PRIVATE OpenGL::OpenGL)
else()
	target_link_libraries(ironwail PRIVATE OpenGL::GL)
endif()

target_compile_definitions(ironwail PRIVATE $<$<CONFIG:Debug>:DEBUG=1> $<$<NOT:$<CONFIG:Debug>>:NDEBUG=1> USE_SDL2 USE_CODEC_WAVE)
if (MSVC)
	target_compile_definitions(ironwail PRIVATE _CRT_SECURE_NO_WARNINGS _WIN32)
endif()

if (ENABLE_USERDIRS)
	target_compile_definitions(ironwail PRIVATE DO_USERDIRS=1)
endif()

if (PC_MPG123_FOUND)
	target_compile_definitions(ironwail PRIVATE USE_CODEC_MP3)
    target_link_libraries(ironwail PUBLIC PkgConfig::PC_MPG123)
elseif (PC_MAD_FOUND)
	target_compile_definitions(ironwail PRIVATE USE_CODEC_MP3)
	target_link_libraries(ironwail PUBLIC PkgConfig::PC_MAD)
endif()

if (PC_OPUSFILE_FOUND)
	target_compile_definitions(ironwail PRIVATE USE_CODEC_OPUS)
    target_link_libraries(ironwail PUBLIC PkgConfig::PC_OPUSFILE)
endif()

if (PC_FLAC_FOUND)
	target_compile_definitions(ironwail PRIVATE USE_CODEC_FLAC)
    target_link_libraries(ironwail PUBLIC PkgConfig::PC_FLAC)
endif()

if (PC_VORBISFILE_FOUND)
	target_compile_definitions(ironwail PRIVATE USE_CODEC_VORBIS)
    target_link_libraries(ironwail PUBLIC PkgConfig::PC_VORBISFILE)
elseif (PC_TREMOR_FOUND)
	target_compile_definitions(ironwail PRIVATE USE_CODEC_VORBIS VORBIS_USE_TREMOR)
    target_link_libraries(ironwail PUBLIC PkgConfig::PC_TREMOR)
endif()

if (PC_MIKMOD_FOUND)
	target_compile_definitions(ironwail PRIVATE USE_CODEC_MIKMOD)
    target_link_libraries(ironwail PUBLIC PkgConfig::PC_MIKMOD)
endif()

if (PC_MODPLUG_FOUND)
	target_compile_definitions(ironwail PRIVATE USE_CODEC_MODPLUG)
    target_link_libraries(ironwail PUBLIC PkgConfig::PC_MODPLUG)
endif()

if (PC_XMP_FOUND)
	target_compile_definitions(ironwail PRIVATE USE_CODEC_XMP)
    target_link_libraries(ironwail PUBLIC PkgConfig::PC_XMP)
endif()

if (PC_MIKMOD_FOUND OR PC_MODPLUG_FOUND OR PC_XMP_FOUND)
    target_compile_definitions(ironwail PRIVATE USE_CODEC_UMX)
endif()

if (WIN32)
	#Copy DLL dependencies to output folder
	add_custom_command(TARGET ironwail POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:ironwail> $<TARGET_FILE_DIR:ironwail>
		COMMAND_EXPAND_LISTS)

	#VORBISFILE and OPUSFILE both use the libogg-0.dll
	if (OGG_DLL AND (PC_VORBISFILE_FOUND OR PC_OPUSFILE_FOUND))
		add_custom_command(TARGET ironwail POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${OGG_DLL} $<TARGET_FILE_DIR:ironwail>
			COMMAND_EXPAND_LISTS)
	endif()
	#libvorbisfile depends on libvorbis-0.dll
	if (PC_VORBISFILE_FOUND AND VORBIS_DLL)
		add_custom_command(TARGET ironwail POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VORBIS_DLL} $<TARGET_FILE_DIR:ironwail>
			COMMAND_EXPAND_LISTS)
	endif()
	#libopusfile depends on libopus-0.dll
	if (PC_OPUSFILE_FOUND AND OPUS_DLL)
		add_custom_command(TARGET ironwail POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${OPUS_DLL} $<TARGET_FILE_DIR:ironwail>
			COMMAND_EXPAND_LISTS)
	endif()
	#CURL wants zlib1.dll
	if (CURL_FOUND AND ZLIB1_DLL)
		add_custom_command(TARGET ironwail POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ZLIB1_DLL} $<TARGET_FILE_DIR:ironwail>
				COMMAND_EXPAND_LISTS)
	endif()
endif()

