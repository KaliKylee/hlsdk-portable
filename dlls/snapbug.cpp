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
#include	"trex_log.h"

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
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	void MonsterThink( void );
	void EXPORT LeapTouch( CBaseEntity *pOther );
	void EXPORT SnapBugSubterrThink( void );

	void UpdateOnRemove( void );
	void ReleaseVictim( void );
	void CheckSubterrTrigger( void );
	void CheckStepOn( void );

	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	float m_flNextCapture;
	BOOL m_fCatchPlayer;	// currently holding something
	BOOL m_fCaughtPrey;	// touch disabled until m_flNextCapture
	EHANDLE m_hVictim;
	Vector m_vecOldOrigin;
	BOOL m_fTrapActivated;
	Vector m_vecHold;
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
	DEFINE_FIELD( CSnapBug, m_vecHold, FIELD_POSITION_VECTOR ),
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
	TREX_Log( "snapbug: spawned health=%.1f takedamage=%.0f solid=%d", pev->health, pev->takedamage, pev->solid );

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

void CSnapBug::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	TREX_Log( "snapbug: TraceAttack dmg=%.1f type=0x%x hitgroup=%d health=%.1f takedamage=%.0f", flDamage, bitsDamageType, ptr->iHitgroup, pev->health, pev->takedamage );
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

int CSnapBug::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	TREX_Log( "snapbug: TakeDamage dmg=%.1f type=0x%x health=%.1f", flDamage, bitsDamageType, pev->health );
	m_afMemory |= bits_MEMORY_PROVOKED;
	if( m_fCatchPlayer )
		ReleaseVictim();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CSnapBug::UpdateOnRemove( void )
{
	if( m_fCatchPlayer )
		ReleaseVictim();
	CBaseMonster::UpdateOnRemove();
}

void CSnapBug::ReleaseVictim( void )
{
	CBaseEntity *pVictim = m_hVictim;
	m_hVictim = NULL;
	m_fCatchPlayer = FALSE;
	ALERT( at_aiconsole, "CSnapBug: Released the target!\n" );
}

//=========================================================
// Touch: snap shut on whatever stepped on us
//=========================================================
void CSnapBug::LeapTouch( CBaseEntity *pOther )
{
	static float s_flLog = 0;
	BOOL fLog = ( gpGlobals->time > s_flLog );
	if( fLog )
		s_flLog = gpGlobals->time + 1.0f;

	if( pev->movetype == MOVETYPE_NOCLIP )	// still buried
		return;
	if( !pOther->pev->takedamage )
		return;
	if( pOther->pev->origin.z < pev->origin.z )	// target must be above us
		return;
	if( fLog )
		TREX_Log( "snapbug: touch %s onground=%d dz=%.1f", STRING( pOther->pev->classname ), ( pev->flags & FL_ONGROUND ) ? 1 : 0, pOther->pev->origin.z - pev->origin.z );

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
			m_vecHold = pOther->pev->origin;	// held in place, but can still look and shoot
			TREX_Log( "snapbug: captured player" );
		}
		else
		{
			ALERT( at_aiconsole, "CSnapbug: Can't capture target as it will get stuck!\n" );
			TREX_Log( "snapbug: capture blocked (trace fraction %.2f)", tr.flFraction );
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
		CheckStepOn();
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
		|| sqrt( flDx * flDx + flDy * flDy ) > 48.0f )
	{
		ALERT( at_aiconsole, "CSnapBug: Target's too far!\n" );
		TREX_Log( "snapbug: released, target out of range (dz=%.1f dxy=%.1f)", pevV->origin.z - pev->origin.z, (float)sqrt( flDx * flDx + flDy * flDy ) );
		ReleaseVictim();
		m_flNextCapture = gpGlobals->time + 1.5f;
		return;
	}

	if( m_flNextCapture - 1.5f <= gpGlobals->time )
	{
		ReleaseVictim();
		return;
	}

	pevV->velocity = g_vecZero;
	pevV->origin.x = m_vecHold.x;
	pevV->origin.y = m_vecHold.y;
	if( pevV->origin.z - pev->origin.z < 80.0f )
		pevV->origin.z += 1.0f;
}

//=========================================================
// The player can simply walk over a bug this small without the engine
// ever calling Touch, so also check proximity every think.
//=========================================================
void CSnapBug::CheckStepOn( void )
{
	if( m_fCaughtPrey || pev->movetype == MOVETYPE_NOCLIP || pev->deadflag != DEAD_NO )
		return;

	CBaseEntity *pEnt = NULL;
	while( ( pEnt = UTIL_FindEntityInSphere( pEnt, pev->origin, 48 ) ) != NULL )
	{
		if( pEnt == this || !pEnt->pev->takedamage || !pEnt->IsAlive() )
			continue;
		if( !pEnt->IsPlayer() && IRelationship( pEnt ) != R_HT )
			continue;

		float flDx = pEnt->pev->origin.x - pev->origin.x;
		float flDy = pEnt->pev->origin.y - pev->origin.y;
		float flDz = pEnt->pev->origin.z - pev->origin.z;
		if( sqrt( flDx * flDx + flDy * flDy ) <= 30.0f && flDz >= 0 && flDz <= 60.0f )
		{
			LeapTouch( pEnt );
			return;
		}
	}
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
