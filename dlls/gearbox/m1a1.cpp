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
// monster template
//=========================================================
// UNDONE: Holster weapon?

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"explode.h"
#include	"func_break.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		BARNEY_AE_DRAW		( 2 )
#define		BARNEY_AE_SHOOT		( 3 )
#define		BARNEY_AE_HOLSTER	( 4 )

#define	BARNEY_BODY_GUNHOLSTERED	0
#define	BARNEY_BODY_GUNDRAWN		1
#define BARNEY_BODY_GUNGONE			2

#define M1A1_DAMAGE	(DMG_ENERGYBEAM|DMG_CRUSH|DMG_MORTAR|DMG_BLAST)
#define M1A1_GIB_MODEL "models/metalplategibs.mdl"

int gM1A1GibModel = 0;
short g_sModelIndexBlastCircle;
short m_sNuclearSprite;

class CM1A1 : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  ISoundMask( void );
	void BarneyFirePistol( void );
	void AlertSound( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	
	void RunTask( Task_t *pTask );
	void PrescheduleThink( void );
	void IdleSoundUpdate( void );
	void HealthBarUpdate( void );
	void StartTask( Task_t *pTask );
	virtual int	ObjectCaps( void ) { return CBaseMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	BOOL CheckRangeAttack1 ( float flDot, float flDist );	

	void DeclineFollowing( void );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	void DeathSound( void );
	void PainSound( void );

	void TalkInit( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed( entvars_t *pevAttacker, int iGib );
	
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL	m_fGunDrawn;
	float	m_painTime;
	float	m_checkAttackTime;
	float	m_shakeTime;
	BOOL	m_lastAttackCheck;
	float	m_painSoundTime;

	// UNDONE: What is this for?  It isn't used?
	float	m_flPlayerDamage;// how much pain has the player inflicted on me?

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_m1a1, CM1A1 );

TYPEDESCRIPTION	CM1A1::m_SaveData[] = 
{
	DEFINE_FIELD( CM1A1, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CM1A1, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CM1A1, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CM1A1, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CM1A1, m_flPlayerDamage, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CM1A1, CBaseMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlM1A1Follow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slM1A1Follow[] =
{
	{
		tlM1A1Follow,
		ARRAYSIZE ( tlM1A1Follow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

//=========================================================
// BarneyDraw- much better looking draw schedule for when
// barney knows who he's gonna attack.
//=========================================================
Task_t	tlM1A1EnemyDraw[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_ARM },
};

Schedule_t slM1A1EnemyDraw[] = 
{
	{
		tlM1A1EnemyDraw,
		ARRAYSIZE ( tlM1A1EnemyDraw ),
		0,
		0,
		"Barney Enemy Draw"
	}
};

Task_t	tlM1A1FaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slM1A1FaceTarget[] =
{
	{
		tlM1A1FaceTarget,
		ARRAYSIZE ( tlM1A1FaceTarget ),
		bits_COND_CLIENT_PUSH	|
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlIdleM1A1Stand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleM1A1Stand[] =
{
	{ 
		tlIdleM1A1Stand,
		ARRAYSIZE ( tlIdleM1A1Stand ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|
		
		bits_SOUND_DANGER		|
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};

DEFINE_CUSTOM_SCHEDULES( CM1A1 )
{
	slM1A1Follow,
	slM1A1EnemyDraw,
	slM1A1FaceTarget,
	slIdleM1A1Stand,
};


IMPLEMENT_CUSTOM_SCHEDULES( CM1A1, CBaseMonster );

void CM1A1 :: StartTask( Task_t *pTask )
{
	CBaseMonster::StartTask( pTask );	
}

void CM1A1 :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if (m_hEnemy != NULL && (m_hEnemy->IsPlayer()))
		{
			pev->framerate = 1.5;
		}
		CBaseMonster::RunTask( pTask );
		break;
	default:
		CBaseMonster::RunTask( pTask );
		break;
	}
}




//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CM1A1 :: ISoundMask ( void) 
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CM1A1 :: Classify ( void )
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CM1A1 :: AlertSound( void )
{

}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CM1A1 :: SetYawSpeed ( void )
{
	int ys;

	ys = 0;

	switch ( m_Activity )
	{
	case ACT_IDLE:		
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CM1A1 :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 1024 && flDot >= 0.5 )
	{
		if ( gpGlobals->time > m_checkAttackTime )
		{
			TraceResult tr;
			
			Vector shootOrigin = pev->origin + Vector( 0, 0, 55 );
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
			UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr );
			m_checkAttackTime = gpGlobals->time + 1;
			if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) )
				m_lastAttackCheck = TRUE;
			else
				m_lastAttackCheck = FALSE;
			m_checkAttackTime = gpGlobals->time + 1.5;
		}
		return m_lastAttackCheck;
	}
	return FALSE;
}


//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
void CM1A1 :: BarneyFirePistol ( void )
{
	Vector vecShootOrigin;
	TraceResult		tr;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0,0,0);
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );
	Vector angDir = UTIL_VecToAngles( vecShootDir );

	FireBulletsM1A1(1, vecShootOrigin, vecShootDir, VECTOR_CONE_1DEGREES, 8024, BULLET_MONSTER_M1A1 );

    GetAttachment( 0, vecShootOrigin, angDir );

		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecShootOrigin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( vecShootOrigin.x );
			WRITE_COORD( vecShootOrigin.y );
			WRITE_COORD( vecShootOrigin.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 20 ); // scale * 10
			WRITE_BYTE( 12  ); // framerate
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecShootOrigin );
			WRITE_BYTE(TE_DLIGHT);
			WRITE_COORD(vecShootOrigin.x);	// X
			WRITE_COORD(vecShootOrigin.y);	// Y
			WRITE_COORD(vecShootOrigin.z);	// Z
			WRITE_BYTE( 20 );		// radius * 0.1
			WRITE_BYTE( 255 );		// r
			WRITE_BYTE( 255 );		// g
			WRITE_BYTE( 255 );		// b
			WRITE_BYTE( 255 );		// brightness 
			WRITE_BYTE( 20 );		// time * 10
	    	WRITE_BYTE( 1 );        // decay * 0.1
	    MESSAGE_END( );

		MESSAGE_BEGIN(MSG_BROADCAST ,SVC_TEMPENTITY);
	    WRITE_BYTE(TE_EXPLOSION);
		WRITE_COORD(vecShootOrigin.x);	// X
		WRITE_COORD(vecShootOrigin.y);	// Y
		WRITE_COORD(vecShootOrigin.z);	// Z
	    WRITE_SHORT(PRECACHE_MODEL("sprites/bexplo.spr"));	// sprite index
	    WRITE_BYTE(10);	// scale in 0.1's
	    WRITE_BYTE(30);	// framerate
	    WRITE_BYTE(TE_EXPLFLAG_NODLIGHTS | TE_EXPLFLAG_NOPARTICLES | TE_EXPLFLAG_NOSOUND);	// flags
	    MESSAGE_END( );

		vec3_t	vecPlaneNormal;
	// draw sparks
	if ( !( pev->spawnflags & SF_ENVEXPLOSION_NOSPARKS ) )
	{
		int sparkCount = RANDOM_LONG(0,3);

		for ( int i = 0; i < sparkCount; i++ )
		{
			Create( "spark_shower", vecShootOrigin, vecPlaneNormal, NULL );
		}
	}

	int pitchShift = RANDOM_LONG( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "tank/tir.wav", 1, 0.1, 0, 100 + pitchShift );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CM1A1 :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BARNEY_AE_SHOOT:
		BarneyFirePistol();
		break;

	case BARNEY_AE_DRAW:
		// barney's bodygroup switches here so he can pull gun from holster
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "ambience/loader_step1.wav", 1, ATTN_NORM, 0, PITCH_NORM);
		pev->body = BARNEY_BODY_GUNDRAWN;
		m_fGunDrawn = TRUE;
		break;

	case BARNEY_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		pev->body = BARNEY_BODY_GUNHOLSTERED;
		m_fGunDrawn = FALSE;
		break;

	default:
		CBaseMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CM1A1 :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/m1a1.mdl");
	UTIL_SetSize(pev, Vector( -152, -62, 0 ), Vector( 152, 62, 100) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= -1;
	pev->health			= gSkillData.TankHealth;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	pev->body			= 0; // gun in holster
	m_fGunDrawn			= FALSE;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CM1A1 :: Precache()
{
	PRECACHE_MODEL("models/m1a1.mdl");

	PRECACHE_SOUND("barney/ba_attack1.wav" );
	PRECACHE_SOUND("barney/ba_attack2.wav" );

	PRECACHE_SOUND("barney/ba_pain1.wav");
	PRECACHE_SOUND("barney/ba_pain2.wav");
	PRECACHE_SOUND("barney/ba_pain3.wav");

	PRECACHE_SOUND("barney/ba_die1.wav");
	PRECACHE_SOUND("barney/ba_die2.wav");
	PRECACHE_SOUND("barney/ba_die3.wav");
	
	PRECACHE_SOUND("tank/accelere1.wav");
	PRECACHE_SOUND("tank/accelere2.wav");
	PRECACHE_SOUND("ambience/loader_step1.wav");
	PRECACHE_SOUND("ambience/tankidle2.wav");
	PRECACHE_SOUND("tank/dxmexplode.wav");
	PRECACHE_SOUND("tank/explode.wav");
	PRECACHE_SOUND("tank/tir.wav");
    PRECACHE_MODEL("sprites/steam1.spr");
    PRECACHE_MODEL("sprites/bexplo.spr");
    PRECACHE_MODEL( "sprites/fexplo.spr" );
    PRECACHE_MODEL( "sprites/white.spr" );
	
	m_sNuclearSprite = PRECACHE_MODEL("sprites/fexplo.spr");
    g_sModelIndexBlastCircle = PRECACHE_MODEL("sprites/white.spr");
	gM1A1GibModel = PRECACHE_MODEL( M1A1_GIB_MODEL );

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CBaseMonster::Precache();
}	

// Init talk data
void CM1A1 :: TalkInit()
{
}


void CM1A1 :: PrescheduleThink( void )
{
	IdleSoundUpdate();
    HealthBarUpdate();
}

void CM1A1 :: IdleSoundUpdate( void )
{
	if ( m_shakeTime < gpGlobals->time )
	{   
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "ambience/tankidle2.wav", 1, 0.5);
	m_shakeTime = gpGlobals->time + 10.0;
	}
}

extern int gmsgBossBarHealthView;
extern int gmsgBossBarHealthNameView;

void CM1A1 :: HealthBarUpdate( void )
{
	edict_t	*pentTarget = NULL;
	pentTarget = FIND_ENTITY_BY_STRING( pentTarget, "classname", "startbosshud" );
	if (FNullEnt(pentTarget))
	   return;

	MESSAGE_BEGIN( MSG_ONE, gmsgBossBarHealthView, NULL, UTIL_PlayerByIndex(1)->pev );
		WRITE_BYTE	( 1 );
		WRITE_COORD ( ENTINDEX ( edict() ) );
		WRITE_LONG ( pev->health );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgBossBarHealthNameView, NULL, UTIL_PlayerByIndex(1)->pev );
		WRITE_BYTE	( 1 );
		WRITE_COORD ( ENTINDEX ( edict() ) );
		WRITE_LONG ( 0 );
	MESSAGE_END(); 
}

static BOOL M1A1_IsFacing( entvars_t *pevTest, const Vector &reference )
{
	Vector vecDir = (reference - pevTest->origin);
	vecDir.z = 0;
	vecDir = vecDir.Normalize();
	Vector forward, angle;
	angle = pevTest->v_angle;
	angle.x = 0;
	UTIL_MakeVectorsPrivate( angle, forward, NULL, NULL );
	// He's facing me, he meant it
	if ( DotProduct( forward, vecDir ) > 0.96 )	// +/- 15 degrees or so
	{
		return TRUE;
	}
	return FALSE;
}

void StreakSplashM1A1( const Vector &origin, const Vector &direction, int color, int count, int speed, int velocityRange )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, origin );
		WRITE_BYTE( TE_STREAK_SPLASH );
		WRITE_COORD( origin.x );		// origin
		WRITE_COORD( origin.y );
		WRITE_COORD( origin.z );
		WRITE_COORD( direction.x );	// direction
		WRITE_COORD( direction.y );
		WRITE_COORD( direction.z );
		WRITE_BYTE( color );	// Streak color 6
		WRITE_SHORT( count );	// count
		WRITE_SHORT( speed );
		WRITE_SHORT( velocityRange );	// Random velocity modifier
	MESSAGE_END();
}

void CM1A1::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	ALERT( at_aiconsole, "CGargantua::TraceAttack\n");

	if ( !IsAlive() )
	{
		CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
		return;
	}

	bitsDamageType &= M1A1_DAMAGE;

	if ( bitsDamageType == 0)
	{
		if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20) )
		{
			UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT(0.5,1.5) );
			pev->dmgtime = gpGlobals->time;
//			if ( RANDOM_LONG(0,100) < 25 )
//				EMIT_SOUND_DYN( ENT(pev), CHAN_BODY, pRicSounds[ RANDOM_LONG(0,ARRAYSIZE(pRicSounds)-1) ], 1.0, ATTN_NORM, 0, PITCH_NORM );
		}
		flDamage = 0;
	return;
	}

	if ( pev->takedamage )
	{	
	    StreakSplashM1A1( ptr->vecEndPos, Vector(0,0,1), 6, 50, RANDOM_LONG(150,350), 500 );
		TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );

}

int CM1A1 :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if ( !(bitsDamageType & M1A1_DAMAGE) )
	    return 0;

	if ( !(bitsDamageType & DMG_BLAST) )
	    return 0;

	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	if ( m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) )
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( m_hEnemy == NULL )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ( (m_afMemory & bits_MEMORY_SUSPICIOUS) || M1A1_IsFacing( pevAttacker, pev->origin ) )
			{
				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				// Hey, be careful with that
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO )
		{
		}
	}



	return ret;
}

	
//=========================================================
// PainSound
//=========================================================
void CM1A1 :: PainSound ( void )
{
	if (gpGlobals->time < m_painTime)
		return;
	
	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "tank/dxmexplode.wav", 1, 0.1, 0, PITCH_NORM); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "tank/dxmexplode.wav", 1, 0.1, 0, PITCH_NORM); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "tank/dxmexplode.wav", 1, 0.1, 0, PITCH_NORM); break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CM1A1 :: DeathSound ( void )
{
	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "tank/explode.wav", 1, 0.1, 0, PITCH_NORM); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "tank/explode.wav", 1, 0.1, 0, PITCH_NORM); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "tank/explode.wav", 1, 0.1, 0, PITCH_NORM); break;
	}
}


void CM1A1::Killed( entvars_t *pevAttacker, int iGib )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgBossBarHealthView, NULL, UTIL_PlayerByIndex(1)->pev );
		WRITE_BYTE	( 0 );
		WRITE_COORD ( ENTINDEX ( edict() ) );
		WRITE_LONG ( pev->health );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgBossBarHealthNameView, NULL, UTIL_PlayerByIndex(1)->pev );
		WRITE_BYTE	( 0 );
		WRITE_COORD ( ENTINDEX ( edict() ) );
		WRITE_LONG ( 0 );
	MESSAGE_END();		
	
	entvars_t	*pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	::RadiusDamage( pev->origin, pev, pevOwner, 100, 1000, CLASS_NONE, DMG_TANKBLAST );			
	DeathSound();
	
			MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
				WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z + 200 );
				WRITE_SHORT( m_sNuclearSprite );
				WRITE_BYTE( 100  ); // scale * 10
				WRITE_BYTE( 10  ); // framerate
				WRITE_BYTE( TE_EXPLFLAG_NONE );
			MESSAGE_END();

			// cercles

			MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_BEAMCYLINDER );
				WRITE_COORD( pev->origin.x);
				WRITE_COORD( pev->origin.y);
				WRITE_COORD( pev->origin.z);
				WRITE_COORD( pev->origin.x);
				WRITE_COORD( pev->origin.y);
				WRITE_COORD( pev->origin.z + 600 * 5 ); // reach damage radius over .3 seconds
				WRITE_SHORT( g_sModelIndexBlastCircle );
				WRITE_BYTE( 0 ); // startframe
				WRITE_BYTE( 0 ); // framerate
				WRITE_BYTE( 3 ); // life
				WRITE_BYTE( 128 );  // width
				WRITE_BYTE( 0 );   // noise
				WRITE_BYTE( 255   );	// rgb
				WRITE_BYTE( 255 );
				WRITE_BYTE( 200  );
				WRITE_BYTE( 170 ); //brightness
				WRITE_BYTE( 0 );		// speed
			MESSAGE_END();
			MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_BEAMCYLINDER );
				WRITE_COORD( pev->origin.x);
				WRITE_COORD( pev->origin.y);
				WRITE_COORD( pev->origin.z + 32 );
				WRITE_COORD( pev->origin.x);
				WRITE_COORD( pev->origin.y);
				WRITE_COORD( pev->origin.z - 600 * 5 / 2 ); // reach damage radius over .3 seconds
				WRITE_SHORT( g_sModelIndexBlastCircle );
				WRITE_BYTE( 0 ); // startframe
				WRITE_BYTE( 0 ); // framerate
				WRITE_BYTE( 4 ); // life
				WRITE_BYTE( 48 );  // width
				WRITE_BYTE( 0 );   // noise
				WRITE_BYTE( 255   );	// rgb
				WRITE_BYTE( 255 );
				WRITE_BYTE( 200  );
				WRITE_BYTE( 170 ); //brightness
				WRITE_BYTE( 0 );		// speed
			MESSAGE_END();
	
	
	        int i;
			int parts = MODEL_FRAMES( gM1A1GibModel );
			for ( i = 0; i < 10; i++ )
			{
				CGib *pGib = GetClassPtr( (CGib *)NULL );

				pGib->Spawn( M1A1_GIB_MODEL );
				
				int bodyPart = 0;
				if ( parts > 1 )
					bodyPart = RANDOM_LONG( 0, pev->body-1 );

				pGib->pev->body = bodyPart;
				pGib->m_bloodColor = DONT_BLEED;
				pGib->m_material = matNone;
				pGib->pev->origin = pev->origin;
				pGib->pev->velocity = UTIL_RandomBloodVector() * RANDOM_FLOAT( 300, 500 );
				pGib->pev->nextthink = gpGlobals->time + 1.25;
				pGib->SetThink( &CBaseEntity::SUB_FadeOut );
			}
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_BREAKMODEL);

				// position
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );

				// size
				WRITE_COORD( 200 );
				WRITE_COORD( 200 );
				WRITE_COORD( 128 );

				// velocity
				WRITE_COORD( 0 ); 
				WRITE_COORD( 0 );
				WRITE_COORD( 0 );

				// randomization
				WRITE_BYTE( 200 ); 

				// Model
				WRITE_SHORT( gM1A1GibModel );	//model id#

				// # of shards
				WRITE_BYTE( 200 );

				// duration
				WRITE_BYTE( 100 );// 3.0 seconds

				// flags

				WRITE_BYTE( BREAK_METAL );
			MESSAGE_END();	
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_BREAKMODEL);

				// position
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );

				// size
				WRITE_COORD( 200 );
				WRITE_COORD( 200 );
				WRITE_COORD( 128 );

				// velocity
				WRITE_COORD( 0 ); 
				WRITE_COORD( 0 );
				WRITE_COORD( 0 );

				// randomization
				WRITE_BYTE( 200 ); 

				// Model
				WRITE_SHORT( gM1A1GibModel );	//model id#

				// # of shards
				WRITE_BYTE( 200 );

				// duration
				WRITE_BYTE( 100 );// 3.0 seconds

				// flags

				WRITE_BYTE( BREAK_METAL );
			MESSAGE_END();		
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_BREAKMODEL);

				// position
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );

				// size
				WRITE_COORD( 200 );
				WRITE_COORD( 200 );
				WRITE_COORD( 128 );

				// velocity
				WRITE_COORD( 0 ); 
				WRITE_COORD( 0 );
				WRITE_COORD( 0 );

				// randomization
				WRITE_BYTE( 200 ); 

				// Model
				WRITE_SHORT( gM1A1GibModel );	//model id#

				// # of shards
				WRITE_BYTE( 200 );

				// duration
				WRITE_BYTE( 100 );// 3.0 seconds

				// flags

				WRITE_BYTE( BREAK_METAL );
			MESSAGE_END();				
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_BREAKMODEL);

				// position
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );

				// size
				WRITE_COORD( 200 );
				WRITE_COORD( 200 );
				WRITE_COORD( 128 );

				// velocity
				WRITE_COORD( 0 ); 
				WRITE_COORD( 0 );
				WRITE_COORD( 0 );

				// randomization
				WRITE_BYTE( 200 ); 

				// Model
				WRITE_SHORT( gM1A1GibModel );	//model id#

				// # of shards
				WRITE_BYTE( 200 );

				// duration
				WRITE_BYTE( 100 );// 3.0 seconds

				// flags

				WRITE_BYTE( BREAK_METAL );
			MESSAGE_END();					

	SetUse( NULL );		
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
	UTIL_Remove( this );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CM1A1 :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slM1A1EnemyDraw;
		}
		break;

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used' 
		psched = CBaseMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slM1A1FaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slM1A1Follow;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CBaseMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleM1A1Stand;
		}
		else
			return psched;	
	}

	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CM1A1 :: GetSchedule ( void )
{
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			// always act surprized with a new enemy
			if ( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE) )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
				
			// wait for one schedule to draw gun
			if (!m_fGunDrawn )
				return GetScheduleOfType( SCHED_ARM_WEAPON );

			if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;

	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:
		if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		if ( HasConditions( bits_COND_CLIENT_PUSH ) )
		{
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		}
		break;
	}
	
	return CBaseMonster::GetSchedule();
}

MONSTERSTATE CM1A1 :: GetIdealState ( void )
{
	return CBaseMonster::GetIdealState();
}



void CM1A1::DeclineFollowing( void )
{
	PlaySentence( "BA_POK", 2, VOL_NORM, ATTN_NORM );
}