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
// monster_diablo - ThrillEX leaping predator (panther.mdl)
// Reconstructed from the decompiled hl.dll (CDiablo). It is a CBaseMonster
// that melee-slashes like a zombie and leaps like a headcrab (long leap,
// 128..448 units, with its own cooldown).
// Parts marked TODO(verify) are guesses.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"skill.h"
#include	"game.h"

#define	DIABLO_AE_SLASH_LEFT	( 1 )
#define	DIABLO_AE_SLASH_RIGHT	( 2 )
#define	DIABLO_AE_LEAP		( 3 )

class CDiablo : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int Classify( void ) { return CLASS_ALIEN_PREDATOR; }
	void IdleSound( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	void EXPORT LeapTouch( CBaseEntity *pOther );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	BOOL CheckMeleeAttack1( float flDot, float flDist );
	BOOL CheckMeleeAttack2( float flDot, float flDist );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	Schedule_t *GetScheduleOfType( int Type );

	void Slash( int iSide );
	void Leap( void );

	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	float m_flNextLeap;
	float m_flNextMelee;
	float m_flLastHurt;

	CUSTOM_SCHEDULES
};

LINK_ENTITY_TO_CLASS( monster_diablo, CDiablo )

TYPEDESCRIPTION CDiablo::m_SaveData[] =
{
	DEFINE_FIELD( CDiablo, m_flNextLeap, FIELD_TIME ),
	DEFINE_FIELD( CDiablo, m_flNextMelee, FIELD_TIME ),
	DEFINE_FIELD( CDiablo, m_flLastHurt, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CDiablo, CBaseMonster )

static const char *pDiabloIdleSounds[] =
{
	"panthereye/pa_idle1.wav",
	"panthereye/pa_idle2.wav",
	"panthereye/pa_idle3.wav",
	"panthereye/pa_idle4.wav",
};

// slashes (hit / miss), same set the zombie uses
static const char *pDiabloClawSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

static const char *pDiabloMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

//=========================================================
// Schedules
//=========================================================
Task_t tlDiabloRangeAttack1[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
};

Schedule_t slDiabloRangeAttack1[] =
{
	{
		tlDiabloRangeAttack1,
		ARRAYSIZE( tlDiabloRangeAttack1 ),
		0,
		0,
		"Diablo Range Attack1"
	},
};

DEFINE_CUSTOM_SCHEDULES( CDiablo )
{
	slDiabloRangeAttack1,
};

IMPLEMENT_CUSTOM_SCHEDULES( CDiablo, CBaseMonster )

Schedule_t *CDiablo::GetScheduleOfType( int Type )
{
	if( Type == SCHED_RANGE_ATTACK1 )
		return &slDiabloRangeAttack1[0];

	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// Spawn / Precache
//=========================================================
void CDiablo::Spawn( void )
{
	Precache();

	SET_MODEL( ENT( pev ), "models/panther.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 48 ) );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = 145;
	pev->effects = 0;
	pev->health = gSkillData.diabloHealth;
	m_flFieldOfView = 0.4f;
	m_MonsterState = MONSTERSTATE_NONE;

	// TODO(verify): the original also tweaks two distances right before MonsterInit
	// (96 and 272); they match m_flDistTooFar / m_flDistLook.
	m_flDistTooFar = 96;
	m_flDistLook = 272;

	m_flNextLeap = 0;
	m_flNextMelee = 0;
	m_flLastHurt = 0;

	MonsterInit();
}

void CDiablo::Precache( void )
{
	PRECACHE_MODEL( "models/panther.mdl" );

	for( int i = 0; i < (int)ARRAYSIZE( pDiabloIdleSounds ); i++ )
		PRECACHE_SOUND( pDiabloIdleSounds[i] );

	for( int i = 0; i < (int)ARRAYSIZE( pDiabloClawSounds ); i++ )
		PRECACHE_SOUND( pDiabloClawSounds[i] );
}

void CDiablo::SetYawSpeed( void )
{
	pev->yaw_speed = 100;
}

void CDiablo::IdleSound( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pDiabloIdleSounds[RANDOM_LONG( 0, ARRAYSIZE( pDiabloIdleSounds ) - 1 )], 1.0, 0.8, 0, 100 );
}

//=========================================================
// Damage
//=========================================================
int CDiablo::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if( !FClassnameIs( pevAttacker, "monster_headcrab" ) )
		m_flLastHurt = gpGlobals->time;

	// bullets hurt it 30% less
	if( bitsDamageType & DMG_BULLET )
		flDamage *= 0.7f;

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// Attack decisions
//=========================================================
BOOL CDiablo::CheckMeleeAttack1( float flDot, float flDist )
{
	if( m_flNextMelee > gpGlobals->time || !CBaseMonster::CheckMeleeAttack1( flDot, flDist ) )
		return FALSE;

	m_flNextMelee = gpGlobals->time + RANDOM_FLOAT( 2.5f, 3.5f );
	return TRUE;
}

BOOL CDiablo::CheckMeleeAttack2( float flDot, float flDist )
{
	return !( pev->flags & FL_WATERJUMP ) && CBaseMonster::CheckMeleeAttack2( flDot, flDist );
}

BOOL CDiablo::CheckRangeAttack1( float flDot, float flDist )
{
	if( ( pev->flags & FL_ONGROUND ) && flDist <= 448 && flDist >= 128 && flDot >= 0.65f && m_flNextLeap <= gpGlobals->time )
	{
		if( m_hEnemy == 0 )
			return FALSE;

		TraceResult tr;
		UTIL_TraceLine( pev->origin, m_hEnemy->Center(), ignore_monsters, ignore_glass, ENT( pev ), &tr );

		if( tr.flFraction == 1.0f )
			return TRUE;
	}

	return FALSE;
}

//=========================================================
// Tasks
//=========================================================
void CDiablo::StartTask( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		m_IdealActivity = ACT_RANGE_ATTACK1;
		break;
	default:
		CBaseMonster::StartTask( pTask );
		break;
	}
}

void CDiablo::RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if( m_fSequenceFinished )
		{
			TaskComplete();
			SetTouch( NULL );
			m_IdealActivity = ACT_IDLE;
		}
		break;
	default:
		CBaseMonster::RunTask( pTask );
		break;
	}
}

//=========================================================
// Anim events
//=========================================================
void CDiablo::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case DIABLO_AE_SLASH_LEFT:
		Slash( 1 );
		break;
	case DIABLO_AE_SLASH_RIGHT:
		Slash( 2 );
		break;
	case DIABLO_AE_LEAP:
		Leap();
		break;
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

void CDiablo::Slash( int iSide )
{
	CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.diabloDmgSlash, DMG_SLASH );

	if( pHurt )
	{
		if( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) )
		{
			float flSide = ( iSide == 2 ) ? 1.0f : -1.0f;

			pHurt->pev->punchangle.z = -18;
			pHurt->pev->punchangle.x = 5 * flSide;
			pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100 * flSide;
		}

		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pDiabloClawSounds[RANDOM_LONG( 0, 2 )], 1.0, 0.8, 0, 100 + RANDOM_LONG( -5, 5 ) );
	}
	else
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pDiabloMissSounds[RANDOM_LONG( 0, 1 )], 1.0, 0.8, 0, 100 + RANDOM_LONG( -5, 5 ) );
	}
}

void CDiablo::Leap( void )
{
	SetTouch( &CDiablo::LeapTouch );

	ClearBits( pev->flags, FL_ONGROUND );

	UTIL_SetOrigin( pev, pev->origin + Vector( 0, 0, 1 ) );

	UTIL_MakeVectors( pev->angles );

	Vector vecJumpDir;

	if( m_hEnemy != 0 )
	{
		float gravity = g_psv_gravity->value;
		if( gravity <= 1 )
			gravity = 1;

		// how high the enemy's centre is above us
		float height = m_hEnemy->Center().z - pev->origin.z;
		if( height < 16 )
			height = 16;

		float speed = sqrt( 2 * gravity * height );
		float time = speed / gravity;

		vecJumpDir = ( m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs ) - pev->origin;
		vecJumpDir = vecJumpDir * ( 1.0f / time );

		// the original scales the whole jump down to 80%
		vecJumpDir.x *= 0.8f;
		vecJumpDir.y *= 0.8f;
		vecJumpDir.z = speed * 0.8f;
	}
	else
	{
		vecJumpDir = Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z ) * 350;
	}

	pev->velocity = vecJumpDir;

	m_flNextLeap = gpGlobals->time + RANDOM_FLOAT( 3.0f, 7.5f );
}

void CDiablo::LeapTouch( CBaseEntity *pOther )
{
	if( !pOther->pev->takedamage )
		return;

	if( !( pev->flags & FL_ONGROUND ) )
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pDiabloClawSounds[RANDOM_LONG( 0, 2 )], 1.0, 0.8, 0, 100 + RANDOM_LONG( -5, 5 ) );
		pOther->TakeDamage( pev, pev, gSkillData.diabloDmgJump, DMG_SLASH );
	}

	SetTouch( NULL );
}
