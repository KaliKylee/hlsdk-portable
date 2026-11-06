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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"


enum m24_e {
	M24_IDLE1 = 0,
	M24_IDLE2_1,
	M24_IDLE2,	
	M24_RELOAD_1,		
	M24_SHOOT_1_1,		
	M24_SHOOT_1_2,			
	M24_SHOOT_1_3,		
	M24_RELOAD_2,		
	M24_DRAW_1,		
	M24_DRAW_2,		
	M24_IDLE2_2			
};

LINK_ENTITY_TO_CLASS( weapon_m24, CM24 );

int CM24::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = _357_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = M24_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 5;
	p->iId = m_iId = WEAPON_M24;
	p->iWeight = M24_WEIGHT;

	return 1;
}

int CM24::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void CM24::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_m24"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_M24;
	SET_MODEL(ENT(pev), "models/w_m24.mdl");

	m_iDefaultAmmo = M24_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CM24::Precache( void )
{
	PRECACHE_MODEL("models/v_m24.mdl");
	PRECACHE_MODEL("models/w_m24.mdl");
	PRECACHE_MODEL("models/p_m24.mdl");

	PRECACHE_MODEL("models/w_m24_clip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND ("weapons/m24_reload1.wav");
	PRECACHE_SOUND ("weapons/sniper_zoom.wav");

	m_usFireM24 = PRECACHE_EVENT( 1, "events/m24.sc" );
}

BOOL CM24::Deploy( )
{
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2.0;
	if (m_iClip)
		return DefaultDeploy( "models/v_m24.mdl", "models/p_m24.mdl", M24_DRAW_1, "m24" );
	return DefaultDeploy( "models/v_m24.mdl", "models/p_m24.mdl", M24_DRAW_2, "m24" );
}


void CM24::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if ( m_fInZoom )
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CM24::SecondaryAttack( void )
{
	if ( m_pPlayer->pev->fov != 0 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
		m_fInZoom = FALSE;
	}
	else if ( m_pPlayer->pev->fov != 15 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 15;
		m_fInZoom = TRUE;
	}

	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/sniper_zoom.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CM24::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		if (!m_fFireOnEmpty)
			Reload( );
		else
		{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	float flSpread = 0.001;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );


	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_M24, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

    int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireM24, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = 0.75;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CM24::Reload( void )
{
	if ( m_pPlayer->ammo_357 <= 0 )
		return;

	if ( m_pPlayer->pev->fov != 0 )
	{
		m_fInZoom = FALSE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
	}

	int bUseScope = FALSE;
#ifdef CLIENT_DLL
	bUseScope = bIsMultiplayer();
#else
	bUseScope = g_pGameRules->IsMultiplayer();
#endif

	if ( DefaultReload( 10, M24_RELOAD_1, 3.5 ) )
	{
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/m24_reload1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0,0xF));
	}
}


void CM24::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	// ALERT( at_console, "%.2f\n", gpGlobals->time - m_flSoundDelay );
	if (m_flSoundDelay != 0 && m_flSoundDelay <= UTIL_WeaponTimeBase() )
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/m24_reload1.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		m_flSoundDelay = 0;
	}

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	if (flRand <= 0.5)
	{
		iAnim = M24_IDLE1;
		m_flTimeWeaponIdle = (70.0/30.0);
	}
	else if (flRand <= 0.7)
	{
		iAnim = M24_IDLE2;
		m_flTimeWeaponIdle = (60.0/30.0);
	}
	else if (flRand <= 0.9)
	{
		iAnim = M24_IDLE2;
		m_flTimeWeaponIdle = (88.0/30.0);
	}
	else
	{
		iAnim = M24_IDLE2_2;
		m_flTimeWeaponIdle = (170.0/30.0);
	}
	
	int bUseScope = FALSE;
#ifdef CLIENT_DLL
	bUseScope = bIsMultiplayer();
#else
	bUseScope = g_pGameRules->IsMultiplayer();
#endif
	
	SendWeaponAnim( iAnim, UseDecrement() ? 1 : 0, bUseScope );
}



class CM24Ammo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_m24_clip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_m24_clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_M24BOX_GIVE, "357", _357_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_m24_clip, CM24Ammo );


#endif