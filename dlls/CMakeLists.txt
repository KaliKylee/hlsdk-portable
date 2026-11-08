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
project (SVDLL)

set (SVDLL_LIBRARY server)

add_definitions(-DVALVE_DLL -DQUIVER -DVOXEL -DQUAKE2)

if(NOT MSVC)
	# Veja cl_dll/CMakeLists.txt: sem -fno-exceptions (util.cpp usa try/catch).
	# Os simbolos tambem NAO ficam ocultos: o save/restore resolve os ponteiros de funcao das entidades
	# por NOME (NameForFunction / FunctionFromName do engine), entao os metodos precisam estar exportados.
	add_compile_options(-fsigned-char)
	add_compile_options(-fno-strict-aliasing)
	add_compile_options(-fms-extensions)
	add_compile_options(-Wno-invalid-offsetof)
	add_compile_options(-w)
	add_definitions(-D_LINUX -DLINUX -DPOSIX)
	add_definitions(-Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf )
	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-c++11-narrowing>)
		add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-address-of-temporary>)
		add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-non-pod-varargs>)
		add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-implicit-function-declaration>)
		add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-incompatible-pointer-types>)
		add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-int-conversion>)
	endif()
	add_compile_options("SHELL:-include ${CMAKE_SOURCE_DIR}/public/az_compat.h")
	add_definitions(-DAZ_ANDROID)
else()
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

set (SVDLL_SOURCES
	../config/external_lib_include.cpp
	../common/vector.cpp
	../game_shared/util/util_entity.cpp
	../game_shared/util/util_printout.cpp
	../game_shared/util/util_shared.cpp
	../game_shared/util/util_version.cpp
	../game_shared/voice_gamemgr.cpp
	../pm_shared/pm_debug.c
	../pm_shared/pm_math.c
	../pm_shared/pm_printout.c
	../pm_shared/pm_shared.c
	basemonster.cpp
	cbase.cpp
	client.cpp
	client_message.cpp
	combat.cpp
	decals.cpp
	defaultai.cpp
	entity/aflock.cpp
	entity/agrunt.cpp
	entity/airtank.cpp
	entity/animating.cpp
	entity/apache.cpp
	entity/archer.cpp
	entity/archer_ball.cpp
	entity/barnacle.cpp
	entity/barney.cpp
	entity/bigmomma.cpp
	entity/bmodels.cpp
	entity/bullsquid.cpp
	entity/buttons.cpp
	entity/chumtoad.cpp
	entity/controller.cpp
	entity/controller_head_ball.cpp
	entity/controller_zap_ball.cpp
	entity/crossbowbolt.cpp
	entity/doors.cpp
	entity/effects.cpp
	entity/explode.cpp
	entity/floater.cpp
	entity/flyingmonster.cpp
	entity/friendly.cpp
	entity/func_break.cpp
	entity/func_door_health.cpp
	entity/func_tank.cpp
	entity/gargantua.cpp
	entity/genericmonster.cpp
	entity/ggrenade.cpp
	entity/gib.cpp
	entity/gman.cpp
	entity/hassassin.cpp
	entity/hassault.cpp
	entity/headcrab.cpp
	entity/healthkit.cpp
	entity/hgrunt.cpp
	entity/hornet.cpp
	entity/hornet_kingpin.cpp
	entity/houndeye.cpp
	entity/h_battery.cpp
	entity/h_cine.cpp
	entity/h_cycler.cpp
	entity/ichthyosaur.cpp
	entity/islave.cpp
	entity/items.cpp
	entity/kingpin.cpp
	entity/kingpin_ball.cpp
	entity/leech.cpp
	entity/lights.cpp
	entity/maprules.cpp
	entity/monstermaker.cpp
	entity/mortar.cpp
	entity/nihilanth.cpp
	entity/nodes.cpp
	entity/osprey.cpp
	entity/panthereye.cpp
	entity/pathcorner.cpp
	entity/pickupwalker.cpp
	entity/plats.cpp
	entity/player.cpp
	entity/player_extra.cpp
	entity/rat.cpp
	entity/roach.cpp
	entity/scientist.cpp
	entity/scripted.cpp
	entity/snapbug.cpp
	entity/sound.cpp
	entity/soundent.cpp
	entity/spectator.cpp
	entity/squadmonster.cpp
	entity/squidspit.cpp
	entity/stukabat.cpp
	entity/subs.cpp
	entity/talkmonster.cpp
	entity/tentacle.cpp
	entity/triggers.cpp
	entity/turret.cpp
	entity/world.cpp
	entity/xen.cpp
	entity/zombie.cpp
	game.cpp
	gamerules.cpp
	gamerules_multiplay.cpp
	gamerules_singleplay.cpp
	gamerules_teamplay.cpp
	globals.cpp
	healthmodule.cpp
	h_export.cpp
	monstersavestate.cpp
	monsterstate.cpp
	plane.cpp
	schedule.cpp
	skill.cpp
	util.cpp
	util_debugdraw.cpp
	util_model.cpp
	weapons.cpp
	weapon/chumtoadweapon.cpp
	weapon/crossbow.cpp
	weapon/crowbar.cpp
	weapon/egon.cpp
	weapon/gauss.cpp
	weapon/glock.cpp
	weapon/handgrenade.cpp
	weapon/hornetgun.cpp
	weapon/mp5.cpp
	weapon/python.cpp
	weapon/rpg.cpp
	weapon/satchel.cpp
	weapon/shotgun.cpp
	weapon/squeak.cpp
	weapon/tripmine.cpp
)

if (AZ_CRASH_HANDLER AND NOT WIN32)
	list(APPEND SVDLL_SOURCES ../public/az_crash_handler.cpp)
endif()

include_directories (. entity weapon ../common ../engine ../pm_shared ../game_shared ../game_shared/util ../config ../utils/common ../public ..)

add_library (${SVDLL_LIBRARY} SHARED ${SVDLL_SOURCES})
target_link_libraries(${SVDLL_LIBRARY} vcs_info)
if (NOT WIN32)
	target_link_libraries(${SVDLL_LIBRARY} ${CMAKE_DL_LIBS})
endif()
if (ANDROID)
	target_link_libraries(${SVDLL_LIBRARY} log)
endif()
if (NOT MSVC)
	set_property(TARGET ${SVDLL_LIBRARY} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--allow-multiple-definition")
endif()

set_target_properties (${SVDLL_LIBRARY} PROPERTIES POSITION_INDEPENDENT_CODE 1)
if (ANDROID)
	set_target_properties (${SVDLL_LIBRARY} PROPERTIES
		LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/${ANDROID_ABI})
endif()

if(APPLE AND NOT "${SERVER_LIBRARY_NAME_OSX}" STREQUAL "")
	# on Apple platforms we always use gamedll_osx liblist/gameinfo key, if it's set
	set(SVDLL_NAME "${SERVER_LIBRARY_NAME_OSX}")
elseif(NOT WIN32 AND NOT "${SERVER_LIBRARY_NAME_LINUX}" STREQUAL "")
	# on non-Windows platforms we use gamedll_linux liblist/gameinfo key, if it's set
	set(SVDLL_NAME "${SERVER_LIBRARY_NAME_LINUX}")
else()
	# on Windows or if no gamedll_osx or gamedll_linux is set, use default gamedll key
	set(SVDLL_NAME "${SERVER_LIBRARY_NAME}")
endif()

set_target_postfix_with_name(${SVDLL_LIBRARY} ${SVDLL_NAME})

# LTO desligado de proposito: o mod tem simbolos duplicados (--allow-multiple-definition) e codigo legado que o LTO pode agravar.

if(XASH_PS4)
	# signed module and names of its functions, see cmake/PS4.cmake
	ps4_add_module(${SVDLL_LIBRARY} "${GAMEDIR}/${SERVER_INSTALL_DIR}/")
else()
	install(TARGETS ${SVDLL_LIBRARY}
		RUNTIME DESTINATION "${GAMEDIR}/${SERVER_INSTALL_DIR}/"
		LIBRARY DESTINATION "${GAMEDIR}/${SERVER_INSTALL_DIR}/"
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
	add_custom_command(TARGET ${SVDLL_LIBRARY}
			POST_BUILD DEPENDS ${SVDLL_LIBRARY}
			COMMAND ${CMAKE_STRIP} ${STRIP_FLAGS} $<TARGET_FILE:${SVDLL_LIBRARY}>)
endif()
