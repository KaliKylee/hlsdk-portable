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


LINK_ENTITY_TO_CLASS( grenadebullet, CGrenadeBullet );

// Grenades flagged with this will be triggered when the owner calls detonateSatchelCharges
#define SF_DETONATE		0x0001

//
// Grenade Explode
//
void CGrenadeBullet::Explode( Vector vecSrc, Vector vecAim )
{
	TraceResult tr;
	UTIL_TraceLine ( pev->origin, pev->origin + Vector ( 0, 0, -32 ),  ignore_monsters, ENT(pev), & tr);

	Explode( &tr, DMG_BLAST );
}

// UNDONE: temporary scorching for PreAlpha - find a less sleazy permenant solution.
void CGrenadeBullet::Explode( TraceResult *pTrace, int bitsDamageType )
{
	float		flRndSound;// sound randomizer

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->flFraction != 1.0 )
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * (pev->dmg - 24) * 0.6);
	}

	if ( RANDOM_FLOAT( 0 , 1 ) < 0.5 )
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH1 );
	}
	else
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH2 );
	}

	SetThink( &CGrenadeBullet::Smoke );
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.3;
}


void CGrenadeBullet::Smoke( void )
{
	if (UTIL_PointContents ( pev->origin ) == CONTENTS_WATER)
	{
	}
	else
	{
	}
	UTIL_Remove( this );
}

void CGrenadeBullet::Killed( entvars_t *pevAttacker, int iGib )
{
	Detonate( );
}


// Timed grenade, this think is called when time runs out.
void CGrenadeBullet::DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CGrenadeBullet::Detonate );
	pev->nextthink = gpGlobals->time;
}

void CGrenadeBullet::PreDetonate( void )
{
	CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin, 400, 0.3 );

	SetThink( &CGrenadeBullet::Detonate );
	pev->nextthink = gpGlobals->time + 1;
}


void CGrenadeBullet::Detonate( void )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	vecSpot = pev->origin;
	UTIL_TraceLine( vecSpot - Vector( 64, 64, 64 ), vecSpot + Vector( 64, 64, 64 ), ignore_monsters, ENT(pev), &tr );

	TraceResult tr2;
	Vector		vecSpot2;// trace starts here!

	vecSpot2 = pev->origin;
	UTIL_TraceLine( vecSpot2, vecSpot2 + Vector( -40, -40, -40 ), ignore_monsters, ENT(pev), &tr2 );

	Explode( &tr, DMG_BLAST );
	Explode( &tr2, DMG_BLAST );
}


//
// Contact grenade, explode when it touches something
// 
void CGrenadeBullet::ExplodeTouch( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT(pev), &tr );

	Explode( &tr, DMG_BLAST );
}


void CGrenadeBullet::DangerSoundThink( void )
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


void CGrenadeBullet::BounceTouch( CBaseEntity *pOther )
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



void CGrenadeBullet::SlideTouch( CBaseEntity *pOther )
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

void CGrenadeBullet :: BounceSound( void )
{
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit1.wav", 0.25, ATTN_NORM);	break;
	case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit2.wav", 0.25, ATTN_NORM);	break;
	case 2:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit3.wav", 0.25, ATTN_NORM);	break;
	}
}

void CGrenadeBullet :: TumbleThink( void )
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->dmgtime - 1 < gpGlobals->time)
	{
		CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400, 0.1 );
	}

	if (pev->dmgtime <= gpGlobals->time)
	{
		SetThink( &CGrenadeBullet::Detonate );
	}
	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2;
	}
}


void CGrenadeBullet:: Spawn( void )
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->classname = MAKE_STRING( "grenadebullet" );
	
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/grenade.mdl");
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));

	pev->dmg = 100;
	m_fRegisteredSound = FALSE;
}


CGrenadeBullet *CGrenadeBullet::ShootContact( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CGrenadeBullet *pGrenadeBullet = GetClassPtr( (CGrenadeBullet *)NULL );
	pGrenadeBullet->Spawn();
	// contact grenades arc lower
	pGrenadeBullet->pev->gravity = 0.5;// lower gravity since grenade is aerodynamic and engine doesn't know it.
	UTIL_SetOrigin( pGrenadeBullet->pev, vecStart );
	pGrenadeBullet->pev->velocity = vecVelocity;
	pGrenadeBullet->pev->angles = UTIL_VecToAngles (pGrenadeBullet->pev->velocity);
	pGrenadeBullet->pev->owner = ENT(pevOwner);
	
	// make monsters afaid of it while in the air
	pGrenadeBullet->SetThink( &CGrenadeBullet::DangerSoundThink );
	pGrenadeBullet->pev->nextthink = gpGlobals->time;
	
	// Tumble in air
	pGrenadeBullet->pev->avelocity.x = RANDOM_FLOAT ( -100, -500 );
	
	// Explode on contact
	pGrenadeBullet->SetTouch( &CGrenadeBullet::ExplodeTouch );

	pGrenadeBullet->pev->dmg = gSkillData.plrDmgM203Grenade;

	return pGrenadeBullet;
}


CGrenadeBullet * CGrenadeBullet:: ShootTimed( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time )
{
	CGrenadeBullet *pGrenadeBullet = GetClassPtr( (CGrenadeBullet *)NULL );
	pGrenadeBullet->Spawn();
	UTIL_SetOrigin( pGrenadeBullet->pev, vecStart );
	pGrenadeBullet->pev->velocity = vecVelocity;
	pGrenadeBullet->pev->angles = UTIL_VecToAngles(pGrenadeBullet->pev->velocity);
	pGrenadeBullet->pev->owner = ENT(pevOwner);
	
	pGrenadeBullet->SetTouch( &CGrenadeBullet::BounceTouch );	// Bounce if touched
	
	// Take one second off of the desired detonation time and set the think to PreDetonate. PreDetonate
	// will insert a DANGER sound into the world sound list and delay detonation for one second so that 
	// the grenade explodes after the exact amount of time specified in the call to ShootTimed(). 

	pGrenadeBullet->pev->dmgtime = gpGlobals->time + time;
	pGrenadeBullet->SetThink( &CGrenadeBullet::TumbleThink );
	pGrenadeBullet->pev->nextthink = gpGlobals->time + 0.1;
	if (time < 0.1)
	{
		pGrenadeBullet->pev->nextthink = gpGlobals->time;
		pGrenadeBullet->pev->velocity = Vector( 0, 0, 0 );
	}
		
	pGrenadeBullet->pev->sequence = RANDOM_LONG( 3, 6 );
	pGrenadeBullet->pev->framerate = 1.0;

	// Tumble through the air
	// pGrenadeBullet->pev->avelocity.x = -400;

	pGrenadeBullet->pev->gravity = 0.5;
	pGrenadeBullet->pev->friction = 0.8;

	SET_MODEL(ENT(pGrenadeBullet->pev), "models/null.mdl");
	pGrenadeBullet->pev->dmg = 100;

	return pGrenadeBullet;
}


CGrenadeBullet * CGrenadeBullet :: ShootSatchelCharge( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CGrenadeBullet *pGrenadeBullet = GetClassPtr( (CGrenadeBullet *)NULL );
	pGrenadeBullet->pev->movetype = MOVETYPE_BOUNCE;
	pGrenadeBullet->pev->classname = MAKE_STRING( "grenadebullet" );
	
	pGrenadeBullet->pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pGrenadeBullet->pev), "models/grenade.mdl");	// Change this to satchel charge model

	UTIL_SetSize(pGrenadeBullet->pev, Vector( 0, 0, 0), Vector(0, 0, 0));

	pGrenadeBullet->pev->dmg = 200;
	UTIL_SetOrigin( pGrenadeBullet->pev, vecStart );
	pGrenadeBullet->pev->velocity = vecVelocity;
	pGrenadeBullet->pev->angles = g_vecZero;
	pGrenadeBullet->pev->owner = ENT(pevOwner);
	
	// Detonate in "time" seconds
	pGrenadeBullet->SetThink( &CGrenadeBullet::SUB_DoNothing );
	pGrenadeBullet->SetUse( &CGrenadeBullet::DetonateUse );
	pGrenadeBullet->SetTouch( &CGrenadeBullet::SlideTouch );
	pGrenadeBullet->pev->spawnflags = SF_DETONATE;

	pGrenadeBullet->pev->friction = 0.9;

	return pGrenadeBullet;
}



void CGrenadeBullet :: UseSatchelCharges( entvars_t *pevOwner, SATCHELCODE code )
{
	edict_t *pentFind;
	edict_t *pentOwner;

	if ( !pevOwner )
		return;

	CBaseEntity	*pOwner = CBaseEntity::Instance( pevOwner );

	pentOwner = pOwner->edict();

	pentFind = FIND_ENTITY_BY_CLASSNAME( NULL, "grenadebullet" );
	while ( !FNullEnt( pentFind ) )
	{
		CBaseEntity *pEnt = Instance( pentFind );
		if ( pEnt )
		{
			if ( FBitSet( pEnt->pev->spawnflags, SF_DETONATE ) && pEnt->pev->owner == pentOwner )
			{
				if ( code == SATCHEL_DETONATE )
					pEnt->Use( pOwner, pOwner, USE_ON, 0 );
				else	// SATCHEL_RELEASE
					pEnt->pev->owner = NULL;
			}
		}
		pentFind = FIND_ENTITY_BY_CLASSNAME( pentFind, "grenadebullet" );
	}
}

//======================end grenade

