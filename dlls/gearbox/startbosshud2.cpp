/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// teleport.cpp - tiny, jumpy alien parasite
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"game.h"

extern int gmsgBossBarHealthNameView2;
extern int gmsgBossBarHealthNameView3;

class CStartBossHud2 : public CBaseMonster
{
     void Spawn(void);
     void EXPORT BOSSHUD2(void);
     void EXPORT BOSSHUD3(void);
};
LINK_ENTITY_TO_CLASS( startbosshud2, CStartBossHud2 );

void CStartBossHud2 :: Spawn()
{
	MESSAGE_BEGIN( MSG_ONE, gmsgBossBarHealthNameView2, NULL, UTIL_PlayerByIndex(1)->pev );
		WRITE_BYTE	( 1 );
		WRITE_COORD ( ENTINDEX ( edict() ) );
		WRITE_LONG ( 0 );
	MESSAGE_END(); 

	SetThink( &CStartBossHud2::BOSSHUD2 );
	pev->nextthink = gpGlobals->time + 1.0;
}

void CStartBossHud2 :: BOSSHUD2()
{
	MESSAGE_BEGIN( MSG_ONE, gmsgBossBarHealthNameView3, NULL, UTIL_PlayerByIndex(1)->pev );
		WRITE_BYTE	( 1 );
		WRITE_COORD ( ENTINDEX ( edict() ) );
		WRITE_LONG ( 0 );
	MESSAGE_END(); 

	SetThink( &CStartBossHud2::BOSSHUD3 );
	pev->nextthink = gpGlobals->time + 1.0;
}


void CStartBossHud2 :: BOSSHUD3()
{
	MESSAGE_BEGIN( MSG_ONE, gmsgBossBarHealthNameView2, NULL, UTIL_PlayerByIndex(1)->pev );
		WRITE_BYTE	( 0 );
		WRITE_COORD ( ENTINDEX ( edict() ) );
		WRITE_LONG ( 0 );
	MESSAGE_END();
	
	MESSAGE_BEGIN( MSG_ONE, gmsgBossBarHealthNameView3, NULL, UTIL_PlayerByIndex(1)->pev );
		WRITE_BYTE	( 0 );
		WRITE_COORD ( ENTINDEX ( edict() ) );
		WRITE_LONG ( 0 );
	MESSAGE_END(); 
}
