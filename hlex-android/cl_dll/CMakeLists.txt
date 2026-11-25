#
# Copyright (c) 2015 Pavlo Lavrenenko
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

set (CLDLL_LIBRARY client)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DCLIENT_DLL -DOPFOR -DHL_DLL)

if(NOT MSVC)
	add_compile_options(-w) # HL:E code base is not warning-clean
	add_definitions(-D_LINUX -DLINUX)
	add_definitions(-Dstricmp=strcasecmp -Dstrnicmp=strncasecmp)
	if(NOT MINGW)
		add_definitions(-D_snprintf=snprintf -D_vsnprintf=vsnprintf)
	endif()
	if(CMAKE_BUILD_TYPE MATCHES "Release"
		OR (CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo"
		AND CMAKE_SYSTEM_NAME STREQUAL "Android"))
		add_compile_options(-fno-unwind-tables -fno-asynchronous-unwind-tables) # GCC/Clang flag
		add_compile_options(-fomit-frame-pointer) # GCC/Clang flag
		add_compile_options(-ftree-vectorize) # GCC/Clang flag
	endif()
else()
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

if (GOLDSOURCE_SUPPORT)
	add_definitions(-DGOLDSOURCE_SUPPORT)
endif()

if (USE_STATIC_FREEVGUI)
	add_definitions(-DINTERNAL_VGUI_SUPPORT)
endif()

# Set to ON to use the HL:E custom studio renderer (fullbright _light.mdl system).
# It touches engine internals, so it is OFF by default on Xash3D builds.
option(HLEX_STUDIO_RENDERER "Use HL:E custom studio model renderer" OFF)
if (HLEX_STUDIO_RENDERER)
	add_definitions(-DHLEX_STUDIO_RENDERER)
endif()

set (CLDLL_SOURCES
	ammo.cpp
	ammohistory.cpp
	ammo_secondary.cpp
	battery.cpp
	cdll_int.cpp
	com_weapons.cpp
	death.cpp
	demo.cpp
	entity.cpp
	events.cpp
	ev_common.cpp
	ev_hldm.cpp
	flashlight.cpp
	fog.cpp
	GameStudioModelRenderer.cpp
	geiger.cpp
	health.cpp
	hl/hl_baseentity.cpp
	hl/hl_events.cpp
	hl/hl_objects.cpp
	hl/hl_weapons.cpp
	hud.cpp
	hud_bench.cpp
	hud_benchtrace.cpp
	hud_flagicons.cpp
	hud_msg.cpp
	hud_playerbrowse.cpp
	hud_redraw.cpp
	hud_spectator.cpp
	hud_update.cpp
	input.cpp
	interpolation.cpp
	in_camera.cpp
	menu.cpp
	message.cpp
	rain.cpp
	saytext.cpp
	scoreboard.cpp
	statusbar.cpp
	status_icons.cpp
	StudioModelRenderer.cpp
	studio_util.cpp
	text_message.cpp
	train.cpp
	tri.cpp
	util.cpp
	vgui_ClassMenu.cpp
	vgui_CustomObjects.cpp
	vgui_int.cpp
	vgui_MOTDWindow.cpp
	vgui_SchemeManager.cpp
	vgui_ScorePanel.cpp
	vgui_SpectatorPanel.cpp
	vgui_StatsMenuPanel.cpp
	vgui_TeamFortressViewport.cpp
	vgui_teammenu.cpp
	view.cpp
	voice_status.cpp
	zoom.cpp
	../common/mathlib.cpp
	../common/parsemsg.cpp
	../dlls/crossbow.cpp
	../dlls/crowbar.cpp
	../dlls/egon.cpp
	../dlls/ehandle.cpp
	../dlls/gauss.cpp
	../dlls/handgrenade.cpp
	../dlls/hornetgun.cpp
	../dlls/mp5.cpp
	../dlls/python.cpp
	../dlls/rpg.cpp
	../dlls/satchel.cpp
	../dlls/shotgun.cpp
	../dlls/squeakgrenade.cpp
	../dlls/tripmine.cpp
	../dlls/weapons/assaultrifle.cpp
	../dlls/weapons/CDisplacer.cpp
	../dlls/weapons/CEagle.cpp
	../dlls/weapons/CGrapple.cpp
	../dlls/weapons/CGrappleTip.cpp
	../dlls/weapons/chummy.cpp
	../dlls/weapons/CKnife.cpp
	../dlls/weapons/CM249.cpp
	../dlls/weapons/CPenguin.cpp
	../dlls/weapons/CPipewrench.cpp
	../dlls/weapons/CShockRifle.cpp
	../dlls/weapons/CSniperRifle.cpp
	../dlls/weapons/CSporeLauncher.cpp
	../dlls/weapons/glauncher.cpp
	../dlls/weapons_shared.cpp
	../dlls/wpn_shared/hl_wpn_glock.cpp
	../game_shared/vgui_checkbutton2.cpp
	../game_shared/vgui_grid.cpp
	../game_shared/vgui_helpers.cpp
	../game_shared/vgui_listbox.cpp
	../game_shared/vgui_loadtga.cpp
	../game_shared/vgui_scrollbar2.cpp
	../game_shared/vgui_slider2.cpp
	../game_shared/voice_banmgr.cpp
	../pm_shared/pm_debug.cpp
	../pm_shared/pm_math.cpp
	../pm_shared/pm_shared.cpp
	../public/interface.cpp
	demo.cpp
	input_xash3d.cpp
	input_mouse.cpp
	../public/safe_snprintf.c
	../public/interface.cpp
)

if (HAVE_STRLCPY)
	add_definitions(-DHAVE_STRLCPY)
else()
	list(APPEND CLDLL_SOURCES
		../external/openbsd/strlcpy.c)
endif()

if (HAVE_STRLCAT)
	add_definitions(-DHAVE_STRLCAT)
else()
	list(APPEND CLDLL_SOURCES
		../external/openbsd/strlcat.c)
endif()

include_directories (. hl/ ../dlls ../dlls/weapons ../dlls/wpn_shared ../common ../engine ../pm_shared ../game_shared ../public)

if (GOLDSOURCE_SUPPORT)
	include_directories (../external/)
endif()

include_directories(../utils/vgui/include)

add_library (${CLDLL_LIBRARY} SHARED ${CLDLL_SOURCES})
target_link_libraries(${CLDLL_LIBRARY} vcs_info)
target_link_libraries(${CLDLL_LIBRARY} ${CMAKE_DL_LIBS})
target_link_libraries(${CLDLL_LIBRARY} vgui)

set_target_properties (${CLDLL_LIBRARY} PROPERTIES
	POSITION_INDEPENDENT_CODE 1)

set_target_postfix(${CLDLL_LIBRARY})

if(HAVE_LTO)
	set_property(TARGET ${CLDLL_LIBRARY} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS ${CLDLL_LIBRARY}
	RUNTIME DESTINATION "${GAMEDIR}/${CLIENT_INSTALL_DIR}/"
	LIBRARY DESTINATION "${GAMEDIR}/${CLIENT_INSTALL_DIR}/"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
		GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE)

if(CMAKE_BUILD_TYPE MATCHES "Release" AND NOT WIN32)
	add_custom_command(TARGET ${CLDLL_LIBRARY}
			POST_BUILD DEPENDS ${CLDLL_LIBRARY}
			COMMAND ${CMAKE_STRIP} -s $<TARGET_FILE:${CLDLL_LIBRARY}>)
endif()
