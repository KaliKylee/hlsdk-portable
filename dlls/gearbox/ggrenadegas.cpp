/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== generic grenade.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"


//===================grenade


LINK_ENTITY_TO_CLASS( grenadegas, CGasGrenade );

// Grenades flagged with this will be triggered when the owner calls detonateSatchelCharges
#define SF_DETONATE		0x0001

void CGasGrenade::Killed (entvars_t *pevAttacker, int iGib)
{
	UTIL_Remove( this );
}

void CGasGrenade::Explode( TraceResult *pTrace, int bitsDamageType )
{
	pev->velocity = g_vecZero;
	pev->origin = pev->origin + Vector(0,0,0);

	entvars_t *pevOwner = VARS( pev->owner );
	::RadiusDamage( pev->origin, pev, NULL, 20, 500, CLASS_MACHINE, DMG_NERVEGAS | DMG_POISON );

	GasStayTime = 80;
    
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/GASgrenade.wav", 1, ATTN_NORM);

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin  );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD( pev->origin.x );	// X
		WRITE_COORD( pev->origin.y );	// Y
		WRITE_COORD( pev->origin.z );	// Z
		WRITE_BYTE( 80 );		// radius * 0.1
		WRITE_BYTE( 255 );		// r
		WRITE_BYTE( 255 );		// g
		WRITE_BYTE( 255 );		// b
		WRITE_BYTE( 10 );		// time * 10
		WRITE_BYTE( 10 );		// decay * 0.1
	MESSAGE_END( );

    MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
    WRITE_BYTE( TE_SPRITE );
    WRITE_COORD( pev->origin.x );
    WRITE_COORD( pev->origin.y );
    WRITE_COORD( pev->origin.z );
    WRITE_SHORT( PRECACHE_MODEL( "sprites/ballsmoke.spr" ) );
    WRITE_BYTE( 80 );
    WRITE_BYTE( 100 );
    MESSAGE_END();	

	SetTouch(NULL);
	SetThink( &CGasGrenade::Smoke );
	pev->nextthink = gpGlobals->time + 0.2;
}


void CGasGrenade::Smoke( void )
{
	if(GasStayTime <=0 )
	{
		STOP_SOUND( edict(), CHAN_VOICE, "weapons/GASgrenade_explode.wav" );
		UTIL_Remove( this );
	}
	else
		GasStayTime--;

	entvars_t *pevOwner = VARS( pev->owner );
	if (UTIL_PointContents ( pev->origin ) != CONTENTS_WATER)
	{
		::RadiusDamage( pev->origin, pev, NULL, 10, 350, CLASS_MACHINE, DMG_NERVEGAS | DMG_POISON );
		
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin  );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD( pev->origin.x );	// X
		WRITE_COORD( pev->origin.y );	// Y
		WRITE_COORD( pev->origin.z );	// Z
		WRITE_BYTE( 30 );		// radius * 0.1
		WRITE_BYTE( 0 );		// r
		WRITE_BYTE( 255 );		// g
		WRITE_BYTE( 0 );		// b
		WRITE_BYTE( 80 );		// time * 10
		WRITE_BYTE( 10 );		// decay * 0.1
	MESSAGE_END( );

    MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
    WRITE_BYTE( TE_SPRITE );
    WRITE_COORD( pev->origin.x );
    WRITE_COORD( pev->origin.y );
    WRITE_COORD( pev->origin.z );
    WRITE_SHORT( PRECACHE_MODEL( "sprites/ef_smoke_poison.spr" ) );
    WRITE_BYTE( 50 );
    WRITE_BYTE( 20 );
    MESSAGE_END();		
		
		if ( gpGlobals->time >= m_flSoundTime )
		{
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/GASgrenade_explode.wav", 0.7, ATTN_NORM);
			m_flSoundTime = gpGlobals->time + 3;
		}
	}
	pev->nextthink = gpGlobals->time + 0.2;
}

// Timed grenade, this think is called when time runs out.
void CGasGrenade::DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CGasGrenade::Detonate );
	pev->nextthink = gpGlobals->time;
}

void CGasGrenade::PreDetonate( void )
{
	CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin, 400, 0.3 );

	SetThink( &CGasGrenade::Detonate );
	pev->nextthink = gpGlobals->time + 1;
}


void CGasGrenade::Detonate( void )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -40 ),  ignore_monsters, ENT(pev), & tr);

	Explode( &tr, DMG_BLAST );
}


//
// Contact grenade, explode when it touches something
// 
void CGasGrenade::ExplodeTouch( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT(pev), &tr );

	Explode( &tr, DMG_BLAST );
}


void CGasGrenade::DangerSoundThink( void )
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5, pev->velocity.Length( ), 0.2 );
	pev->nextthink = gpGlobals->time + 0.2;

	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
	}
}


void CGasGrenade::BounceTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if ( pOther->edict() == pev->owner )
		return;

	// only do damage if we're moving fairly fast
	if (m_flNextAttack < gpGlobals->time && pev->velocity.Length() > 100)
	{
		entvars_t *pevOwner = VARS( pev->owner );
		if (pevOwner)
		{
			TraceResult tr = UTIL_GetGlobalTrace( );
			ClearMultiDamage( );
			pOther->TraceAttack(pevOwner, 1, gpGlobals->v_forward, &tr, DMG_CLUB ); 
			ApplyMultiDamage( pev, pevOwner);
		}
		m_flNextAttack = gpGlobals->time + 1.0; // debounce
	}

	Vector vecTestVelocity;
	// pev->avelocity = Vector (300, 300, 300);

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity. 
	// trimming the Z velocity a bit seems to help quite a bit.
	vecTestVelocity = pev->velocity; 
	vecTestVelocity.z *= 0.45;

	if ( !m_fRegisteredSound && vecTestVelocity.Length() <= 60 )
	{
		//ALERT( at_console, "Grenade Registered!: %f\n", vecTestVelocity.Length() );

		// grenade is moving really slow. It's probably very close to where it will ultimately stop moving. 
		// go ahead and emit the danger sound.
		
		// register a radius louder than the explosion, so we make sure everyone gets out of the way
		CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin, pev->dmg / 0.4, 0.3 );
		m_fRegisteredSound = TRUE;
	}

	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8;

		pev->sequence = RANDOM_LONG( 1, 1 );
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
	pev->framerate = pev->velocity.Length() / 200.0;
	if (pev->framerate > 1.0)
		pev->framerate = 1;
	else if (pev->framerate < 0.5)
		pev->framerate = 0;

}



void CGasGrenade::SlideTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if ( pOther->edict() == pev->owner )
		return;

	// pev->avelocity = Vector (300, 300, 300);

	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.95;

		if (pev->velocity.x != 0 || pev->velocity.y != 0)
		{
			// maintain sliding sound
		}
	}
	else
	{
		BounceSound();
	}
}

void CGasGrenade :: BounceSound( void )
{
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit1.wav", 0.25, ATTN_NORM);	break;
	case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit2.wav", 0.25, ATTN_NORM);	break;
	case 2:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit3.wav", 0.25, ATTN_NORM);	break;
	}
}

void CGasGrenade :: TumbleThink( void )
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPRITE );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( PRECACHE_MODEL( "sprites/ef_smoke_poison.spr" ) );
		WRITE_BYTE( 5 ); // scale * 10
		WRITE_BYTE( 150  ); // framerate
	MESSAGE_END();

	if (pev->dmgtime - 1 < gpGlobals->time)
	{
		CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400, 0.1 );
	}

	if (pev->dmgtime <= gpGlobals->time)
	{
		SetThink( &CGasGrenade::Detonate );
	}
	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2;
	}
}


void CGasGrenade:: Spawn( void )
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->classname = MAKE_STRING( "grenadegas" );
	
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_grenadegas.mdl");
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));

	pev->dmg = 100;
	m_fRegisteredSound = FALSE;
}


CGasGrenade * CGasGrenade:: ShootTimed( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time )
{
	CGasGrenade *pGasGrenade = GetClassPtr( (CGasGrenade *)NULL );
	pGasGrenade->Spawn();
	UTIL_SetOrigin( pGasGrenade->pev, vecStart );
	pGasGrenade->pev->velocity = vecVelocity;
	pGasGrenade->pev->angles = UTIL_VecToAngles(pGasGrenade->pev->velocity);
	pGasGrenade->pev->owner = ENT(pevOwner);
	
	pGasGrenade->SetTouch( &CGasGrenade::BounceTouch );	// Bounce if touched
	
	// Take one second off of the desired detonation time and set the think to PreDetonate. PreDetonate
	// will insert a DANGER sound into the world sound list and delay detonation for one second so that 
	// the grenade explodes after the exact amount of time specified in the call to ShootTimed(). 

	pGasGrenade->pev->dmgtime = gpGlobals->time + time;
	pGasGrenade->SetThink( &CGasGrenade::TumbleThink );
	pGasGrenade->pev->nextthink = gpGlobals->time + 0.1;
	if (time < 0.1)
	{
		pGasGrenade->pev->nextthink = gpGlobals->time;
		pGasGrenade->pev->velocity = Vector( 0, 0, 0 );
	}
		
	pGasGrenade->pev->sequence = RANDOM_LONG( 3, 6 );
	pGasGrenade->pev->framerate = 1.0;

	// Tumble through the air
	// pGasGrenade->pev->avelocity.x = -400;

	pGasGrenade->pev->gravity = 0.5;
	pGasGrenade->pev->friction = 0.8;

	SET_MODEL(ENT(pGasGrenade->pev), "models/w_grenadegas.mdl");
	pGasGrenade->pev->dmg = 100;

	return pGasGrenade;
}

//======================end grenade

