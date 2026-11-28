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
// monster_human_assault - ThrillEX
// Reconstructed from the decompiled hl.dll (CHassault).
//
// Heavy weapons soldier with a minigun:
//  - has to spin the gun up before it can shoot (spin up delay is a skill cvar)
//  - spins down when the fight is over / when it has to move or is hurt badly
//  - fires bursts with a lot of spread (smaller on hard skill)
//  - punches enemies that come close (anim events 10/11/12)
//  - squad monster, can suppress an enemy hiding behind cover
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"squadmonster.h"
#include	"soundent.h"
#include	"weapons.h"
#include	"skill.h"
#include	"hitproxy.h"
#include	"trex_log.h"

#define HASSAULT_AE_SHOOT		1
#define HASSAULT_AE_PUNCH_A		10
#define HASSAULT_AE_PUNCH_B		11
#define HASSAULT_AE_PUNCH_C		12
#define HASSAULT_AE_STEP		13

#define HASSAULT_SPINUP_SCALE	2.0f	// the original delay felt too short on the phone

#define HASSAULT_SPIN_OFF		0
#define HASSAULT_SPIN_UP		1	// spinning up, can not shoot yet
#define HASSAULT_SPIN_READY		2	// ready to fire

enum
{
	TASK_HASSAULT_SPINDOWN = LAST_COMMON_TASK + 1,
};

enum
{
	SCHED_HASSAULT_SUPPRESS = LAST_COMMON_SCHEDULE + 1,	// unused id kept for numbering
	SCHED_HASSAULT_UNUSED1,
	SCHED_HASSAULT_SPINUP,
	SCHED_HASSAULT_UNUSED2,
	SCHED_HASSAULT_SPINDOWN,
};

static const char *pHAssaultStepSounds[] =
{
	"hgrunt/gr_step1.wav",
	"hgrunt/gr_step2.wav",
	"hgrunt/gr_step3.wav",
	"hgrunt/gr_step4.wav",
};

static const char *pHAssaultMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

class CHAssault : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int Classify( void ) { return CLASS_HUMAN_MILITARY; }
	int ISoundMask( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );
	void MonsterThink( void );
	void Killed( entvars_t *pevAttacker, int iGib );
	void UpdateOnRemove( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	Vector GetGunPosition( void );
	BOOL CheckMeleeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	void IdleSound( void );
	void PainSound( void );
	void DeathSound( void );
	void AlertSound( void );

	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );

	void Shoot( void );
	void Punch( float flDamage );
	void SpinDown( void );

	CUSTOM_SCHEDULES

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	int m_iBrassShell;
	int m_iSpinState;
	int m_iShotLog;
	float m_flSpinUpTime;
	float m_flNextPainTime;
	float m_flNextSuppress;
	EHANDLE m_hProxy;
};

LINK_ENTITY_TO_CLASS( monster_human_assault, CHAssault )

TYPEDESCRIPTION CHAssault::m_SaveData[] =
{
	DEFINE_FIELD( CHAssault, m_iSpinState, FIELD_INTEGER ),
	DEFINE_FIELD( CHAssault, m_flSpinUpTime, FIELD_TIME ),
	DEFINE_FIELD( CHAssault, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CHAssault, m_flNextSuppress, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CHAssault, CSquadMonster )

//=========================================================
// Schedules
//=========================================================
Task_t tlHAssaultFail[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, (float)2 },
	{ TASK_WAIT_PVS, (float)0 },
};

Schedule_t slHAssaultFail[] =
{
	{
		tlHAssaultFail,
		ARRAYSIZE( tlHAssaultFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Hassault Fail"
	},
};

Task_t tlHAssaultSpinup[] =
{
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_WAIT, (float)1.9 },
};

Schedule_t slHAssaultSpinup[] =
{
	{
		tlHAssaultSpinup,
		ARRAYSIZE( tlHAssaultSpinup ),
		bits_COND_HEAVY_DAMAGE,
		0,
		"Sergeant Spinup"
	},
};

Task_t tlHAssaultSpindown[] =
{
	{ TASK_HASSAULT_SPINDOWN, (float)0 },
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_WAIT, (float)0.75 },
};

Schedule_t slHAssaultSpindown[] =
{
	{
		tlHAssaultSpindown,
		ARRAYSIZE( tlHAssaultSpindown ),
		bits_COND_HEAVY_DAMAGE,
		0,
		"Hassault Spindown"
	},
};

Task_t tlHAssaultChase[] =
{
	{ TASK_HASSAULT_SPINDOWN, (float)0 },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_CHASE_ENEMY_FAILED },
	{ TASK_GET_PATH_TO_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slHAssaultChase[] =
{
	{
		tlHAssaultChase,
		ARRAYSIZE( tlHAssaultChase ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2 |
		bits_COND_HEAR_SOUND |
		bits_COND_TASK_FAILED,
		bits_SOUND_DANGER,
		"Hassault Chase Enemy"
	},
};

Task_t tlHAssaultMelee[] =
{
	{ TASK_HASSAULT_SPINDOWN, (float)0 },
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_MELEE_ATTACK1, (float)0 },
};

Schedule_t slHAssaultMelee[] =
{
	{
		tlHAssaultMelee,
		ARRAYSIZE( tlHAssaultMelee ),
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"Hassault Melee Attack"
	},
};

Task_t tlHAssaultRange1[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_TAKE_COVER_FROM_ENEMY },
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
};

Schedule_t slHAssaultRange1[] =
{
	{
		tlHAssaultRange1,
		ARRAYSIZE( tlHAssaultRange1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_NO_AMMO_LOADED |
		bits_COND_SPECIAL1,
		bits_SOUND_DANGER,
		"Range Attack1"
	},
};

DEFINE_CUSTOM_SCHEDULES( CHAssault )
{
	slHAssaultFail,
	slHAssaultSpinup,
	slHAssaultSpindown,
	slHAssaultChase,
	slHAssaultMelee,
	slHAssaultRange1,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHAssault, CSquadMonster )

//=========================================================
// Spin up / down
//=========================================================
void CHAssault::SpinDown( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "hassault/hw_gun4.wav", 0, 0, SND_STOP, 100 );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "hassault/hw_spin.wav", 0, 0, SND_STOP, 100 );
	ALERT( at_aiconsole, "HSarge: Spin down!\n" );
	TREX_Log( "hassault: spin down" );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "common/null.wav", 1.0, ATTN_NORM, 0, 100 );
	m_iSpinState = HASSAULT_SPIN_OFF;
	EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "hassault/hw_spindown.wav", 1.0, ATTN_NORM, 0, 100 );
	m_flSpinUpTime = 0;
}

void CHAssault::MonsterThink( void )
{
	if( m_iSpinState <= HASSAULT_SPIN_UP && m_flSpinUpTime != 0 && m_flSpinUpTime <= gpGlobals->time )
	{
		m_flSpinUpTime = 0;
		m_iSpinState = HASSAULT_SPIN_READY;
		TREX_Log( "hassault: spun up, ready to fire" );
	}

	CSquadMonster::MonsterThink();

	HitProxy_Sync( this, m_hProxy );
}

//=========================================================
// Schedule selection
//=========================================================
Schedule_t *CHAssault::GetSchedule( void )
{
	switch( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		if( m_iSpinState > HASSAULT_SPIN_UP )
			SpinDown();
		return CSquadMonster::GetSchedule();

	case MONSTERSTATE_COMBAT:
		if( HasConditions( bits_COND_ENEMY_DEAD ) )
		{
			if( m_iSpinState > HASSAULT_SPIN_UP )
				SpinDown();
			return CSquadMonster::GetSchedule();
		}

		if( HasConditions( bits_COND_NEW_ENEMY ) )
			return GetScheduleOfType( SCHED_WAKE_ANGRY );

		if( HasConditions( bits_COND_LIGHT_DAMAGE ) && !HasMemory( bits_MEMORY_FLINCHED ) )
			return GetScheduleOfType( SCHED_SMALL_FLINCH );

		if( HasConditions( bits_COND_HEAVY_DAMAGE ) && !HasMemory( bits_MEMORY_FLINCHED ) )
		{
			if( m_iSpinState > HASSAULT_SPIN_OFF )
				SpinDown();
			return GetScheduleOfType( SCHED_ALERT_BIG_FLINCH );
		}

		if( HasConditions( bits_COND_SEE_ENEMY ) )
		{
			if( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				if( m_iSpinState > HASSAULT_SPIN_UP )
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			if( m_iSpinState < HASSAULT_SPIN_UP )
			{
				TREX_Log( "hassault: spin up" );
				return GetScheduleOfType( SCHED_HASSAULT_SPINUP );
			}

			if( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				ALERT( at_aiconsole, "HSarge: Melee Attack!\n" );
				TREX_Log( "hassault: melee attack" );
				return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
			}

			if( HasConditions( bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK1 ) )
				return CSquadMonster::GetSchedule();
		}
		else
		{
			if( !HasConditions( bits_COND_ENEMY_OCCLUDED ) )
				return GetScheduleOfType( SCHED_COMBAT_FACE );

			if( m_iSpinState > HASSAULT_SPIN_UP && m_flNextSuppress < gpGlobals->time )
			{
				ALERT( at_aiconsole, "HSarge: Supressing!\n" );
				TREX_Log( "hassault: suppressing" );
				m_flNextSuppress = gpGlobals->time + 8.0f;
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}
		}
		TREX_Log( "hassault: chase" );
		return GetScheduleOfType( SCHED_CHASE_ENEMY );

	default:
		break;
	}
	return CSquadMonster::GetSchedule();
}

Schedule_t *CHAssault::GetScheduleOfType( int Type )
{
	switch( Type )
	{
	case SCHED_CHASE_ENEMY:
		return slHAssaultChase;
	case SCHED_MELEE_ATTACK1:
		return slHAssaultMelee;
	case SCHED_RANGE_ATTACK1:
		return slHAssaultRange1;
	case SCHED_FAIL:
		return slHAssaultFail;
	case SCHED_HASSAULT_SPINUP:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "hassault/hw_spinup.wav", 1.0, ATTN_NORM, 0, 100 );
		RouteClear();
		m_iSpinState = HASSAULT_SPIN_UP;
		m_flSpinUpTime = gpGlobals->time + HASSAULT_SPINUP_SCALE * ( gSkillData.hassaultSpinupDelay > 0 ? gSkillData.hassaultSpinupDelay : 1.0f );
		return slHAssaultSpinup;
	case SCHED_HASSAULT_SPINDOWN:
		return slHAssaultSpindown;
	default:
		break;
	}
	return CSquadMonster::GetScheduleOfType( Type );
}

//=========================================================
// Tasks
//=========================================================
void CHAssault::StartTask( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch( pTask->iTask )
	{
	case TASK_HASSAULT_SPINDOWN:
		if( m_iSpinState > HASSAULT_SPIN_OFF )
			SpinDown();
		TaskComplete();
		break;
	case TASK_RANGE_ATTACK1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "hassault/hw_gun4.wav", 1.0, 0.8, 0, 100 );
		if( m_IdealActivity != ACT_RANGE_ATTACK1 )
			m_IdealActivity = ACT_RANGE_ATTACK1;
		break;
	default:
		CSquadMonster::StartTask( pTask );
		break;
	}
}

void CHAssault::RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_WAIT_FOR_MOVEMENT:
		// stop chasing as soon as the gun can be used
		if( MovementIsComplete() || HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		{
			TaskComplete();
			RouteClear();
		}
		break;
	default:
		CSquadMonster::RunTask( pTask );
		break;
	}
}

//=========================================================
// Attack checks
//=========================================================
BOOL CHAssault::CheckMeleeAttack1( float flDot, float flDist )
{
	if( m_hEnemy == 0 )
		return FALSE;
	return flDist <= 64.0f && flDot >= 0.7f;
}

BOOL CHAssault::CheckRangeAttack1( float flDot, float flDist )
{
	if( flDist <= 2048.0f && flDot >= 0.5f && flDist > 64.0f &&
		( m_iSpinState >= HASSAULT_SPIN_READY || !HasConditions( bits_COND_ENEMY_OCCLUDED ) ) &&
		NoFriendlyFire() )
		return TRUE;
	return FALSE;
}

//=========================================================
// Shooting / punching
//=========================================================
Vector CHAssault::GetGunPosition( void )
{
	return pev->origin + Vector( 0, 0, 48 );
}

void CHAssault::Shoot( void )
{
	if( m_hEnemy == 0 )
		return;

	Vector vecShootOrigin = GetGunPosition();
	// aim straight at the enemy (ShootAtEnemy used a stale last-known position)
	Vector vecShootDir = ( m_hEnemy->BodyTarget( vecShootOrigin ) - vecShootOrigin ).Normalize();

	UTIL_MakeVectors( pev->angles );

	Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT( 40, 90 ) + gpGlobals->v_up * RANDOM_FLOAT( 75, 200 ) + gpGlobals->v_forward * RANDOM_FLOAT( -40, 40 );
	EjectBrass( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL );

	Vector vecSpread;
	switch( g_iSkillLevel )
	{
	case 1:
		vecSpread = VECTOR_CONE_7DEGREES;
		break;
	case 2:
		vecSpread = VECTOR_CONE_6DEGREES;
		break;
	case 3:
		vecSpread = VECTOR_CONE_4DEGREES;
		break;
	default:
		vecSpread = VECTOR_CONE_7DEGREES;
		break;
	}

	float flDmg = gSkillData.hassaultDmgCannon;
	if( flDmg <= 0 )
		flDmg = 8;

	// own bullet trace: the stock FireBullets did not reach the player in this port
	Vector vecDir = vecShootDir + RANDOM_FLOAT( -1, 1 ) * vecSpread.x * gpGlobals->v_right + RANDOM_FLOAT( -1, 1 ) * vecSpread.y * gpGlobals->v_up;
	TraceResult tr;
	UTIL_TraceLine( vecShootOrigin, vecShootOrigin + vecDir * 4096, dont_ignore_monsters, ENT( pev ), &tr );

	CBaseEntity *pHit = NULL;
	if( tr.flFraction != 1.0f && !FNullEnt( tr.pHit ) )
		pHit = CBaseEntity::Instance( tr.pHit );

	if( pHit && pHit->pev->takedamage )
	{
		ClearMultiDamage();
		pHit->TraceAttack( pev, flDmg, vecDir, &tr, DMG_BULLET );
		ApplyMultiDamage( pev, pev );
	}
	else if( tr.flFraction != 1.0f )
	{
		DecalGunshot( &tr, BULLET_MONSTER_MP5 );
	}

	if( ++m_iShotLog <= 8 || ( m_iShotLog % 25 ) == 0 )
	{
		float flEnemyDist = ( m_hEnemy->pev->origin - vecShootOrigin ).Length();
		TREX_Log( "hassault: shot #%d hit=%s end=%.0f dist_to_enemy=%.0f me=(%.0f %.0f %.0f) enemy=(%.0f %.0f %.0f) takedamage=%d",
			m_iShotLog,
			pHit ? STRING( pHit->pev->classname ) : ( tr.flFraction != 1.0f ? "world" : "nothing" ),
			tr.flFraction * 4096.0f, flEnemyDist,
			pev->origin.x, pev->origin.y, pev->origin.z,
			m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->origin.z,
			pHit ? (int)pHit->pev->takedamage : -1 );
	}

	pev->effects |= EF_MUZZLEFLASH;

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );

	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 384, 0.3f );
}

void CHAssault::Punch( float flDamage )
{
	CBaseEntity *pHurt = CheckTraceHullAttack( 70, (int)flDamage, DMG_CLUB );
	BOOL fHit = ( pHurt && pHurt->pev->takedamage != DAMAGE_NO );

	// fallback: the hull trace often hits nothing / the world in this port
	if( !fHit && m_hEnemy != 0 && m_hEnemy->pev->takedamage != DAMAGE_NO )
	{
		CBaseEntity *pEnemy = m_hEnemy;
		Vector2D vec2LOS = ( pEnemy->pev->origin - pev->origin ).Make2D();
		float flDist = vec2LOS.Length();
		UTIL_MakeAimVectors( pev->angles );
		float flDot = 0;
		if( flDist > 0 )
			flDot = DotProduct( vec2LOS.Normalize(), gpGlobals->v_forward.Make2D() );

		TraceResult tr;
		UTIL_TraceLine( pev->origin + Vector( 0, 0, 32 ), pEnemy->BodyTarget( pev->origin ), ignore_monsters, ENT( pev ), &tr );

		TREX_Log( "hassault: punch fallback dist=%.0f dot=%.2f los=%.2f hull=%s", flDist, flDot, tr.flFraction,
			pHurt ? STRING( pHurt->pev->classname ) : "nothing" );

		if( flDist <= 100 && flDot > 0.3f && tr.flFraction >= 0.95f )
		{
			pEnemy->TakeDamage( pev, pev, flDamage, DMG_CLUB );
			pHurt = pEnemy;
			fHit = TRUE;
		}
	}

	if( fHit )
	{
		if( pHurt->pev->flags & ( FL_CLIENT | FL_MONSTER ) )
		{
			pHurt->pev->punchangle.z = -20;
			pHurt->pev->punchangle.x = 5;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100;
		}
		TREX_Log( "hassault: punch hit %s dmg=%.0f", STRING( pHurt->pev->classname ), flDamage );
	}
	else
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pHAssaultMissSounds[RANDOM_LONG( 0, 1 )], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );
		TREX_Log( "hassault: punch missed" );
	}
}

void CHAssault::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case HASSAULT_AE_SHOOT:
		Shoot();
		break;
	case HASSAULT_AE_PUNCH_A:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/hoot2.wav", 1.0, ATTN_NORM );
		Punch( 15 );
		break;
	case HASSAULT_AE_PUNCH_B:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/hoot1.wav", 1.0, ATTN_NORM );
		Punch( 10 );
		break;
	case HASSAULT_AE_PUNCH_C:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/hoot3.wav", 1.0, ATTN_NORM );
		Punch( 5 );
		break;
	case HASSAULT_AE_STEP:
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, pHAssaultStepSounds[RANDOM_LONG( 0, 3 )], 1.0, ATTN_NORM );
		break;
	default:
		TREX_Log( "hassault: anim event %d", pEvent->event );
		CSquadMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Sounds / misc
//=========================================================
int CHAssault::ISoundMask( void )
{
	return bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_PLAYER | bits_SOUND_DANGER;
}

void CHAssault::SetYawSpeed( void )
{
	switch( m_Activity )
	{
	case ACT_WALK:
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		pev->yaw_speed = 180;
		break;
	case ACT_RANGE_ATTACK1:
		pev->yaw_speed = 120;
		break;
	default:
		pev->yaw_speed = 150;
		break;
	}
}

void CHAssault::IdleSound( void )
{
	SENTENCEG_PlayRndSz( ENT( pev ), "HG_RADIO", 0.4f, 0.8f, 0, 100 );
}

void CHAssault::PainSound( void )
{
	if( gpGlobals->time > m_flNextPainTime )
	{
		switch( RANDOM_LONG( 0, 6 ) )
		{
		case 0:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hgrunt/gr_pain3.wav", 1, ATTN_NORM );
			break;
		case 1:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hgrunt/gr_pain4.wav", 1, ATTN_NORM );
			break;
		case 2:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hgrunt/gr_pain5.wav", 1, ATTN_NORM );
			break;
		case 3:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hgrunt/gr_pain1.wav", 1, ATTN_NORM );
			break;
		case 4:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hgrunt/gr_pain2.wav", 1, ATTN_NORM );
			break;
		default:
			break;
		}
		m_flNextPainTime = gpGlobals->time + 1;
	}
}

void CHAssault::DeathSound( void )
{
	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hgrunt/gr_die1.wav", 1, ATTN_IDLE );
		break;
	case 1:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hgrunt/gr_die2.wav", 1, ATTN_IDLE );
		break;
	default:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hgrunt/gr_die3.wav", 1, ATTN_IDLE );
		break;
	}
}

void CHAssault::AlertSound( void )
{
	if( IsLeader() && RANDOM_LONG( 0, 1 ) )
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hgrunt/makesquad.wav", 1, ATTN_IDLE );
	else
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hassault/hw_alert.wav", 1, ATTN_IDLE );
}

int CHAssault::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if( g_iSkillLevel == 3 )
		flDamage *= 0.75f;
	return CSquadMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CHAssault::Killed( entvars_t *pevAttacker, int iGib )
{
	TREX_Log( "hassault: killed" );
	if( m_iSpinState > HASSAULT_SPIN_OFF )
		SpinDown();
	HitProxy_Remove( m_hProxy );
	CSquadMonster::Killed( pevAttacker, iGib );
}

void CHAssault::UpdateOnRemove( void )
{
	HitProxy_Remove( m_hProxy );
	CSquadMonster::UpdateOnRemove();
}

//=========================================================
// Spawn / Precache
//=========================================================
void CHAssault::Spawn( void )
{
	Precache();

	SET_MODEL( ENT( pev ), "models/hassault.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = 275;	// fixed (ThrillEX port); sk_hassault_health is ignored
	m_flFieldOfView = -1.0f;
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_flNextPainTime = gpGlobals->time;
	m_flNextSuppress = 0;
	m_iSpinState = HASSAULT_SPIN_OFF;
	m_iShotLog = 0;
	m_flSpinUpTime = 0;
	m_fEnemyEluded = FALSE;

	MonsterInit();

	TREX_Log( "hassault: spawned at %.0f %.0f %.0f health=%.0f spinup=%.2f(x2) dmg=%.1f skill=%d",
		pev->origin.x, pev->origin.y, pev->origin.z, pev->health, gSkillData.hassaultSpinupDelay, gSkillData.hassaultDmgCannon, g_iSkillLevel );
}

void CHAssault::Precache( void )
{
	PRECACHE_MODEL( "models/hassault.mdl" );
	PRECACHE_SOUND( "hassault/hw_gun4.wav" );
	PRECACHE_SOUND( "hassault/hw_spin.wav" );
	PRECACHE_SOUND( "hassault/hw_spinup.wav" );
	PRECACHE_SOUND( "hassault/hw_spindown.wav" );
	PRECACHE_SOUND( "hassault/hw_alert.wav" );
	PRECACHE_SOUND( "hassault/hw_shoot1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die3.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain3.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain4.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain5.wav" );
	PRECACHE_SOUND_ARRAY( pHAssaultStepSounds );
	PRECACHE_SOUND_ARRAY( pHAssaultMissSounds );
	PRECACHE_SOUND( "player/hoot1.wav" );
	PRECACHE_SOUND( "player/hoot2.wav" );
	PRECACHE_SOUND( "player/hoot3.wav" );
	PRECACHE_SOUND( "hgrunt/makesquad.wav" );
	PRECACHE_SOUND( "common/null.wav" );
	m_iBrassShell = PRECACHE_MODEL( "models/shell.mdl" );
}
