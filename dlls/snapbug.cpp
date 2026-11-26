/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
****/
//=========================================================
// monster_snapbug - ThrillEX snap trap (snapbug.mdl)
// Reconstructed from the decompiled hl.dll (CSnapBug).
// A tiny insect that sits on the floor. When something steps on it, it
// snaps shut on the target's feet, holds it and lifts it for a few seconds.
// Spawnflag 1024 = "subterranean": it starts buried 16 units under the floor
// and rises when a living entity gets within 100 units.
// Insects (headcrab, chumtoad, cockroach) are simply eaten.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"skill.h"
#include	"game.h"

#define SF_SNAPBUG_SUBTERRANEAN		1024

class CSnapBug : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void ) { return CLASS_INSECT; }
	void SetYawSpeed( void ) { pev->yaw_speed = 100; }
	int IRelationship( CBaseEntity *pTarget );
	void PainSound( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void MonsterThink( void );
	void EXPORT LeapTouch( CBaseEntity *pOther );
	void EXPORT SnapBugSubterrThink( void );

	void ReleaseVictim( void );
	void CheckSubterrTrigger( void );

	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	float m_flNextCapture;
	BOOL m_fCatchPlayer;	// currently holding something
	BOOL m_fCaughtPrey;	// touch disabled until m_flNextCapture
	EHANDLE m_hVictim;
	Vector m_vecOldOrigin;
	BOOL m_fTrapActivated;
};

LINK_ENTITY_TO_CLASS( monster_snapbug, CSnapBug )

TYPEDESCRIPTION CSnapBug::m_SaveData[] =
{
	DEFINE_FIELD( CSnapBug, m_flNextCapture, FIELD_TIME ),
	DEFINE_FIELD( CSnapBug, m_fCatchPlayer, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSnapBug, m_fCaughtPrey, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSnapBug, m_hVictim, FIELD_EHANDLE ),
	DEFINE_FIELD( CSnapBug, m_vecOldOrigin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CSnapBug, m_fTrapActivated, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CSnapBug, CBaseMonster )

void CSnapBug::Spawn( void )
{
	Precache();

	SET_MODEL( ENT( pev ), "models/snapbug.mdl" );
	UTIL_SetSize( pev, Vector( -12, -12, 0 ), Vector( 12, 12, 8 ) );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = 54;
	pev->effects = 0;
	pev->health = gSkillData.snapbugHealth;
	m_flFieldOfView = 0.5f;
	m_MonsterState = MONSTERSTATE_NONE;
	pev->yaw_speed = 100;

	m_flNextCapture = 0;
	m_fCatchPlayer = FALSE;
	m_fCaughtPrey = FALSE;
	m_hVictim = NULL;
	m_fTrapActivated = FALSE;

	MonsterInit();

	pev->nextthink = gpGlobals->time + 0.5f;
	UTIL_SetOrigin( pev, pev->origin );
	SetTouch( &CSnapBug::LeapTouch );

	if( pev->spawnflags & SF_SNAPBUG_SUBTERRANEAN )
	{
		m_vecOldOrigin = pev->origin;
		pev->origin.z -= 16;
		SetThink( &CSnapBug::SnapBugSubterrThink );
		pev->movetype = MOVETYPE_NOCLIP;
	}
}

void CSnapBug::Precache( void )
{
	PRECACHE_SOUND( "snapbug/catch.wav" );
	PRECACHE_MODEL( "models/snapbug.mdl" );
	PRECACHE_SOUND( "snapbug/sb_pain1.wav" );
	PRECACHE_SOUND( "snapbug/sb_dig1.wav" );
}

void CSnapBug::PainSound( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "snapbug/sb_pain1.wav", 1.0, ATTN_IDLE, 0, 100 );
}

// Everything is an enemy except passive things and insects; players only once provoked.
int CSnapBug::IRelationship( CBaseEntity *pTarget )
{
	if( pTarget->IsPlayer() )
		return ( m_afMemory & bits_MEMORY_PROVOKED ) ? R_HT : R_NO;

	switch( pTarget->Classify() )
	{
	case CLASS_NONE:
	case CLASS_HUMAN_PASSIVE:
	case CLASS_INSECT:
	case CLASS_PLAYER_ALLY:
	case 99:
		return R_NO;
	default:
		return R_HT;
	}
}

int CSnapBug::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	m_afMemory |= bits_MEMORY_PROVOKED;
	if( m_fCatchPlayer )
		ReleaseVictim();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CSnapBug::ReleaseVictim( void )
{
	CBaseEntity *pVictim = m_hVictim;
	if( pVictim )
		pVictim->pev->flags &= ~FL_IMMUNE_LAVA;	// original uses bit 0x80000 as "held" marker
	m_hVictim = NULL;
	m_fCatchPlayer = FALSE;
	ALERT( at_aiconsole, "CSnapBug: Released the target!\n" );
}

//=========================================================
// Touch: snap shut on whatever stepped on us
//=========================================================
void CSnapBug::LeapTouch( CBaseEntity *pOther )
{
	if( !( pev->flags & FL_ONGROUND ) )
		return;
	if( !pOther->pev->takedamage )
		return;
	if( pOther->pev->origin.z < pev->origin.z )	// target must be above us
		return;

	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "snapbug/catch.wav", 1.0, ATTN_NORM, 0, 100 );
	MakeIdealYaw( pOther->pev->origin );
	ChangeYaw( (int)pev->yaw_speed );
	m_fCaughtPrey = TRUE;

	if( pOther->IsPlayer() )
	{
		pOther->TakeDamage( pev, pev, gSkillData.snapbugDmgBite, DMG_SLASH );

		TraceResult tr;
		UTIL_TraceLine( pev->origin, pOther->pev->origin, ignore_monsters, ignore_glass, ENT( pev ), &tr );
		if( tr.flFraction == 1.0f )
		{
			m_flNextCapture = gpGlobals->time + 10.0f;
			pOther->pev->velocity.x = 0;
			pOther->pev->velocity.y = 0;
			m_hVictim = pOther;
			m_fCatchPlayer = TRUE;
			SetTouch( NULL );
			pOther->pev->flags |= FL_IMMUNE_LAVA;
		}
		else
		{
			ALERT( at_aiconsole, "CSnapbug: Can't capture target as it will get stuck!\n" );
			SetTouch( NULL );
			m_flNextCapture = gpGlobals->time + 1.0f;
		}
	}
	else
	{
		const char *szClass = STRING( pOther->pev->classname );
		if( FStrEq( szClass, "monster_headcrab" ) || FStrEq( szClass, "monster_chumtoad" ) || FStrEq( szClass, "monster_cockroach" ) )
			pOther->Killed( pev, GIB_ALWAYS );
		else
			pOther->TakeDamage( pev, pev, gSkillData.snapbugDmgBite, DMG_SLASH );

		SetTouch( NULL );
		m_flNextCapture = gpGlobals->time + 1.0f;
	}
}

//=========================================================
// Think: hold and lift the victim
//=========================================================
void CSnapBug::MonsterThink( void )
{
	pev->nextthink = gpGlobals->time;

	if( m_fCaughtPrey && gpGlobals->time >= m_flNextCapture )
	{
		m_fCaughtPrey = FALSE;
		SetTouch( &CSnapBug::LeapTouch );
	}

	if( !m_fCatchPlayer )
	{
		CBaseMonster::MonsterThink();
		return;
	}

	CBaseEntity *pVictim = m_hVictim;
	if( !pVictim )
	{
		m_fCatchPlayer = FALSE;
		m_flNextCapture = gpGlobals->time + 1.5f;
		return;
	}

	entvars_t *pevV = pVictim->pev;
	float flDx = pevV->origin.x - pev->origin.x;
	float flDy = pevV->origin.y - pev->origin.y;

	if( pevV->movetype == MOVETYPE_NOCLIP
		|| pevV->origin.z <= pev->origin.z
		|| pevV->origin.z - 72.0f > pev->origin.z
		|| sqrt( flDx * flDx + flDy * flDy ) > 36.0f )
	{
		ALERT( at_aiconsole, "CSnapBug: Target's too far!\n" );
		ReleaseVictim();
		m_flNextCapture = gpGlobals->time + 1.5f;
		return;
	}

	if( m_flNextCapture - 1.5f <= gpGlobals->time )
	{
		ReleaseVictim();
		return;
	}

	if( pevV->origin.z - pev->origin.z < 80.0f )
		pevV->origin.z += 1.0f;
}

//=========================================================
// Buried version: wait for something to come close, then rise
//=========================================================
void CSnapBug::CheckSubterrTrigger( void )
{
	CBaseEntity *pEnt = NULL;
	while( ( pEnt = UTIL_FindEntityInSphere( pEnt, pev->origin, 100 ) ) != NULL )
	{
		if( pEnt == this || !pEnt->IsAlive() || pEnt->Classify() == CLASS_NONE )
			continue;
		if( FStrEq( STRING( pEnt->pev->classname ), "monster_snapbug" ) )
			continue;

		m_fTrapActivated = TRUE;
		ALERT( at_aiconsole, "Snaptrap found target\n" );
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "snapbug/sb_dig1.wav", 1.0, ATTN_NORM, 0, 100 );
		SetTouch( &CSnapBug::LeapTouch );
		pev->spawnflags &= ~SF_SNAPBUG_SUBTERRANEAN;
		return;
	}
}

void CSnapBug::SnapBugSubterrThink( void )
{
	if( !m_fTrapActivated )
		CheckSubterrTrigger();

	if( pev->origin.z <= m_vecOldOrigin.z && m_fTrapActivated )
		pev->origin.z += 0.75f;

	if( pev->origin.z >= m_vecOldOrigin.z )
	{
		// risen: finish the normal monster start-up that Spawn skipped
		m_fTrapActivated = FALSE;
		pev->movetype = MOVETYPE_STEP;
		SetTouch( &CSnapBug::LeapTouch );
		SetThink( &CBaseMonster::MonsterInitThink );
		pev->nextthink = gpGlobals->time + 0.1f;
		return;
	}

	pev->nextthink = m_fTrapActivated ? gpGlobals->time : gpGlobals->time + 0.1f;
}
