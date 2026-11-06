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
#include "soundent.h"
#include "gamerules.h"

enum glauncher_e
{
	GLAUNCHER_GIDLE = 0,
	GLAUNCHER_GFIRE,
	GLAUNCHER_GRELOAD1,
	GLAUNCHER_GRELOAD2,	
	GLAUNCHER_GDRAW,
	GLAUNCHER_GHOLSTER
};

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

LINK_ENTITY_TO_CLASS( weapon_glauncher, CGlauncher );

int CGlauncher::SecondaryAmmoIndex( void )
{
	return m_iSecondaryAmmoType;
}

//=========================================================
//=========================================================
void CGlauncher::Spawn( )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/w_glauncher.mdl");
	m_iId = WEAPON_GLAUNCHER;

	m_iDefaultAmmo = GLAUNCHER_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CGlauncher::Precache( void )
{
	PRECACHE_MODEL("models/v_glauncher.mdl");
	PRECACHE_MODEL("models/w_glauncher.mdl");
	PRECACHE_MODEL("models/p_glauncher.mdl");

	PRECACHE_MODEL("models/grenade.mdl");	// grenade            

	PRECACHE_SOUND( "weapons/glauncher_fire.wav" );
	PRECACHE_SOUND( "weapons/glauncher_fire2.wav" );
	PRECACHE_SOUND( "weapons/glauncher_insert1.wav" );
	PRECACHE_SOUND( "weapons/glauncher_insert2.wav" );	
	PRECACHE_SOUND( "weapons/glauncher_start_reload.wav" );

	PRECACHE_SOUND ("weapons/357_cock1.wav");

    PRECACHE_MODEL ("models/w_ARgrenadebox.mdl");

	m_usGlauncher = PRECACHE_EVENT( 1, "events/glauncher.sc" );
}

int CGlauncher::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "ARgrenades";
	p->iMaxAmmo1 = M203_GRENADE_MAX_CARRY;
	p->pszAmmo2 = "GasGrenade";
	p->iMaxAmmo2 = HANDGRENADEGAS_MAX_CARRY;
	p->iMaxClip = GLAUNCHER_MAX_CLIP;
	p->iSlot = 5;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLAUNCHER;
	p->iWeight = GLAUNCHER_WEIGHT;

	return 1;
}

int CGlauncher::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		CBaseEntity::Create( "weapon_handgrenadegas", pev->origin, pev->angles, edict() );		
		return TRUE;
	}
	return FALSE;
}

BOOL CGlauncher::Deploy( )
{
	return DefaultDeploy( "models/v_glauncher.mdl", "models/p_glauncher.mdl", GLAUNCHER_GDRAW, "glauncher" );
}

void CGlauncher::Holster(int skiplocal)
{
    SetThink( NULL );
}

void CGlauncher::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		Reload( );
		if (m_iClip == 0)
			PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
	m_pPlayer->m_flStopExtraSoundTime = UTIL_WeaponTimeBase() + 0.2;
			
	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

 	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	// we don't add in player velocity anymore.
	CGrenade::ShootContact( m_pPlayer->pev, 
							m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 20 + gpGlobals->v_right * 8, 
							gpGlobals->v_forward * 2000 );

#ifndef CLIENT_DLL    
	Vector vecSmokeOrigin;
	UTIL_MakeAimVectors(m_pPlayer->pev->v_angle);	

	vecSmokeOrigin = m_pPlayer->GetGunPosition() +
					 gpGlobals->v_forward * 16 +
					 gpGlobals->v_right * 2 +
					 gpGlobals->v_up * -26;	
	
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( vecSmokeOrigin.x );
			WRITE_COORD( vecSmokeOrigin.y );
			WRITE_COORD( vecSmokeOrigin.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 5 ); // scale * 10
			WRITE_BYTE( 12  ); // framerate
		MESSAGE_END();
	
	UTIL_ScreenShake( pev->origin, 5.0, 10.0, 1.0, 100 );
#endif

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT( flags, m_pPlayer->edict(), m_usGlauncher );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	if (m_iClip != 0)
		m_flPumpTime = gpGlobals->time + 0.5;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.7;
	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.7;
	m_fInSpecialReload = 0;
}

void CGlauncher::SecondaryAttack()
{
	if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] == 0)
	{
		return;
	}	
	
	DefaultDeploy( "models/v_gasgrenade.mdl", "models/p_grenade.mdl", HANDGRENADEGAS_THROW1, "crowbar" );
	
#ifndef CLIENT_DLL
		CGasGrenade::ShootTimed( m_pPlayer->pev, m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16, gpGlobals->v_forward * 600, 3 );
#endif

    m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]--;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;// idle pretty soon after shooting.

	SetThink( &CGlauncher::StopAttack );
	pev->nextthink = gpGlobals->time + 1.0;
}

void CGlauncher::StopAttack(void)
{
	DefaultDeploy("models/v_glauncher.mdl", "models/p_glauncher.mdl", GLAUNCHER_GDRAW, "crowbar");
}

void CGlauncher::Reload( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == GLAUNCHER_MAX_CLIP)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		SendWeaponAnim( GLAUNCHER_GRELOAD1 );
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.7;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.7;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.7;
		return;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
			return;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		if (RANDOM_LONG(0,1))
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/glauncher_insert1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));
		else
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/glauncher_insert2.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));

		m_flNextReload = UTIL_WeaponTimeBase() + 0.7;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.7;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInSpecialReload = 1;
	}	
}


void CGlauncher::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flTimeWeaponIdle <  UTIL_WeaponTimeBase() )
	{
		if (m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload( );
		}
		else if (m_fInSpecialReload != 0)
		{
			if (m_iClip != 6 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			{
				Reload( );
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim( GLAUNCHER_GRELOAD2 );
				
				// play cocking sound
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/glauncher_start_reload.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
			}
		}
		else
		{
			int iAnim;
			float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
			if (flRand <= 0.8)
			{
				iAnim = GLAUNCHER_GIDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (60.0/12.0);// * RANDOM_LONG(2, 5);
			}
			else if (flRand <= 0.95)
			{
				iAnim = GLAUNCHER_GIDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (20.0/9.0);
			}
			else
			{
				iAnim = GLAUNCHER_GIDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (20.0/9.0);
			}
			SendWeaponAnim( iAnim );
		}
	}
}

class CGLAmmoGrenadeBox : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_ARgrenadebox.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_ARgrenadebox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_M203GRENADEBOX_GIVE, "ARgrenades", M203_GRENADE_MAX_CARRY ) != -1);

		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_grenadesbox, CGLAmmoGrenadeBox );