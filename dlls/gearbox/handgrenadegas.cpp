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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"


#define	HANDGRENADEGAS_PRIMARY_VOLUME		450

enum handgrenadegas_e {
	HANDGRENADEGAS_IDLE = 0,
	HANDGRENADEGAS_FIDGET,
	HANDGRENADEGAS_PINPULL,
	HANDGRENADEGAS_THROW1,	// toss
	HANDGRENADEGAS_THROW2,	// medium
	HANDGRENADEGAS_THROW3,	// hard
	HANDGRENADEGAS_HOLSTER,
	HANDGRENADEGAS_DRAW
};


LINK_ENTITY_TO_CLASS( weapon_handgrenadegas, CHandGrenadeGas );


void CHandGrenadeGas::Spawn( )
{
	Precache( );
	m_iId = WEAPON_HANDGRENADEGAS;
	SET_MODEL(ENT(pev), "models/w_grenadegas.mdl");

#ifndef CLIENT_DLL
	pev->dmg = gSkillData.plrDmgHandGrenade;
#endif

	m_iDefaultAmmo = HANDGRENADEGAS_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CHandGrenadeGas::Precache( void )
{
	PRECACHE_MODEL("models/w_grenadegas.mdl");
	PRECACHE_MODEL("models/v_gasgrenade.mdl");
	PRECACHE_MODEL("models/p_grenade.mdl");
    PRECACHE_MODEL("sprites/ef_smoke_poison.spr"); // nggrenade sprite
    PRECACHE_MODEL("sprites/ballsmoke.spr");
	PRECACHE_SOUND("weapons/gasgrenade.wav"); 
	PRECACHE_SOUND("weapons/gasgrenade_explode.wav");
}

int CHandGrenadeGas::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "GasGrenade";
	p->iMaxAmmo1 = HANDGRENADEGAS_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 5;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_HANDGRENADEGAS;
	p->iWeight = HANDGRENADEGAS_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}


BOOL CHandGrenadeGas::Deploy( )
{
	m_flReleaseThrow = -1;
	return DefaultDeploy( "models/v_gasgrenade.mdl", "models/p_grenade.mdl", HANDGRENADEGAS_DRAW, "crowbar" );
}

BOOL CHandGrenadeGas::CanHolster( void )
{
	// can only holster hand grenades when not primed!
	return ( m_flStartThrow == 0 );
}

void CHandGrenadeGas::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
	{
		SendWeaponAnim( HANDGRENADEGAS_HOLSTER );
	}
	else
	{
		// no more grenades!
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_HANDGRENADEGAS);
		SetThink( &CBasePlayerItem::DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
	}

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CHandGrenadeGas::PrimaryAttack()
{
	if ( !m_flStartThrow && m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] > 0 )
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;

		SendWeaponAnim( HANDGRENADEGAS_PINPULL );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
}


void CHandGrenadeGas::WeaponIdle( void )
{
	if ( m_flReleaseThrow == 0 && m_flStartThrow )
		 m_flReleaseThrow = gpGlobals->time;

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if ( m_flStartThrow )
	{
		Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if ( angThrow.x < 0 )
			angThrow.x = -10 + angThrow.x * ( ( 90 - 10 ) / 90.0 );
		else
			angThrow.x = -10 + angThrow.x * ( ( 90 + 10 ) / 90.0 );

		float flVel = ( 90 - angThrow.x ) * 4;
		if ( flVel > 500 )
			flVel = 500;

		UTIL_MakeVectors( angThrow );

		Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;

		Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;

		// alway explode 3 seconds after the pin was pulled
		float time = m_flStartThrow - gpGlobals->time + 3.0;
		if (time < 0)
			time = 0;

		CGasGrenade::ShootTimed( m_pPlayer->pev, vecSrc, vecThrow, time );

		if ( flVel < 500 )
		{
			SendWeaponAnim( HANDGRENADEGAS_THROW1 );
		}
		else if ( flVel < 1000 )
		{
			SendWeaponAnim( HANDGRENADEGAS_THROW2 );
		}
		else
		{
			SendWeaponAnim( HANDGRENADEGAS_THROW3 );
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_flReleaseThrow = 0;
		m_flStartThrow = 0;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;

		m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]--;

		if ( !m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
		{
			// just threw last grenade
			// set attack times in the future, and weapon idle in the future so we can see the whole throw
			// animation, weapon idle will automatically retire the weapon for us.
			m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;// ensure that the animation can finish playing
		}
		return;
	}
	else if ( m_flReleaseThrow > 0 )
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0;

		if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
		{
			SendWeaponAnim( HANDGRENADEGAS_DRAW );
		}
		else
		{
			RetireWeapon();
			return;
		}

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		m_flReleaseThrow = -1;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if (flRand <= 0.75)
		{
			iAnim = HANDGRENADEGAS_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );// how long till we do this again.
		}
		else 
		{
			iAnim = HANDGRENADEGAS_FIDGET;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 75.0 / 30.0;
		}

		SendWeaponAnim( iAnim );
	}
}




