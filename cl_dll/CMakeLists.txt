#
# Copyright (c) 2016 Alibek Omarov
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

cmake_minimum_required(VERSION 3.9)
project (CLDLL)

# O mod usa "using namespace std" e tem o proprio typedef "byte": em C++17 isso conflita com std::byte
# ("reference to 'byte' is ambiguous"). Por isso fica em C++14 (o NDK novo usa C++17 por padrao).
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS ON)

set (CLDLL_LIBRARY client)

add_definitions(-DCLIENT_DLL)

if(NOT MSVC)
	# Absolute Zero usa try/catch (util.cpp) e o resto do mod nao foi escrito para -fno-exceptions/-fno-rtti.
	# Os simbolos NAO ficam ocultos (-fvisibility=hidden): o AZ declara EXPORT vazio fora do Windows.
	add_compile_options(-fsigned-char) # o codigo assume char COM sinal (x86/MSVC); no ARM o padrao e sem sinal
	add_compile_options(-fno-strict-aliasing)
	add_compile_options(-fms-extensions) # "extra qualification" etc. (aceito pelo MSVC)
	add_compile_options(-Wno-invalid-offsetof)
	add_compile_options(-w) # codigo de 2000-2005: silencia warnings para o log ficar legivel
	add_definitions(-D_LINUX -DLINUX -DPOSIX) # It seems enough for all non-Win32 systems
	add_definitions(-Dstricmp=strcasecmp -Dstrnicmp=strncasecmp)
	if(NOT MINGW)
		add_definitions(-D_snprintf=snprintf -D_vsnprintf=vsnprintf)
	endif()
	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-c++11-narrowing>)
		add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-address-of-temporary>) # (float*)&Vector(0,0,0)
		add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-non-pod-varargs>)
		add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-implicit-function-declaration>)
		add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-incompatible-pointer-types>)
		add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-int-conversion>)
	endif()
	# compat Win32 -> POSIX (itoa, min/max, __declspec, BOOLEAN, ...) forcada em todos os arquivos
	add_compile_options("SHELL:-include ${CMAKE_SOURCE_DIR}/public/az_compat.h")
	# tri.cpp/cl_util.cpp: caminho "port" (neblina via triapi do engine, FOV padrao 80, sem GL direto)
	add_definitions(-DAZ_ANDROID)
else()
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

if (GOLDSOURCE_SUPPORT)
	add_definitions(-DGOLDSOURCE_SUPPORT)
endif()

if (AZ_NO_VGUI)
	add_definitions(-DNO_VGUI)
elseif (USE_STATIC_FREEVGUI)
	add_definitions(-DINTERNAL_VGUI_SUPPORT)
endif()

set (CLDLL_SOURCES
	../common/vector.cpp
	../config/external_lib_include.cpp
	../dlls/weapon/chumtoadweapon.cpp
	../dlls/weapon/crossbow.cpp
	../dlls/weapon/crowbar.cpp
	../dlls/weapon/egon.cpp
	../dlls/weapon/gauss.cpp
	../dlls/weapon/glock.cpp
	../dlls/weapon/handgrenade.cpp
	../dlls/weapon/hornetgun.cpp
	../dlls/weapon/mp5.cpp
	../dlls/weapon/python.cpp
	../dlls/weapon/rpg.cpp
	../dlls/weapon/satchel.cpp
	../dlls/weapon/shotgun.cpp
	../dlls/weapon/squeak.cpp
	../dlls/weapon/tripmine.cpp
	../game_shared/util/util_entity.cpp
	../game_shared/util/util_printout.cpp
	../game_shared/util/util_shared.cpp
	../game_shared/util/util_version.cpp
	../game_shared/voice_banmgr.cpp
	../pm_shared/pm_debug.c
	../pm_shared/pm_math.c
	../pm_shared/pm_printout.c
	../pm_shared/pm_shared.c
	cdll_int.cpp
	cl_util.cpp
	custom_message.cpp
	demo.cpp
	entity.cpp
	events.cpp
	ev_common.cpp
	ev_hldm.cpp
	GameStudioModelRenderer.cpp
	hl/cl_player.cpp
	hl/cl_weapons.cpp
	hl/hl_baseentity.cpp
	hl/hl_events.cpp
	hl/hl_objects.cpp
	hl/hl_weapons.cpp
	hud.cpp
	hud/ammo.cpp
	hud/ammohistory.cpp
	hud/ammo_secondary.cpp
	hud/battery.cpp
	hud/death.cpp
	hud/flashlight.cpp
	hud/geiger.cpp
	hud/health.cpp
	hud/hud_spectator.cpp
	hud/menu.cpp
	hud/message.cpp
	hud/pain.cpp
	hud/saytext.cpp
	hud/statusbar.cpp
	hud/status_icons.cpp
	hud/text_message.cpp
	hud/train.cpp
	hud_redraw.cpp
	hud_update.cpp
	input.cpp
	in_camera.cpp
	parsemsg.cpp
	StudioModelRenderer.cpp
	studio_util.cpp
	tri.cpp
	view.cpp
	weapons_resource.cpp
)

# --- VGUI (FreeVGUI em plataformas nao-x86) ---
set (CLDLL_VGUI_SOURCES
	../game_shared/vgui_checkbutton2.cpp
	../game_shared/vgui_grid.cpp
	../game_shared/vgui_helpers.cpp
	../game_shared/vgui_listbox.cpp
	../game_shared/vgui_loadtga.cpp
	../game_shared/vgui_scrollbar2.cpp
	../game_shared/vgui_slider2.cpp
	../game_shared/voice_status.cpp
	vgui_ClassMenu.cpp
	vgui_ConsolePanel.cpp
	vgui_ControlConfigPanel.cpp
	vgui_CustomObjects.cpp
	vgui_int.cpp
	vgui_MOTDWindow.cpp
	vgui_SchemeManager.cpp
	vgui_ScorePanel.cpp
	vgui_SpectatorPanel.cpp
	vgui_TeamFortressViewport.cpp
	vgui_teammenu.cpp
)

if (AZ_NO_VGUI)
	# sem VGUI: stubs em vgui_stub.h / voice_status_stub.h (placar, menus de time e spectator ficam desligados)
else()
	list(APPEND CLDLL_SOURCES ${CLDLL_VGUI_SOURCES})
endif()

# --- plataforma ---
if (WIN32)
	list(APPEND CLDLL_SOURCES inputw32.cpp hud_servers.cpp)
else()
	list(APPEND CLDLL_SOURCES input_unix.cpp hud_servers_stub.cpp)
endif()

if (AZ_CRASH_HANDLER AND NOT WIN32)
	list(APPEND CLDLL_SOURCES ../public/az_crash_handler.cpp)
endif()

include_directories (. hl hud ../dlls ../dlls/entity ../dlls/weapon ../common ../engine ../pm_shared ../game_shared ../game_shared/util ../config ../utils/common ../public ..)

if (GOLDSOURCE_SUPPORT)
	include_directories (../external/)
endif()

include_directories(../utils/vgui/include)

add_library (${CLDLL_LIBRARY} SHARED ${CLDLL_SOURCES})
target_link_libraries(${CLDLL_LIBRARY} vcs_info)

if (NOT WIN32)
	target_link_libraries( ${CLDLL_LIBRARY} ${CMAKE_DL_LIBS} )
endif()
if (ANDROID)
	target_link_libraries( ${CLDLL_LIBRARY} log )
endif()
# o mesmo simbolo (vec3_origin, vJumpOrigin...) e definido em mais de um arquivo do mod original
if (NOT MSVC)
	set_property(TARGET ${CLDLL_LIBRARY} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--allow-multiple-definition")
endif()

if (NOT AZ_NO_VGUI)
	target_link_libraries(${CLDLL_LIBRARY} vgui)
endif()

if(WIN32)
	target_link_libraries( ${CLDLL_LIBRARY} user32.lib )
	if (GOLDSOURCE_SUPPORT)
		target_link_libraries( ${CLDLL_LIBRARY} winmm.lib )
	endif()
endif()

set_target_properties (${CLDLL_LIBRARY} PROPERTIES
	POSITION_INDEPENDENT_CODE 1)
if (ANDROID)
	set_target_properties (${CLDLL_LIBRARY} PROPERTIES
		LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/${ANDROID_ABI})
endif()

set_target_postfix(${CLDLL_LIBRARY})

# LTO desligado de proposito: o mod tem simbolos duplicados (--allow-multiple-definition) e codigo legado que o LTO pode agravar.

if(XASH_PS4)
	# signed module and names of its functions, see cmake/PS4.cmake
	ps4_add_module(${CLDLL_LIBRARY} "${GAMEDIR}/${CLIENT_INSTALL_DIR}/")
else()
	install(TARGETS ${CLDLL_LIBRARY}
		RUNTIME DESTINATION "${GAMEDIR}/${CLIENT_INSTALL_DIR}/"
		LIBRARY DESTINATION "${GAMEDIR}/${CLIENT_INSTALL_DIR}/"
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
			GROUP_READ GROUP_EXECUTE
			WORLD_READ WORLD_EXECUTE)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Release" AND NOT WIN32 AND NOT XASH_PS4 AND NOT AZ_NO_STRIP)
	if(APPLE)
		# Apple strip treats -s as a symbol list file, -x keeps exported symbols
		set(STRIP_FLAGS -x)
	else()
		set(STRIP_FLAGS -s)
	endif()
	add_custom_command(TARGET ${CLDLL_LIBRARY}
			POST_BUILD DEPENDS ${CLDLL_LIBRARY}
			COMMAND ${CMAKE_STRIP} ${STRIP_FLAGS} $<TARGET_FILE:${CLDLL_LIBRARY}>)
endif()
