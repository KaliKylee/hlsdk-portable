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
// monster_kingpin - ThrillEX big alien boss (kingpin.mdl)
// Reconstructed from the decompiled hl.dll (CKingpin).
//  - melee claw (zombie claw sounds), pulls the victim towards it
//  - special anim events: "zap" that throws the enemy, and a psychic
//    pull used by the Range Attack schedule
//  - dies by fading out, turning red and exploding into gibs and blood
// Parts marked TODO(verify) are guesses.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"skill.h"
#include	"game.h"
#include	"trex_log.h"
#include	"hitproxy.h"

#define KINGPIN_AE_CLAW		1
#define KINGPIN_AE_ZAP		3
#define KINGPIN_AE_NOTHING	4

#define SF_KINGPIN_PASSIVE	64	// does not attack players until provoked

class CKingpin : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void ) { pev->yaw_speed = 100; }
	int Classify( void ) { return CLASS_ALIEN_MONSTER; }
	int IRelationship( CBaseEntity *pTarget );
	void AlertSound( void );
	void IdleSound( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckMeleeAttack1( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	void Killed( entvars_t *pevAttacker, int iGib );
	void MonsterThink( void );
	void UpdateOnRemove( void );
	void RunTask( Task_t *pTask );
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );

	void EXPORT DyingThink( void );
	void EXPORT Blowup( void );

	void Zap( CBaseEntity *pEnemy );

	EHANDLE m_hProxy;

	CUSTOM_SCHEDULES
};

LINK_ENTITY_TO_CLASS( monster_kingpin, CKingpin )

static const char *pKingpinHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

static const char *pKingpinMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

//=========================================================
// Schedules
//=========================================================
Task_t tlKingpinRangeAttack1[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
};

Schedule_t slKingpinRangeAttack1[] =
{
	{
		tlKingpinRangeAttack1,
		ARRAYSIZE( tlKingpinRangeAttack1 ),
		0x110021,
		0,
		"Kingpin Range Attack1"
	},
};

DEFINE_CUSTOM_SCHEDULES( CKingpin )
{
	slKingpinRangeAttack1,
};

IMPLEMENT_CUSTOM_SCHEDULES( CKingpin, CBaseMonster )

Schedule_t *CKingpin::GetScheduleOfType( int Type )
{
	if( Type == SCHED_RANGE_ATTACK2 )
		return &slKingpinRangeAttack1[0];
	return CBaseMonster::GetScheduleOfType( Type );
}

// No flinching in combat: when hurt it just keeps standing its ground.
Schedule_t *CKingpin::GetSchedule( void )
{
	if( m_MonsterState == MONSTERSTATE_COMBAT && HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		return GetScheduleOfType( SCHED_COMBAT_STAND );
	return CBaseMonster::GetSchedule();
}

//=========================================================
// Spawn / Precache
//=========================================================
void CKingpin::Spawn( void )
{
	Precache();

	SET_MODEL( ENT( pev ), "models/kingpin.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 90 ) );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = 54;
	pev->effects = 0;
	pev->health = gSkillData.kingpinHealth;
	m_flFieldOfView = -1.0f;	// sees all around
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_RANGE_ATTACK1 | bits_CAP_MELEE_ATTACK1;

	MonsterInit();
	TREX_Log( "kingpin: spawned health=%.1f takedamage=%.0f solid=%d", pev->health, pev->takedamage, pev->solid );
}

void CKingpin::Precache( void )
{
	PRECACHE_MODEL( "models/kingpin.mdl" );

	PRECACHE_SOUND( "kingpin/ki_alert1.wav" );
	PRECACHE_SOUND( "kingpin/ki_idle1.wav" );
	PRECACHE_SOUND( "kingpin/ki_idle2.wav" );
	PRECACHE_SOUND( "kingpin/ki_attack1.wav" );

	for( int i = 0; i < (int)ARRAYSIZE( pKingpinHitSounds ); i++ )
		PRECACHE_SOUND( (char *)pKingpinHitSounds[i] );
	for( int i = 0; i < (int)ARRAYSIZE( pKingpinMissSounds ); i++ )
		PRECACHE_SOUND( (char *)pKingpinMissSounds[i] );

	UTIL_PrecacheOther( "test_effect" );
	PRECACHE_SOUND( "common/bodysplat.wav" );
	PRECACHE_MODEL( "models/agibs.mdl" );
}

//=========================================================
// Relationship, sounds
//=========================================================
int CKingpin::IRelationship( CBaseEntity *pTarget )
{
	if( pTarget->IsPlayer() && ( pev->spawnflags & SF_KINGPIN_PASSIVE ) && !( m_afMemory & bits_MEMORY_PROVOKED ) )
		return R_NO;
	return CBaseMonster::IRelationship( pTarget );
}

void CKingpin::AlertSound( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "kingpin/ki_alert1.wav", 1.0, ATTN_NORM, 0, 100 );
}

void CKingpin::IdleSound( void )
{
	if( RANDOM_LONG( 0, 1 ) )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "kingpin/ki_idle1.wav", 1.0, ATTN_NORM, 0, 100 );
	else
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "kingpin/ki_idle2.wav", 1.0, ATTN_NORM, 0, 100 );
}

BOOL CKingpin::CheckMeleeAttack1( float flDot, float flDist )
{
	return flDist < 128.0f;
}

void CKingpin::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	TREX_Log( "kingpin: TraceAttack dmg=%.1f type=0x%x hitgroup=%d health=%.1f", flDamage, bitsDamageType, ptr->iHitgroup, pev->health );
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

int CKingpin::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	TREX_Log( "kingpin: TakeDamage dmg=%.1f type=0x%x health=%.1f takedamage=%.0f", flDamage, bitsDamageType, pev->health, pev->takedamage );
	if( pevAttacker && ( pevAttacker->flags & FL_CLIENT ) )
		m_afMemory |= bits_MEMORY_PROVOKED;

	// the original compares for exactly these types
	if( bitsDamageType == DMG_CLUB || bitsDamageType == DMG_BULLET )
		flDamage *= 0.7f;

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// Animation events
//=========================================================
void CKingpin::Zap( CBaseEntity *pEnemy )
{
	// throws the enemy away from the kingpin and hurts it
	UTIL_MakeVectors( pev->angles );
	// TODO(verify): the original adds the kingpin's own origin to this velocity,
	// which would fling the target absurdly; a plain launch is used instead.
	pEnemy->pev->punchangle.x = 5;
	pEnemy->pev->punchangle.z = -10;
	pEnemy->pev->velocity = pEnemy->pev->velocity + gpGlobals->v_forward * 192 + Vector( 0, 0, gpGlobals->v_up.z * 128 );
	pEnemy->pev->flags &= ~FL_ONGROUND;

	pEnemy->TakeDamage( pev, pev, gSkillData.kingpinDmgZap, 0 );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "kingpin/ki_attack1.wav", 1.0, ATTN_NORM, 0, 100 );
}

void CKingpin::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case KINGPIN_AE_CLAW:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 96, gSkillData.kingpinDmgClaw, DMG_SLASH );
			if( pHurt )
			{
				if( pHurt->pev->flags & ( FL_CLIENT | FL_MONSTER ) )
				{
					pHurt->pev->punchangle.x = 5;
					// pulls the victim towards the kingpin
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * -100;
				}
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pKingpinHitSounds[RANDOM_LONG( 0, ARRAYSIZE( pKingpinHitSounds ) - 1 )], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );
			}
			else
			{
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pKingpinMissSounds[RANDOM_LONG( 0, ARRAYSIZE( pKingpinMissSounds ) - 1 )], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );
			}
		}
		break;
	case 2:
		break;
	case KINGPIN_AE_ZAP:
		{
			CBaseEntity *pEnemy = m_hEnemy;
			if( pEnemy )
				Zap( pEnemy );
		}
		break;
	case KINGPIN_AE_NOTHING:
		return;
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Psychic pull while the range attack plays
//=========================================================
void CKingpin::RunTask( Task_t *pTask )
{
	if( pTask->iTask != TASK_RANGE_ATTACK1 )
	{
		CBaseMonster::RunTask( pTask );
		return;
	}

	CBaseEntity *pEnemy = m_hEnemy;
	if( pEnemy )
	{
		if( pEnemy->pev->flags & FL_CLIENT )
		{
			pEnemy->pev->velocity.x *= 0.5f;
			pEnemy->pev->velocity.y *= 0.5f;
		}
		else if( pEnemy->pev->flags & FL_MONSTER )
		{
			CBaseMonster *pMonster = pEnemy->MyMonsterPointer();
			if( pMonster )
				pMonster->m_MonsterState = MONSTERSTATE_NONE;
		}

		Vector vecSrc = pEnemy->BodyTarget( pev->origin );
		vecSrc.z += 8;
		Vector vecDir = pev->origin - pEnemy->pev->origin;
		UTIL_BloodStream( vecSrc, vecDir, 238, 32 );
	}

	MakeIdealYaw( m_vecEnemyLKP );
	ChangeYaw( (int)pev->yaw_speed );

	if( m_fSequenceFinished )
	{
		m_Activity = ACT_RESET;
		TaskComplete();
	}
}

//=========================================================
// Death: fade out, turn red, explode
//=========================================================
void CKingpin::MonsterThink( void )
{
	CBaseMonster::MonsterThink();
	HitProxy_Sync( this, m_hProxy );
}

void CKingpin::UpdateOnRemove( void )
{
	HitProxy_Remove( m_hProxy );
	CBaseMonster::UpdateOnRemove();
}

void CKingpin::Killed( entvars_t *pevAttacker, int iGib )
{
	HitProxy_Remove( m_hProxy );
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );

	Vector vecEffect = pev->origin;
	vecEffect.z += 32;
	CBaseEntity *pEffect = CBaseEntity::Create( "test_effect", vecEffect, g_vecZero, NULL );
	if( pEffect )
		pEffect->Use( this, this, USE_ON, 0 );

	pev->renderamt = 255;
	pev->rendermode = kRenderTransTexture;
	pev->takedamage = DAMAGE_NO;
	SetThink( &CKingpin::DyingThink );
	pev->nextthink = gpGlobals->time;
}

void CKingpin::DyingThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1f;

	if( pev->renderamt <= 80 )
	{
		pev->renderfx = kRenderFxExplode;
		pev->rendercolor = Vector( 255, 0, 0 );
		SetThink( &CKingpin::Blowup );
	}
	else
	{
		pev->renderamt -= 5;
		StudioFrameAdvance();
	}
}

void CKingpin::Blowup( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "common/bodysplat.wav", 1.0, ATTN_NORM, 0, 100 );
	pev->solid = SOLID_NOT;

	CGib::SpawnRandomGibs( pev, 9, 0 );	// TODO(verify): alien gibs assumed

	for( int i = 0; i < 8; i++ )
	{
		Vector vecSrc = pev->origin;
		vecSrc.z += 48;
		UTIL_BloodStream( vecSrc, UTIL_RandomBloodVector(), BloodColor(), RANDOM_LONG( 80, 150 ) );
	}

	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}
