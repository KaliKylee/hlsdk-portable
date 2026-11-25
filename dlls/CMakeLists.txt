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
project (SVDLL)

set (SVDLL_LIBRARY server)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT MSVC)
	add_compile_options(-Wno-invalid-offsetof) # GCC/Clang flag
	add_compile_options(-w) # HL:E code base is not warning-clean
	add_definitions(-D_LINUX -DLINUX)
	add_definitions(-Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf )
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

add_definitions(-DOPFOR -DQUIVER -DVOXEL -DQUAKE2 -DVALVE_DLL -DHL_DLL)

set (SVDLL_SOURCES
	../common/mathlib.cpp
	aflock.cpp
	agrunt.cpp
	anchor.cpp
	archer.cpp
	charger.cpp
	ctf/CHUDIconTrigger.cpp
	ctf/CItemAcceleratorCTF.cpp
	ctf/CItemBackpackCTF.cpp
	ctf/CItemCTF.cpp
	ctf/CItemLongJumpCTF.cpp
	ctf/CItemPortableHEVCTF.cpp
	ctf/CItemRegenerationCTF.cpp
	ctf/CItemSpawnCTF.cpp
	ctf/CTFDetect.cpp
	ctf/CTFGoal.cpp
	ctf/CTFGoalBase.cpp
	ctf/CTFGoalFlag.cpp
	ctf/ctfplay_gamerules.cpp
	ctf/CTFSpawn.cpp
	envmodel.cpp
	fireball.cpp
	airtank.cpp
	animating.cpp
	animation.cpp
	apache.cpp
	baby_voltigore.cpp
	barnacle.cpp
	barney.cpp
	bigmomma.cpp
	blkop_osprey.cpp
	blkop_apache.cpp
	bloater.cpp
	blowercannon.cpp
	bmodels.cpp
	bullsquid.cpp
	buttons.cpp
	cbase.cpp
	CDrillSergeant.cpp
	chummy_monster.cpp
	cleansuit_scientist.cpp
	client.cpp
	COFAllyMonster.cpp
	COFSquadTalkMonster.cpp
	combat.cpp
	controller.cpp
	coopplay_gamerules.cpp
	CRecruit.cpp
	crossbow.cpp
	crowbar.cpp
	defaultai.cpp
	doors.cpp
	effects.cpp
	egon.cpp
	ehandle.cpp
	explode.cpp
	flyingmonster.cpp
	func_break.cpp
	func_tank.cpp
	func_tank_of.cpp
	game.cpp
	gamerules.cpp
	gargantua.cpp
	gauss.cpp
	genericmonster.cpp
	geneworm.cpp
	ggrenade.cpp
	globals.cpp
	gman.cpp
	gonome.cpp
	handgrenade.cpp
	hassassin.cpp
	headcrab.cpp
	healthkit.cpp
	hfgrunt.cpp
	hgrunt.cpp
	hgrunt_ally.cpp
	hgrunt_medic.cpp
	hgrunt_torch.cpp
	hornet.cpp
	hornetgun.cpp
	houndeye.cpp
	h_ai.cpp
	h_battery.cpp
	h_cine.cpp
	h_cycler.cpp
	h_export.cpp
	ichthyosaur.cpp
	islave.cpp
	items.cpp
	item_generic.cpp
	kingpin.cpp
	leech.cpp
	lights.cpp
	loader.cpp
	male_assassin.cpp
	maprules.cpp
	monstermaker.cpp
	monsters.cpp
	monsterstate.cpp
	mortar.cpp
	mp5.cpp
	multiplay_gamerules.cpp
	nihilanth.cpp
	nodes.cpp
	nuclearbomb.cpp
	observer.cpp
	op4mortar.cpp
	osprey.cpp
	otis.cpp
	panthereye.cpp
	pathcorner.cpp
	penguin_grenade.cpp
	pitdrone.cpp
	pitworm_up.cpp
	plane.cpp
	plats.cpp
	player.cpp
	python.cpp
	rat.cpp
	roach.cpp
	rope/CElectrifiedWire.cpp
	rope/CRope.cpp
	rope/CRopeSample.cpp
	rope/CRopeSegment.cpp
	rpg.cpp
	satchel.cpp
	schedule.cpp
	scientist.cpp
	scripted.cpp
	shockroach.cpp
	shocktrooper.cpp
	shotgun.cpp
	singleplay_gamerules.cpp
	skeleton.cpp
	skill.cpp
	sound.cpp
	soundent.cpp
	hassault.cpp
	spectator.cpp
	squadmonster.cpp
	squeakgrenade.cpp
	subs.cpp
	talkmonster.cpp
	teamplay_gamerules.cpp
	tempmonster.cpp
	tentacle.cpp
	triggers.cpp
	tripmine.cpp
	turret.cpp
	UserMessages.cpp
	util.cpp
	voltigore.cpp
	weapons.cpp
	weapons/assaultrifle.cpp
	weapons/CGrapple.cpp
	weapons/CGrappleTip.cpp
	weapons/CDisplacer.cpp
	weapons/CDisplacerBall.cpp
	weapons/CEagle.cpp
	weapons/CEagleLaser.cpp
	weapons/chummy.cpp
	weapons/CKnife.cpp
	weapons/CM249.cpp
	weapons/CPenguin.cpp
	weapons/CPipewrench.cpp
	weapons/CShockBeam.cpp
	weapons/CShockRifle.cpp
	weapons/CSniperRifle.cpp
	weapons/CSpore.cpp
	weapons/CSporeLauncher.cpp
	weapons/glauncher.cpp
	weapons_shared.cpp
	world.cpp
	wpn_shared/hl_wpn_glock.cpp
	xen.cpp
	zombie.cpp
	zombie_barney.cpp
	zombie_hev.cpp
	zombie_soldier.cpp
	../game_shared/voice_gamemgr.cpp
	../pm_shared/pm_debug.cpp
	../pm_shared/pm_math.cpp
	../pm_shared/pm_shared.cpp
	../public/safe_snprintf.c
)

if (HAVE_STRLCPY)
	add_definitions(-DHAVE_STRLCPY)
else()
	list(APPEND SVDLL_SOURCES
		../external/openbsd/strlcpy.c)
endif()

if (HAVE_STRLCAT)
	add_definitions(-DHAVE_STRLCAT)
else()
	list(APPEND SVDLL_SOURCES
		../external/openbsd/strlcat.c)
endif()

include_directories (. weapons wpn_shared ctf rope ../common ../engine ../pm_shared ../game_shared ../public)

add_library (${SVDLL_LIBRARY} SHARED ${SVDLL_SOURCES})
target_link_libraries(${SVDLL_LIBRARY} vcs_info)

set_target_properties (${SVDLL_LIBRARY} PROPERTIES POSITION_INDEPENDENT_CODE 1)

set(SVDLL_NAME "${SERVER_LIBRARY_NAME}")
set_target_postfix_with_name(${SVDLL_LIBRARY} ${SVDLL_NAME})

if(HAVE_LTO)
	set_property(TARGET ${SVDLL_LIBRARY} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS ${SVDLL_LIBRARY}
	RUNTIME DESTINATION "${GAMEDIR}/${SERVER_INSTALL_DIR}/"
	LIBRARY DESTINATION "${GAMEDIR}/${SERVER_INSTALL_DIR}/"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
		GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE)

if(CMAKE_BUILD_TYPE MATCHES "Release" AND NOT WIN32)
	add_custom_command(TARGET ${SVDLL_LIBRARY}
			POST_BUILD DEPENDS ${SVDLL_LIBRARY}
			COMMAND ${CMAKE_STRIP} -s $<TARGET_FILE:${SVDLL_LIBRARY}>)
endif()
