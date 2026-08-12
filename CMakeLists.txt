cmake_minimum_required(VERSION 3.5)

project(re-plants-vs-zombies LANGUAGES C CXX ASM)

file(GLOB SOURCES
	*.cpp
	SexyAppFramework/*.cpp
	SexyAppFramework/graphics/*.cpp
	SexyAppFramework/imagelib/*.cpp
	SexyAppFramework/misc/*.cpp
	SexyAppFramework/paklib/*.cpp
	SexyAppFramework/sound/*.cpp
	SexyAppFramework/widget/*.cpp
	#SexyAppFramework/ogg/*.c
	SexyAppFramework/fcaseopen/*.c
	Sexy.TodLib/*.cpp
	Lawn/*.cpp
	Lawn/Widget/*.cpp
	Lawn/System/*.cpp
	SexyAppFramework/LawnProject.rc
)

# these files are included and must not be built in the project
list(REMOVE_ITEM SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/Sexy.TodLib/TodDrawTriangle.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Sexy.TodLib/TodDrawTriangleInc.cpp
)

set(NO_GLEW OFF)
set(LOW_MEMORY OFF)
set(DOWNSCALE_COUNT 1)
if(NINTENDO_WII)
	# Wii's malloc heap is ~53 MB. Full reanimation preloading consumes about
	# 50 MB before the title screen, so definitions must load on demand.
	set(NO_GLEW ON)
	set(LOW_MEMORY ON)
	set(DOWNSCALE_COUNT 1)
	list(REMOVE_ITEM SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/sound/SDLSoundManager.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/sound/SDLSoundInstance.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/sound/SDLMusicInterface.cpp
	)
	file(GLOB WII_OPENGX_SOURCES CONFIGURE_DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/wii/opengx/src/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/wii/opengx/src/*.cpp
	)
	list(APPEND SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/wii/pvz_wii_main.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/wii/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/wii/Input.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/pc/graphics/GLInterface.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/wii/WiiEarlyInit.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/wii/WiiHeap.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/wii/WiiCursor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/wii/gx_wii.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/wii/sound/WiiSoundManager.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/wii/WiiOnScreenKeyboard.cpp
		${WII_OPENGX_SOURCES}
	)
	list(APPEND PLAT_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/wii
		# Reuse platform-neutral GLInterface declarations for now; Wii supplies
		# the GL implementation through OpenGX and gets its own .cpp backend.
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/pc
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/wii/opengx/include
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/wii/opengx/src
	)
elseif(PS2)
	set(NO_GLEW ON)
	set(LOW_MEMORY ON)
	# Must stay 1: IMG_DOWNSCALE>1 halves *logical* image dims, but the game
	# has no global coordinate scaling — reanim transforms and hardcoded
	# 800x600 positions stay full-res, so everything renders scattered at
	# half size. VRAM/RAM savings come from the texture-level downscale in
	# gles_ps2.cpp instead (PS2_TEX_SHRINK_SHIFT), which is invisible to
	# game-side layout.
	set(DOWNSCALE_COUNT 1)
	list(REMOVE_ITEM SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/sound/SDLSoundManager.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/sound/SDLSoundInstance.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/sound/SDLMusicInterface.cpp
	)
	include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ps2_embed_irx.cmake)
	set(PS2_IRX_BLOBS ${CMAKE_CURRENT_BINARY_DIR}/ps2_irx_blobs.c)
	ps2_embed_irx(${PS2_IRX_BLOBS}
		$ENV{PS2SDK}/iop/irx/usbd.irx
		$ENV{PS2SDK}/iop/irx/usbhdfsd.irx
		# USB HID mouse driver: a usbd client like the mass-storage drivers,
		# polled over SIF RPC from Input.cpp (libmouse EE client).
		$ENV{PS2SDK}/iop/irx/ps2mouse.irx
		$ENV{PS2SDK}/iop/irx/audsrv.irx
		# Modern BDM USB stack (real hardware): iomanX+fileXio route newlib IO
		# to iomanX devices; bdm+bdmfs_fatfs+usbmass_bd replace the legacy
		# usbhdfsd, which wedges under sustained load on real consoles.
		$ENV{PS2SDK}/iop/irx/iomanX.irx
		$ENV{PS2SDK}/iop/irx/fileXio.irx
		$ENV{PS2SDK}/iop/irx/bdm.irx
		$ENV{PS2SDK}/iop/irx/bdmfs_fatfs.irx
		$ENV{PS2SDK}/iop/irx/usbmass_bd.irx
		# CD/DVD (or mounted ISO) boot: cdfs exposes the disc as "cdfs:/..."
		# (case-insensitive, forward slashes) so assets read cleanly, unlike the
		# raw cdrom0: paths that mangle subdirs and need uppercase + ";1".
		$ENV{PS2SDK}/iop/irx/cdfs.irx
	)
	list(APPEND SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/main_pvz_ps2.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/gles_ps2.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/Ps2TextureConversion.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/Ps2VramAllocator.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/Ps2PadState.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/Ps2IoLock.c
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/Ps2Trace.c
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/Ps2AssetIndex.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/graphics/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/graphics/GLInterface.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/Input.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/Ps2PvzServices.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/sound/Ps2SoundManager.cpp
		${PS2_IRX_BLOBS}
	)
	list(APPEND PLAT_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2
	)
elseif(NINTENDO_SWITCH)
	set(NO_GLEW ON)
	set(VORBIS_LIBS vorbisfile vorbis)
	list(APPEND SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/switch/graphics/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/switch/graphics/GLInterface.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/switch/Input.cpp
	)
	list(APPEND PLAT_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/switch
	)
elseif(NINTENDO_3DS)
	set(NO_GLEW ON)
	set(LOW_MEMORY ON)
	set(DOWNSCALE_COUNT 2)
	ctr_add_shader_library(colored ${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/3ds/graphics/colored.v.pica)
	ctr_add_shader_library(textured ${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/3ds/graphics/textured.v.pica)
	dkp_add_embedded_binary_library(shaders colored textured)
	list(APPEND SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/3ds/graphics/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/3ds/graphics/GLInterface.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/3ds/Input.cpp
	)
	list(APPEND PLAT_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/3ds
	)
else()
	set(VORBIS_LIBS vorbisfile vorbis)
	list(APPEND SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/pc/graphics/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/pc/graphics/GLInterface.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/pc/Input.cpp
	)
	list(APPEND PLAT_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/pc
	)
endif()

option(CONSOLE "Show console on Windows" ON)

find_package(ZLIB REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
if (NOT PS2 AND NOT NINTENDO_WII)
	find_package(SDL2 REQUIRED)
	if (NOT NO_GLEW)
		find_package(GLEW REQUIRED)
	endif()
endif()

if (NOT PS2 AND NOT NINTENDO_WII)
	set(SDL_MIXER_X_STATIC ON CACHE BOOL "Build static library of SDL Mixer X" FORCE)
	set(SDL_MIXER_X_SHARED OFF CACHE BOOL "Build shared library of SDL Mixer X" FORCE)
	set(USE_MIDI OFF CACHE BOOL "SDL Mixer X midi" FORCE)
	set(USE_MODPLUG OFF CACHE BOOL "SDL Mixer X modplug" FORCE)
	set(USE_XMP OFF CACHE BOOL "SDL Mixer X xmp" FORCE)
	set(USE_OPUS OFF CACHE BOOL "SDL Mixer X opus" FORCE)
	set(USE_DRFLAC OFF CACHE BOOL "SDL Mixer X flac" FORCE)
	set(USE_PXTONE OFF CACHE BOOL "SDL Mixer X pxtone" FORCE)
	add_subdirectory(SexyAppFramework/sound/SDL-Mixer-X)
endif()

add_executable(re-plants-vs-zombies ${SOURCES})
target_include_directories(re-plants-vs-zombies PRIVATE
	${PROJECT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}/SexyAppFramework
	${PROJECT_SOURCE_DIR}/SexyAppFramework/sound/SDL-Mixer-X/include
	${PLAT_INCLUDES}
)
if (NOT PS2 AND NOT NINTENDO_WII)
	target_include_directories(re-plants-vs-zombies PRIVATE ${SDL2_INCLUDE_DIRS})
endif()
target_compile_definitions(re-plants-vs-zombies PRIVATE _DEBUG)
if (PS2)
	target_compile_definitions(re-plants-vs-zombies PRIVATE PS2_PLATFORM)
	target_compile_options(re-plants-vs-zombies PRIVATE -include SDL.h)

	# Every printf on PS2 is a SIF fio RPC to the IOP TTY. On real hardware
	# that traffic can wedge the console, so printf/puts/putchar are wrapped
	# (TodDebug.cpp) and appended to <save prefix>/userdata/log.txt instead,
	# like the original game's log.
	# __cxa_throw is wrapped too (main_pvz_ps2.cpp): it logs every C++ throw
	# (exception type + throw-site address) before unwinding starts, so a
	# throw that never reaches a catch still leaves its name in log.txt.
	# operator new/new[] (_Znwj/_Znaj, ilp32 mangling) are wrapped as well
	# (main_pvz_ps2.cpp): a failing plain `new` logs its size and call site
	# and retries after shedding purgeable image bits, instead of taking the
	# whole game down from inside a loading screen. The new wrap also routes
	# allocations >= PS2_BIG_ALLOC_THRESHOLD into the reserved big-block
	# arena (Ps2BigArena.h) so they never fight the general heap's
	# fragmentation; free is wrapped so arena pointers (returned by plain
	# free() or by operator delete, which funnels into free) go back to the
	# arena instead of corrupting newlib's heap.
	target_link_options(re-plants-vs-zombies PRIVATE
		-Wl,--wrap=printf -Wl,--wrap=puts -Wl,--wrap=putchar
		-Wl,--wrap=__cxa_throw
		-Wl,--wrap=_Znwj -Wl,--wrap=_Znaj
		-Wl,--wrap=free)
endif()

if (NINTENDO_WII)
	target_compile_definitions(re-plants-vs-zombies PRIVATE
		WII_PLATFORM
		NINTENDO_WII
		WII_TEXTURE_RGB5A3=0
	)
	target_compile_options(re-plants-vs-zombies PRIVATE
		-include SDL.h
		-mcpu=750
		-meabi
		-mhard-float
		-ffunction-sections
		-fdata-sections
	)
endif()

if (LOW_MEMORY)
	target_compile_definitions(re-plants-vs-zombies PRIVATE LOW_MEMORY)
endif()
target_compile_definitions(re-plants-vs-zombies PRIVATE IMG_DOWNSCALE=${DOWNSCALE_COUNT})

if (NINTENDO_WII)
	target_link_libraries(re-plants-vs-zombies
		jpeg png z
		asnd fat wiiuse bte ogc m
	)
elseif (PS2)
	target_link_libraries(re-plants-vs-zombies
		jpeg png z
		gskit gskit_toolkit dmakit
		pad mouse graph dma vux kernel
		audsrv patches mc
		fileXio cdvd
		m
	)
else()
	target_link_libraries(re-plants-vs-zombies jpeg png z SDL2_mixer_ext_Static openmpt mpg123 ${VORBIS_LIBS} ogg ${SDL2_LIBRARIES} ${GLEW_LIBRARIES})
endif()

if (NINTENDO_WII)
	set_target_properties(re-plants-vs-zombies PROPERTIES
		OUTPUT_NAME "pvz_wii"
		SUFFIX ".elf"
	)
	target_link_options(re-plants-vs-zombies PRIVATE
		-mcpu=750
		-meabi
		-mhard-float
		-Wl,--gc-sections
		# Route printf into the log, the same way PS2 does. On Wii stdout is the
		# libogc text console, which stops being read the moment GX adopts the
		# framebuffer -- so an unwrapped printf is a diagnostic written into a
		# void. That is how five separate music failure messages went unseen.
		# The wrappers are in misc/SexyLog.cpp, shared with PS2.
		-Wl,--wrap=printf -Wl,--wrap=puts -Wl,--wrap=putchar
	)
	find_program(ELF2DOL elf2dol HINTS "$ENV{DEVKITPRO}/tools/bin")
	if(ELF2DOL)
		add_custom_command(TARGET re-plants-vs-zombies POST_BUILD
			COMMAND ${ELF2DOL}
				$<TARGET_FILE:re-plants-vs-zombies>
				$<TARGET_FILE_DIR:re-plants-vs-zombies>/boot.dol
			COMMENT "Creating Wii boot.dol"
		)
	endif()
elseif (WIN32)
	target_link_options(re-plants-vs-zombies PRIVATE -static -static-libgcc -static-libstdc++)
	target_link_libraries(re-plants-vs-zombies opengl32 ws2_32 ddraw dinput8 dxguid dxerr8 user32 gdi32 winmm imm32 shlwapi)
	target_compile_definitions(re-plants-vs-zombies PRIVATE WINDOWS)

	if(NOT CONSOLE)
		target_link_options(re-plants-vs-zombies PRIVATE -mwindows)
	endif()
elseif (PS2)
	set_target_properties(re-plants-vs-zombies PROPERTIES OUTPUT_NAME "pvz_ps2" SUFFIX ".elf")

	# The whole ELF stays resident in the 32 MB of EE RAM, so binary size is
	# gameplay memory: optimize for size, drop unreferenced code/data at link
	# time, and strip symbols from the final ELF.
	target_compile_options(re-plants-vs-zombies PRIVATE
		-O2
		-ffunction-sections
		-fdata-sections
	)
	target_link_options(re-plants-vs-zombies PRIVATE -Wl,--gc-sections)
	find_program(PS2_STRIP mips64r5900el-ps2-elf-strip)
	if (PS2_STRIP)
		# Keep an unstripped copy next to the deployable ELF: crash addresses
		# from PCSX2 ("TLB Miss, pc=0x...") or the [THROW] ra= log line resolve
		# to function+line with
		#   mips64r5900el-ps2-elf-addr2line -f -C -e pvz_ps2_syms.elf 0x<pc>
		add_custom_command(TARGET re-plants-vs-zombies POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:re-plants-vs-zombies> $<TARGET_FILE_DIR:re-plants-vs-zombies>/pvz_ps2_syms.elf
			COMMAND ${PS2_STRIP} --strip-all $<TARGET_FILE:re-plants-vs-zombies>
			COMMENT "Saving pvz_ps2_syms.elf + stripping pvz_ps2.elf"
		)
	endif()
elseif (NINTENDO_SWITCH)
	target_link_libraries(re-plants-vs-zombies glad EGL glapi drm_nouveau)

	nx_generate_nacp(re-plants-vs-zombies.nacp
		NAME "Plants vs. Zombies"
		AUTHOR "PopCap Games, Unofficial Switch port by Headshotnoby"
		VERSION "0.1"
	)

	nx_create_nro(re-plants-vs-zombies
		NACP re-plants-vs-zombies.nacp
		ICON ${CMAKE_CURRENT_SOURCE_DIR}/icon-switch.jpg
	)
elseif (NINTENDO_3DS)
	target_link_libraries(re-plants-vs-zombies citro3d shaders)

	ctr_generate_smdh(re-plants-vs-zombies.smdh
		NAME "Plants vs. Zombies"
		DESCRIPTION "GOTY Edition"
		AUTHOR "PopCap Games, Unofficial 3DS port by Headshotnoby"
		VERSION "0.1"
		ICON "${CMAKE_CURRENT_SOURCE_DIR}/icon-3ds.png"
	)

	ctr_create_3dsx(re-plants-vs-zombies
		SMDH re-plants-vs-zombies.smdh
	)
else()
	target_link_libraries(re-plants-vs-zombies GL)
endif()
