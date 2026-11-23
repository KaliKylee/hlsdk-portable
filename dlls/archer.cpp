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
// archer - ThrillEX swimming monster (leech-style movement + electric zap beams)
// Reconstructed from the decompiled thrill/hl.dll. Parts marked TODO(verify) are guesses.
//=========================================================
//
// UNDONE:
// DONE:Steering force model for attack
// DONE:Attack animation control / damage
// DONE:Establish range of up/down motion and steer around vertical obstacles
// DONE:Re-evaluate height periodically
// DONE:Fall (MOVETYPE_TOSS) and play different anim if out of water
// Test in complex room (c2a3?)
// DONE:Sounds? - Kelly will fix
// Blood cloud? Hurt effect?
// Group behavior?
// DONE:Save/restore
// Flop animation - just bind to ACT_TWITCH
// Fix fatal push into wall case
//
// Try this on a bird
// Try this on a model with hulls/tracehull?
//

#include	"float.h"
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"effects.h"
#include	"decals.h"
#include	"weapons.h"

// Animation events
#define ARCHER_AE_ATTACK		1
#define ARCHER_AE_FLOP		2
#define ARCHER_AE_ZAP		3
#define ARCHER_MAX_BEAMS	2

// Movement constants

#define ARCHER_ACCELERATE	10
#define ARCHER_CHECK_DIST	45
#define ARCHER_SWIM_SPEED	50.0f
#define ARCHER_SWIM_ACCEL	80.0f
#define ARCHER_SWIM_DECEL	10.0f
#define ARCHER_TURN_RATE		90
#define ARCHER_SIZEX		10
#define ARCHER_FRAMETIME		0.1f

#define DEBUG_BEAMS		0

#if DEBUG_BEAMS
#include "effects.h"
#endif

class CArcher : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );

	void EXPORT SwimThink( void );
	void EXPORT DeadThink( void );
	void Touch( CBaseEntity *pOther )
	{
		if( pOther->IsPlayer() )
		{
			// If the client is pushing me, give me some base velocity
			if( gpGlobals->trace_ent && gpGlobals->trace_ent == edict() )
			{
				pev->basevelocity = pOther->pev->velocity;
				pev->flags |= FL_BASEVELOCITY;
			}
		}
	}

	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector( -32, -12, -11 );
		pev->absmax = pev->origin + Vector( 32, 12, 11 );
	}

	void AttackSound( void );
	void AlertSound( void );
	void UpdateMotion( void );
	float ObstacleDistance( CBaseEntity *pTarget );
	void MakeVectors( void );
	void RecalculateWaterlevel( void );
	void ArmBeam( void );
	void ZapOff( void );
	void SwitchArcherState( void );
	
	// Base entity functions
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int BloodColor( void ) { return DONT_BLEED; }
	void Killed( entvars_t *pevAttacker, int iGib );
	void Activate( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	int Classify( void ) { return CLASS_INSECT; }
	int IRelationship( CBaseEntity *pTarget );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	static const char *pAttackSounds[];
	static const char *pAlertSounds[];

private:
	// UNDONE: Remove unused boid vars, do group behavior
	float m_flTurning;// is this boid turning?
	BOOL m_fPathBlocked;// TRUE if there is an obstacle ahead
	float m_flAccelerate;
	float m_obstacle;
	float m_top;
	float m_bottom;
	float m_height;
	float m_waterTime;
	float m_sideTime;		// Timer to randomly check clearance on sides
	float m_zTime;
	float m_stateTime;
	float m_attackSoundTime;
	CBeam *m_pBeam[ARCHER_MAX_BEAMS];
	int m_iBeams;
	float m_zapEndTime;

#if DEBUG_BEAMS
	CBeam *m_pb;
	CBeam *m_pt;
#endif
};

LINK_ENTITY_TO_CLASS( monster_archer, CArcher )
LINK_ENTITY_TO_CLASS( monster_archerfish, CArcher )

TYPEDESCRIPTION	CArcher::m_SaveData[] =
{
	DEFINE_FIELD( CArcher, m_flTurning, FIELD_FLOAT ),
	DEFINE_FIELD( CArcher, m_fPathBlocked, FIELD_BOOLEAN ),
	DEFINE_FIELD( CArcher, m_flAccelerate, FIELD_FLOAT ),
	DEFINE_FIELD( CArcher, m_obstacle, FIELD_FLOAT ),
	DEFINE_FIELD( CArcher, m_top, FIELD_FLOAT ),
	DEFINE_FIELD( CArcher, m_bottom, FIELD_FLOAT ),
	DEFINE_FIELD( CArcher, m_height, FIELD_FLOAT ),
	DEFINE_FIELD( CArcher, m_waterTime, FIELD_TIME ),
	DEFINE_FIELD( CArcher, m_sideTime, FIELD_TIME ),
	DEFINE_FIELD( CArcher, m_zTime, FIELD_TIME ),
	DEFINE_FIELD( CArcher, m_stateTime, FIELD_TIME ),
	DEFINE_FIELD( CArcher, m_attackSoundTime, FIELD_TIME ),
	DEFINE_ARRAY( CArcher, m_pBeam, FIELD_CLASSPTR, ARCHER_MAX_BEAMS ),
	DEFINE_FIELD( CArcher, m_iBeams, FIELD_INTEGER ),
	DEFINE_FIELD( CArcher, m_zapEndTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CArcher, CBaseMonster )

const char *CArcher::pAttackSounds[] =
{
	"snapbug/catch.wav",
};

const char *CArcher::pAlertSounds[] =
{
	"leech/leech_alert1.wav",
	"leech/leech_alert2.wav",
};

void CArcher::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/archer.mdl" );
	// Just for fun
	//	SET_MODEL( ENT( pev ), "models/icky.mdl" );
	
	//UTIL_SetSize( pev, g_vecZero, g_vecZero );
	UTIL_SetSize( pev, Vector( -12, -12, 0 ), Vector( 12, 12, 12 ) );
	// Don't push the minz down too much or the water check will fail because this entity is really point-sized
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLY;
	SetBits( pev->flags, FL_SWIM );
	pev->health = gSkillData.archerHealth;

	m_flFieldOfView = -1.0f;	// full FOV
	m_flDistLook = 720;
	MonsterInit();
	SetThink( &CArcher::SwimThink );
	SetUse( NULL );
	SetTouch( NULL );
	pev->view_ofs = g_vecZero;

	m_flTurning = 0;
	m_fPathBlocked = FALSE;
	SetActivity( ACT_SWIM );
	SetState( MONSTERSTATE_IDLE );
	m_stateTime = gpGlobals->time + RANDOM_FLOAT( 1, 5 );
	m_iBeams = 0;
	m_zapEndTime = 0;
	for( int i = 0; i < ARCHER_MAX_BEAMS; i++ )
		m_pBeam[i] = NULL;
}

void CArcher::Activate( void )
{
	RecalculateWaterlevel();
}

void CArcher::RecalculateWaterlevel( void )
{
	// Calculate boundaries
	Vector vecTest = pev->origin - Vector( 0, 0, 400 );

	TraceResult tr;

	UTIL_TraceLine( pev->origin, vecTest, missile, edict(), &tr );
	if( tr.flFraction != 1.0f )
		m_bottom = tr.vecEndPos.z + 1.0f;
	else
		m_bottom = vecTest.z;

	m_top = UTIL_WaterLevel( pev->origin, pev->origin.z, pev->origin.z + 400 ) - 1;

	// Chop off 20% of the outside range
	float newBottom = m_bottom * 0.8f + m_top * 0.2f;
	m_top = m_bottom * 0.2f + m_top * 0.8f;
	m_bottom = newBottom;
	m_height = RANDOM_FLOAT( m_bottom, m_top );
	m_waterTime = gpGlobals->time + RANDOM_FLOAT( 5, 7 );
}

void CArcher::SwitchArcherState( void )
{
	m_stateTime = gpGlobals->time + RANDOM_FLOAT( 3, 6 );
	if( m_MonsterState == MONSTERSTATE_COMBAT )
	{
		m_hEnemy = NULL;
		SetState( MONSTERSTATE_IDLE );
		// We may be up against the player, so redo the side checks
		m_sideTime = 0;
	}
	else
	{
		Look( m_flDistLook );
		CBaseEntity *pEnemy = BestVisibleEnemy();
		if( pEnemy && pEnemy->pev->waterlevel != 0 )
		{
			m_hEnemy = pEnemy;
			SetState( MONSTERSTATE_COMBAT );
			m_stateTime = gpGlobals->time + RANDOM_FLOAT( 18, 25 );
			AlertSound();
		}
	}
}

int CArcher::IRelationship( CBaseEntity *pTarget )
{
	if( pTarget->IsPlayer() )
		return R_DL;
	return CBaseMonster::IRelationship( pTarget );
}

void CArcher::AttackSound( void )
{
	if( gpGlobals->time > m_attackSoundTime )
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAttackSounds ), 1.0f, 0.8f, 0, PITCH_NORM );
		m_attackSoundTime = gpGlobals->time + 0.5f;
	}
}

void CArcher::AlertSound( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAlertSounds ), 1.0f, 0.35f, 0, PITCH_NORM );
}

void CArcher::Precache( void )
{
	//PRECACHE_MODEL( "models/icky.mdl" );
	PRECACHE_MODEL( "models/archer.mdl" );
	PRECACHE_MODEL( "sprites/lgtning.spr" );
	PRECACHE_SOUND( "debris/zap1.wav" );
	PRECACHE_SOUND( "debris/zap4.wav" );
	PRECACHE_SOUND( "weapons/electro4.wav" );

	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
}

int CArcher::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	pev->velocity = g_vecZero;

	// Nudge the leech away from the damage
	if( pevInflictor )
	{
		pev->velocity = ( pev->origin - pevInflictor->origin ).Normalize() * 25.0f;
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CArcher::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case ARCHER_AE_ATTACK:
		AttackSound();
		CBaseEntity *pEnemy;

		pEnemy = m_hEnemy;
		if( pEnemy != NULL )
		{
			Vector dir, face;

			UTIL_MakeVectorsPrivate( pev->angles, face, NULL, NULL );
			face.z = 0;
			dir = (pEnemy->pev->origin - pev->origin);
			dir.z = 0;
			dir = dir.Normalize();
			face = face.Normalize();

			if( DotProduct( dir, face ) > 0.9f )	// Only take damage if the leech is facing the prey
				pEnemy->TakeDamage( pev, pev, gSkillData.archerDmgBite, DMG_SLASH );
		}
		m_stateTime -= 2;
		break;
	case ARCHER_AE_FLOP:
		// Play flop sound
		break;
	case ARCHER_AE_ZAP:
		{
			ZapOff();
			ClearMultiDamage();
			UTIL_MakeVectors( pev->angles );
			ArmBeam();
			ArmBeam();
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_DLIGHT );
				WRITE_COORD( pev->origin.x + 24 );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z + 16 );
				WRITE_BYTE( 20 );	// radius
				WRITE_BYTE( 32 );	// r
				WRITE_BYTE( 96 );	// g
				WRITE_BYTE( 255 );	// b
				WRITE_BYTE( 10 );	// life
				WRITE_BYTE( 8 );	// decay
			MESSAGE_END();
			ApplyMultiDamage( pev, pev );
			m_flNextAttack = gpGlobals->time + RANDOM_FLOAT( gSkillData.archerMinBeamDelay, gSkillData.archerMinBeamDelay + 5.0f );
			m_zapEndTime = gpGlobals->time + 0.4f;
		}
		break;
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

void CArcher::MakeVectors( void )
{
	Vector tmp = pev->angles;
	tmp.x = -tmp.x;
	UTIL_MakeVectors( tmp );
}

//
// ObstacleDistance - returns normalized distance to obstacle
//
float CArcher::ObstacleDistance( CBaseEntity *pTarget )
{
	TraceResult tr;
	Vector vecTest;

	// use VELOCITY, not angles, not all boids point the direction they are flying
	//Vector vecDir = UTIL_VecToAngles( pev->velocity );
	MakeVectors();

	// check for obstacle ahead
	vecTest = pev->origin + gpGlobals->v_forward * ARCHER_CHECK_DIST;
	UTIL_TraceLine( pev->origin, vecTest, missile, edict(), &tr );

	if( tr.fStartSolid )
	{
		pev->speed = -ARCHER_SWIM_SPEED * 0.5f;
		//ALERT( at_console, "Stuck from (%f %f %f) to (%f %f %f)\n", pev->oldorigin.x, pev->oldorigin.y, pev->oldorigin.z, pev->origin.x, pev->origin.y, pev->origin.z );
		//UTIL_SetOrigin( pev, pev->oldorigin );
	}

	if( tr.flFraction != 1.0f )
	{
		if( ( pTarget == NULL || tr.pHit != pTarget->edict() ) )
		{
			return tr.flFraction;
		}
		else
		{
			if( fabs( m_height - pev->origin.z ) > 10 )
				return tr.flFraction;
		}
	}

	if( m_sideTime < gpGlobals->time )
	{
		// extra wide checks
		vecTest = pev->origin + gpGlobals->v_right * ARCHER_SIZEX * 2 + gpGlobals->v_forward * ARCHER_CHECK_DIST;
		UTIL_TraceLine( pev->origin, vecTest, missile, edict(), &tr );
		if( tr.flFraction != 1.0f )
			return tr.flFraction;

		vecTest = pev->origin - gpGlobals->v_right * ARCHER_SIZEX * 2 + gpGlobals->v_forward * ARCHER_CHECK_DIST;
		UTIL_TraceLine( pev->origin, vecTest, missile, edict(), &tr );
		if( tr.flFraction != 1.0f )
			return tr.flFraction;

		// Didn't hit either side, so stop testing for another 0.5 - 1 seconds
		m_sideTime = gpGlobals->time + RANDOM_FLOAT( 0.5f, 1.0f );
	}
	return 1.0;
}

void CArcher::DeadThink( void )
{
	if( m_fSequenceFinished )
	{
		if( m_Activity == ACT_DIEFORWARD )
		{
			SetThink( NULL );
			StopAnimation();
			return;
		}
		else if( pev->flags & FL_ONGROUND )
		{
			pev->solid = SOLID_NOT;
			SetActivity(ACT_DIEFORWARD);
		}
	}
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;

	// Apply damage velocity, but keep out of the walls
	if( pev->velocity.x != 0 || pev->velocity.y != 0 )
	{
		TraceResult tr;

		// Look 0.5 seconds ahead
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 0.5f, missile, edict(), &tr );
		if( tr.flFraction != 1.0f )
		{
			pev->velocity.x = 0.0f;
			pev->velocity.y = 0.0f;
		}
	}
}

void CArcher::UpdateMotion( void )
{
	float flapspeed = ( pev->speed - m_flAccelerate ) / ARCHER_ACCELERATE;
	m_flAccelerate = m_flAccelerate * 0.8f + pev->speed * 0.2f;

	if( flapspeed < 0 )
		flapspeed = -flapspeed;
	flapspeed += 1.0f;
	if( flapspeed < 0.5f )
		flapspeed = 0.5f;
	if( flapspeed > 1.9f )
		flapspeed = 1.9f;

	pev->framerate = flapspeed;

	if( !m_fPathBlocked )
		pev->avelocity.y = pev->ideal_yaw;
	else
		pev->avelocity.y = pev->ideal_yaw * m_obstacle;

	if( pev->avelocity.y > 150.0f )
		m_IdealActivity = ACT_TURN_LEFT;
	else if( pev->avelocity.y < -150.0f )
		m_IdealActivity = ACT_TURN_RIGHT;
	else
		m_IdealActivity = ACT_SWIM;

	// lean
	float targetPitch, delta;
	delta = m_height - pev->origin.z;

	if( delta < -10 )
		targetPitch = -30;
	else if( delta > 10 )
		targetPitch = 30;
	else
		targetPitch = 0;

	pev->angles.x = UTIL_Approach( targetPitch, pev->angles.x, 60.0f * ARCHER_FRAMETIME );

	// bank
	pev->avelocity.z = -( pev->angles.z + ( pev->avelocity.y * 0.25f ) );

	if( m_MonsterState == MONSTERSTATE_COMBAT && HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
		m_IdealActivity = ACT_MELEE_ATTACK1;

	// Out of water check
	if( !pev->waterlevel )
	{
		pev->movetype = MOVETYPE_TOSS;
		m_IdealActivity = ACT_TWITCH;
		pev->velocity = g_vecZero;

		// Animation will intersect the floor if either of these is non-zero
		pev->angles.z = 0.0f;
		pev->angles.x = 0.0f;

		if( pev->framerate < 1.0f )
			pev->framerate = 1.0f;
	}
	else if( pev->movetype == MOVETYPE_TOSS )
	{
		pev->movetype = MOVETYPE_FLY;
		pev->flags &= ~FL_ONGROUND;
		RecalculateWaterlevel();
		m_waterTime = gpGlobals->time + 2;	// Recalc again soon, water may be rising
	}

	if( m_Activity != m_IdealActivity )
	{
		SetActivity( m_IdealActivity );
	}
	float flInterval = StudioFrameAdvance();
	DispatchAnimEvents( flInterval );
#if DEBUG_BEAMS
	if( !m_pb )
		m_pb = CBeam::BeamCreate( "sprites/laserbeam.spr", 5 );
	if( !m_pt )
		m_pt = CBeam::BeamCreate( "sprites/laserbeam.spr", 5 );
	m_pb->PointsInit( pev->origin, pev->origin + gpGlobals->v_forward * ARCHER_CHECK_DIST );
	m_pt->PointsInit( pev->origin, pev->origin - gpGlobals->v_right * ( pev->avelocity.y * 0.25f ) );
	if( m_fPathBlocked )
	{
		float color = m_obstacle * 30;
		if( m_obstacle == 1.0f )
			color = 0;
		if( color > 255 )
			color = 255;
		m_pb->SetColor( 255, (int)color, (int)color );
	}
	else
		m_pb->SetColor( 255, 255, 0 );
	m_pt->SetColor( 0, 0, 255 );
#endif
}

void CArcher::SwimThink( void )
{
	if( m_zapEndTime <= gpGlobals->time )
		ZapOff();

	TraceResult tr;
	float flLeftSide;
	float flRightSide;
	float targetSpeed;
	float targetYaw = 0;
	CBaseEntity *pTarget;

	if( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
	{
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 1.0f, 1.5f );
		pev->velocity = g_vecZero;
		return;
	}
	else
		pev->nextthink = gpGlobals->time + 0.1f;

	targetSpeed = ARCHER_SWIM_SPEED;

	if( m_waterTime < gpGlobals->time )
		RecalculateWaterlevel();

	if( m_stateTime < gpGlobals->time )
		SwitchArcherState();

	ClearConditions( bits_COND_CAN_MELEE_ATTACK1 );
	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		pTarget = m_hEnemy;
		if( !pTarget )
			SwitchArcherState();
		else
		{
			// Chase the enemy's eyes
			m_height = pTarget->pev->origin.z + pTarget->pev->view_ofs.z - 5;
			// Clip to viable water area
			if( m_height < m_bottom )
				m_height = m_bottom;
			else if( m_height > m_top )
				m_height = m_top;
			Vector location = pTarget->pev->origin - pev->origin;
			location.z += (pTarget->pev->view_ofs.z);
			if( location.Length() < 40 )
				SetConditions( bits_COND_CAN_MELEE_ATTACK1 );
			// Turn towards target ent
			targetYaw = UTIL_VecToYaw( location );

			targetYaw = UTIL_AngleDiff( targetYaw, UTIL_AngleMod( pev->angles.y ) );

			if( targetYaw < ( -ARCHER_TURN_RATE * 0.75f ) )
				targetYaw = ( -ARCHER_TURN_RATE * 0.75f );
			else if( targetYaw > ( ARCHER_TURN_RATE * 0.75f ) )
				targetYaw = ( ARCHER_TURN_RATE * 0.75f );
			else
				targetSpeed *= 2;
		}
		break;
	default:
		if( m_zTime < gpGlobals->time )
		{
			float newHeight = RANDOM_FLOAT( m_bottom, m_top );
			m_height = 0.5f * m_height + 0.5f * newHeight;
			m_zTime = gpGlobals->time + RANDOM_FLOAT( 1, 4 );
		}
		if( RANDOM_LONG( 0, 100 ) < 10 )
			targetYaw = RANDOM_LONG( -30, 30 );
		pTarget = NULL;

		// oldorigin test
		if( ( pev->origin - pev->oldorigin ).Length() < 1 )
		{
			// If leech didn't move, there must be something blocking it, so try to turn
			m_sideTime = 0;
		}
		break;
	}

	m_obstacle = ObstacleDistance( pTarget );
	pev->oldorigin = pev->origin;
	if( m_obstacle < 0.1f )
		m_obstacle = 0.1f;

	// is the way ahead clear?
	if( m_obstacle == 1.0f )
	{
		// if the leech is turning, stop the trend.
		if( m_flTurning != 0 )
		{
			m_flTurning = 0;
		}

		m_fPathBlocked = FALSE;
		pev->speed = UTIL_Approach( targetSpeed, pev->speed, ARCHER_SWIM_ACCEL * ARCHER_FRAMETIME );
		pev->velocity = gpGlobals->v_forward * pev->speed;

	}
	else
	{
		m_obstacle = 1.0f / m_obstacle;
		// IF we get this far in the function, the leader's path is blocked!
		m_fPathBlocked = TRUE;

		if( m_flTurning == 0 )// something in the way and leech is not already turning to avoid
		{
			Vector vecTest;
			// measure clearance on left and right to pick the best dir to turn
			vecTest = pev->origin + ( gpGlobals->v_right * ARCHER_SIZEX ) + ( gpGlobals->v_forward * ARCHER_CHECK_DIST );
			UTIL_TraceLine( pev->origin, vecTest, missile, edict(), &tr );
			flRightSide = tr.flFraction;

			vecTest = pev->origin + ( gpGlobals->v_right * -ARCHER_SIZEX ) + ( gpGlobals->v_forward * ARCHER_CHECK_DIST );
			UTIL_TraceLine( pev->origin, vecTest, missile, edict(), &tr );
			flLeftSide = tr.flFraction;

			// turn left, right or random depending on clearance ratio
			float delta = ( flRightSide - flLeftSide );
			if( delta > 0.1f || ( delta > -0.1f && RANDOM_LONG( 0, 100 ) < 50 ) )
				m_flTurning = -ARCHER_TURN_RATE;
			else
				m_flTurning = ARCHER_TURN_RATE;
		}
		pev->speed = UTIL_Approach( -( ARCHER_SWIM_SPEED * 0.5f ), pev->speed, ARCHER_SWIM_DECEL * ARCHER_FRAMETIME * m_obstacle );
		pev->velocity = gpGlobals->v_forward * pev->speed;
	}
	pev->ideal_yaw = m_flTurning + targetYaw;
	UpdateMotion();
}

void CArcher::Killed( entvars_t *pevAttacker, int iGib )
{
	Vector vecSplatDir;
	TraceResult tr;

	ZapOff();

	if( ShouldGibMonster( iGib ) )
	{
		CallGibMonster();
		return;
	}

	//ALERT(at_aiconsole, "Leech: killed\n");
	// tell owner ( if any ) that we're dead.This is mostly for MonsterMaker functionality.
	CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
	if( pOwner )
		pOwner->DeathNotice( pev );

	// When we hit the ground, play the "death_end" activity
	if( pev->waterlevel )
	{
		pev->angles.z = 0;
		pev->angles.x = 0;
		pev->origin.z += 1;
		pev->avelocity = g_vecZero;
		if( RANDOM_LONG( 0, 99 ) < 70 )
			pev->avelocity.y = RANDOM_LONG( -720, 720 );

		pev->gravity = 0.02;
		ClearBits( pev->flags, FL_ONGROUND );
		SetActivity( ACT_DIESIMPLE );
	}
	else
		SetActivity( ACT_DIEFORWARD );
	
	pev->movetype = MOVETYPE_TOSS;
	pev->takedamage = DAMAGE_NO;
	SetThink( &CArcher::DeadThink );
}

//=========================================================
// ZapOff - remove the zap beams
//=========================================================
void CArcher::ZapOff( void )
{
	for( int i = 0; i < ARCHER_MAX_BEAMS; i++ )
	{
		if( m_pBeam[i] )
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;
	pev->skin = 0;
	STOP_SOUND( ENT( pev ), CHAN_WEAPON, "debris/zap4.wav" );
}

//=========================================================
// ArmBeam - beam from the attachment towards the enemy
//=========================================================
void CArcher::ArmBeam( void )
{
	if( m_iBeams >= ARCHER_MAX_BEAMS )
		return;

	CBaseEntity *pEnemy = m_hEnemy;
	if( !pEnemy )
		return;

	Vector vecSrc, vecAngles;
	GetAttachment( 0, vecSrc, vecAngles );

	Vector vecAim = ( pEnemy->pev->origin - vecSrc ).Normalize();
	TraceResult tr;
	UTIL_TraceLine( vecSrc, vecSrc + vecAim * 1024, dont_ignore_monsters, ENT( pev ), &tr );

	CBeam *pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 50 );
	m_pBeam[m_iBeams] = pBeam;
	if( !pBeam )
		return;

	pBeam->PointEntInit( tr.vecEndPos, entindex() );
	pBeam->SetColor( 32, 96, 255 );
	pBeam->SetBrightness( 255 );
	pBeam->SetNoise( 20 );
	pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY; // TODO(verify) flag cleared by decompile via &= 0xFFF
	m_iBeams++;

	CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
	if( pHit && pHit->pev->takedamage )
		pHit->TraceAttack( pev, gSkillData.archerDmgZap, vecAim, &tr, DMG_SHOCK );

	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "weapons/electro4.wav", 0.5f, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
}
