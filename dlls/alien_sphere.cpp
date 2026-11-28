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
// monster_alien_sphere (+ xenlaser) - ThrillEX
// Reconstructed from the decompiled hl.dll (CXenSphere / CXenLaser).
//
//  - stationary, never moves. Hears something -> deploys (ACT_ARM)
//  - when deployed it can see enemies and shoots xen lasers (anim event 1)
//  - when nothing is going on it retires (ACT_DISARM)
//  - takes 40% less damage while its "angry idle" is playing
//  - bursts into shrapnel when killed
//  - can be triggered: USE_ON deploys, USE_OFF retires
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"soundent.h"
#include	"weapons.h"
#include	"decals.h"
#include	"hitproxy.h"
#include	"trex_log.h"

#define SPHERE_AE_FIRE		1
#define SPHERE_LASER_SPEED	2100.0f
#define SPHERE_LASER_DAMAGE	10.0f
#define SPHERE_LOOK_FALLBACK	1.0f	// seconds after which the attack fires even without the anim event

enum
{
	SCHED_SPHERE_DEPLOY = LAST_COMMON_SCHEDULE + 1,
	SCHED_SPHERE_RETIRE,
	SCHED_SPHERE_ANGRY_IDLE,
	SCHED_SPHERE_ALERT_FACE,
	SCHED_SPHERE_COMBAT_FACE,
};

//=========================================================
// xenlaser: the projectile
//=========================================================
class CXenLaser : public CBaseEntity
{
public:
	void Spawn( void );
	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	void Touch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( xenlaser, CXenLaser )

void CXenLaser::Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "xenlaser" );
	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;

	SET_MODEL( ENT( pev ), "models/laser.mdl" );
	pev->frame = 0;
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
}

void CXenLaser::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CXenLaser *pLaser = GetClassPtr( (CXenLaser *)NULL );
	pLaser->Spawn();

	UTIL_SetOrigin( pLaser->pev, vecStart );
	pLaser->pev->velocity = vecVelocity;
	pLaser->pev->owner = ENT( pevOwner );
	pLaser->pev->angles = UTIL_VecToAngles( vecVelocity );
	pLaser->SetTouch( &CXenLaser::Touch );
	// safety: remove after a while if it never hits anything
	pLaser->SetThink( &CBaseEntity::SUB_Remove );
	pLaser->pev->nextthink = gpGlobals->time + 5.0f;
}

void CXenLaser::Touch( CBaseEntity *pOther )
{
	if( pOther->edict() == pev->owner )
		return;

	if( pOther->pev->takedamage )
	{
		entvars_t *pevOwner = pev->owner ? VARS( pev->owner ) : pev;
		pOther->TakeDamage( pev, pevOwner, SPHERE_LASER_DAMAGE, DMG_ENERGYBEAM );
	}

	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "weapons/electro4.wav", 0.3, 0.8, 0, RANDOM_LONG( 90, 99 ) );

	SetTouch( NULL );
	pev->velocity = g_vecZero;
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

//=========================================================
// monster_alien_sphere
//=========================================================
class CXenSphere : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int Classify( void ) { return CLASS_ALIEN_MONSTER; }
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void Killed( entvars_t *pevAttacker, int iGib );
	void UpdateOnRemove( void );
	void MonsterThink( void );
	void Look( int iDistance );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	Vector GetGunPosition( void );
	void SphereUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void FireLaser( void );

	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );
	CBaseEntity *DropItem( const char *, const Vector &, const Vector & ) { return NULL; }

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES

	int m_iShrapnel;
	int m_fDeployed;
	int m_fShooting;
	float m_flNextShot;
	float m_flAtkStart;
	int m_fFiredThisAnim;
	EHANDLE m_hProxy;
};

LINK_ENTITY_TO_CLASS( monster_alien_sphere, CXenSphere )

TYPEDESCRIPTION CXenSphere::m_SaveData[] =
{
	DEFINE_FIELD( CXenSphere, m_fDeployed, FIELD_INTEGER ),
	DEFINE_FIELD( CXenSphere, m_flNextShot, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CXenSphere, CBaseMonster )

//=========================================================
// Schedules
//=========================================================
Task_t tlSphereDeploy[] =
{
	{ TASK_PLAY_SEQUENCE, (float)ACT_ARM },
};

Schedule_t slSphereDeploy[] =
{
	{
		tlSphereDeploy,
		ARRAYSIZE( tlSphereDeploy ),
		0,
		0,
		"Sphere Deploy"
	},
};

Task_t tlSphereRetire[] =
{
	{ TASK_PLAY_SEQUENCE, (float)ACT_DISARM },
};

Schedule_t slSphereRetire[] =
{
	{
		tlSphereRetire,
		ARRAYSIZE( tlSphereRetire ),
		0,
		0,
		"Sphere Retire"
	},
};

Task_t tlSphereAngryIdle[] =
{
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE_ANGRY },
	{ TASK_WAIT_INDEFINITE, (float)0 },
};

Schedule_t slSphereAngryIdle[] =
{
	{
		tlSphereAngryIdle,
		ARRAYSIZE( tlSphereAngryIdle ),
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_FEAR |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_SMELL_FOOD |
		bits_COND_HEAR_SOUND |
		bits_COND_SEE_ENEMY,
		0,
		"Sphere Angry Idle"
	},
};

Task_t tlSphereAlertFace[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE_ANGRY },
	{ TASK_FACE_IDEAL, (float)0 },
};

Schedule_t slSphereAlertFace[] =
{
	{
		tlSphereAlertFace,
		ARRAYSIZE( tlSphereAlertFace ),
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_FEAR |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_PROVOKED,
		0,
		"Sphere Alert Face"
	},
};

Task_t tlSphereCombatFace[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE_ANGRY },
	{ TASK_FACE_ENEMY, (float)0 },
};

Schedule_t slSphereCombatFace[] =
{
	{
		tlSphereCombatFace,
		ARRAYSIZE( tlSphereCombatFace ),
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_FEAR |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_SMELL_FOOD |
		bits_COND_HEAR_SOUND |
		bits_COND_SEE_ENEMY,
		0,
		"Sphere Combat Face"
	},
};

DEFINE_CUSTOM_SCHEDULES( CXenSphere )
{
	slSphereDeploy,
	slSphereRetire,
	slSphereAngryIdle,
	slSphereAlertFace,
	slSphereCombatFace,
};

IMPLEMENT_CUSTOM_SCHEDULES( CXenSphere, CBaseMonster )

Schedule_t *CXenSphere::GetScheduleOfType( int Type )
{
	switch( Type )
	{
	case SCHED_SPHERE_DEPLOY:
		return slSphereDeploy;
	case SCHED_SPHERE_RETIRE:
		return slSphereRetire;
	case SCHED_SPHERE_ANGRY_IDLE:
		return slSphereAngryIdle;
	case SCHED_WAKE_ANGRY:
	case SCHED_COMBAT_FACE:
	case SCHED_CHASE_ENEMY:
		return slSphereCombatFace;
	case SCHED_ALERT_FACE:
		return slSphereAlertFace;
	default:
		break;
	}
	return CBaseMonster::GetScheduleOfType( Type );
}

Schedule_t *CXenSphere::GetSchedule( void )
{
	switch( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
		if( m_fDeployed )
		{
			TREX_Log( "Sphere: retire" );
			m_fDeployed = 0;
			return GetScheduleOfType( SCHED_SPHERE_RETIRE );
		}
		return GetScheduleOfType( SCHED_IDLE_STAND );

	case MONSTERSTATE_COMBAT:
		if( HasConditions( bits_COND_ENEMY_DEAD ) )
			return CBaseMonster::GetSchedule();

		if( HasConditions( bits_COND_SEE_ENEMY ) )
		{
			if( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			return GetScheduleOfType( SCHED_COMBAT_FACE );
		}
		TREX_Log( "Sphere: angry idle" );
		return GetScheduleOfType( SCHED_SPHERE_ANGRY_IDLE );

	case MONSTERSTATE_ALERT:
		TREX_Log( "Sphere: alert (deployed=%d hear=%d)", m_fDeployed, HasConditions( bits_COND_HEAR_SOUND ) ? 1 : 0 );
		if( m_fDeployed )
		{
			if( HasConditions( bits_COND_HEAR_SOUND ) )
				return GetScheduleOfType( SCHED_ALERT_FACE );
			return GetScheduleOfType( SCHED_SPHERE_ANGRY_IDLE );
		}
		TREX_Log( "Sphere: deploy" );
		m_fDeployed = 1;
		return GetScheduleOfType( SCHED_SPHERE_DEPLOY );

	default:
		break;
	}
	return CBaseMonster::GetSchedule();
}

//=========================================================
// Spawn / Precache
//=========================================================
void CXenSphere::Spawn( void )
{
	m_fDeployed = 0;
	m_fShooting = 0;
	m_flNextShot = 0;
	m_flAtkStart = 0;
	m_fFiredThisAnim = 0;

	Precache();

	SET_MODEL( ENT( pev ), "models/sphere.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_TOSS;
	pev->health = 75;
	pev->effects = 0;
	m_flFieldOfView = -1.0f;
	m_bloodColor = BLOOD_COLOR_YELLOW;
	pev->view_ofs = Vector( 0, 0, 16 );
	m_MonsterState = MONSTERSTATE_NONE;

	SetUse( &CXenSphere::SphereUse );

	MonsterInit();

	TREX_Log( "sphere: spawned at %.0f %.0f %.0f health=%.0f", pev->origin.x, pev->origin.y, pev->origin.z, pev->health );
}

void CXenSphere::Precache( void )
{
	PRECACHE_MODEL( "models/sphere.mdl" );
	PRECACHE_MODEL( "models/laser.mdl" );
	m_iShrapnel = PRECACHE_MODEL( "models/shrapnel.mdl" );
	PRECACHE_SOUND( "debris/beamstart4.wav" );
	PRECACHE_SOUND( "weapons/electro4.wav" );
	PRECACHE_SOUND( "common/bodysplat.wav" );
}

void CXenSphere::SetYawSpeed( void )
{
	pev->yaw_speed = 100;
}

Vector CXenSphere::GetGunPosition( void )
{
	UTIL_MakeVectors( pev->angles );
	return pev->origin + Vector( 0, 0, 16 ) + gpGlobals->v_forward * 16;
}

//=========================================================
// Looks only while deployed
//=========================================================
void CXenSphere::Look( int iDistance )
{
	if( m_fDeployed )
		CBaseMonster::Look( iDistance );
}

BOOL CXenSphere::CheckRangeAttack1( float flDot, float flDist )
{
	if( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
		return FALSE;
	if( flDist > 2048 || flDot < 0.5f )
		return FALSE;
	if( gpGlobals->time < m_flNextShot )
		return FALSE;
	if( m_hEnemy == 0 )
		return FALSE;

	TraceResult tr;
	Vector vecGun = GetGunPosition();
	UTIL_TraceLine( vecGun, m_hEnemy->BodyTarget( vecGun ), ignore_monsters, edict(), &tr );
	if( tr.flFraction != 1.0f )
		return FALSE;

	m_fShooting = 1;
	return TRUE;
}

//=========================================================
// Attack
//=========================================================
void CXenSphere::FireLaser( void )
{
	if( m_hEnemy == 0 )
		return;

	Vector vecGun = GetGunPosition();
	Vector vecDir = ( m_hEnemy->BodyTarget( vecGun ) - vecGun );
	if( vecDir.Length() < 1.0f )
		return;
	vecDir = vecDir.Normalize();

	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "debris/beamstart4.wav", 1.0, 0.8, 0, 100 );
	CXenLaser::Shoot( pev, vecGun, vecDir * SPHERE_LASER_SPEED );

	m_fShooting = 0;
	m_flNextShot = gpGlobals->time + 0.5f;
	m_fFiredThisAnim = 1;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_DLIGHT );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z + 16 );
		WRITE_BYTE( 20 );	// radius
		WRITE_BYTE( 255 );
		WRITE_BYTE( 48 );
		WRITE_BYTE( 192 );
		WRITE_BYTE( 6 );	// life
		WRITE_BYTE( 6 );	// decay
	MESSAGE_END();
}

void CXenSphere::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case SPHERE_AE_FIRE:
		TREX_Log( "sphere: fire event (enemy=%s)", m_hEnemy != 0 ? STRING( m_hEnemy->pev->classname ) : "none" );
		FireLaser();
		break;
	default:
		TREX_Log( "sphere: anim event %d", pEvent->event );
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Damage / death / use
//=========================================================
int CXenSphere::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if( m_Activity == ACT_IDLE_ANGRY )
		flDamage *= 0.6f;

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CXenSphere::Killed( entvars_t *pevAttacker, int iGib )
{
	TREX_Log( "sphere: killed" );

	HitProxy_Remove( m_hProxy );

	// burst into shrapnel
	Vector vecSpot = pev->origin + Vector( 0, 0, 8 );

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
		WRITE_BYTE( TE_BREAKMODEL );
		WRITE_COORD( vecSpot.x );
		WRITE_COORD( vecSpot.y );
		WRITE_COORD( vecSpot.z );
		WRITE_COORD( 32 );
		WRITE_COORD( 32 );
		WRITE_COORD( 32 );
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
		WRITE_COORD( 200 );
		WRITE_BYTE( 20 );
		WRITE_SHORT( m_iShrapnel );
		WRITE_BYTE( 12 );
		WRITE_BYTE( 25 );
		WRITE_BYTE( BREAK_METAL );
	MESSAGE_END();

	for( int i = 0; i < 4; i++ )
		UTIL_BloodStream( vecSpot, UTIL_RandomBloodVector(), m_bloodColor, 100 );

	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM );

	pev->takedamage = DAMAGE_NO;
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1f;

	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CXenSphere::UpdateOnRemove( void )
{
	HitProxy_Remove( m_hProxy );
	CBaseMonster::UpdateOnRemove();
}

void CXenSphere::SphereUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int fWant = m_fDeployed;

	switch( useType )
	{
	case USE_ON:
		fWant = 1;
		break;
	case USE_OFF:
		fWant = 0;
		break;
	case USE_TOGGLE:
		fWant = !m_fDeployed;
		break;
	default:
		return;
	}

	TREX_Log( "sphere: use, deployed %d -> %d", m_fDeployed, fWant );

	if( fWant == m_fDeployed )
		return;

	if( fWant )
	{
		m_IdealMonsterState = MONSTERSTATE_ALERT;
		SetConditions( bits_COND_HEAR_SOUND );
	}
	else
	{
		ClearSchedule();
		m_IdealMonsterState = MONSTERSTATE_IDLE;
	}
}

void CXenSphere::MonsterThink( void )
{
	CBaseMonster::MonsterThink();

	// fallback: the attack animation ended up without firing (event not delivered)
	if( m_Activity == ACT_RANGE_ATTACK1 )
	{
		if( m_flAtkStart == 0 )
		{
			m_flAtkStart = gpGlobals->time;
			m_fFiredThisAnim = 0;
		}
		else if( !m_fFiredThisAnim && gpGlobals->time - m_flAtkStart > SPHERE_LOOK_FALLBACK )
		{
			TREX_Log( "sphere: fire fallback (no anim event)" );
			FireLaser();
		}
	}
	else
	{
		m_flAtkStart = 0;
	}

	HitProxy_Sync( this, m_hProxy );
}
