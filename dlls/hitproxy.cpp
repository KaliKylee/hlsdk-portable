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
// monster_hitproxy - see hitproxy.h
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"hitproxy.h"

class CMonsterHitProxy : public CBaseEntity
{
public:
	void Spawn( void )
	{
		pev->movetype = MOVETYPE_NONE;
		pev->solid = SOLID_BBOX;
		pev->takedamage = DAMAGE_YES;
		pev->effects |= EF_NODRAW;
		UTIL_SetSize( pev, m_vecMins, m_vecMaxs );
		UTIL_SetOrigin( pev, pev->origin );
	}
	int ObjectCaps( void ) { return FCAP_DONT_SAVE; }
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
	{
		CBaseEntity *pOwner = m_hOwner;
		if( pOwner )
			pOwner->TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
	}
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
	{
		CBaseEntity *pOwner = m_hOwner;
		// explosions already reach the monster itself through the radius search
		if( !pOwner || ( bitsDamageType & DMG_BLAST ) )
			return 0;
		return pOwner->TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
	}

	EHANDLE m_hOwner;
	Vector m_vecMins;
	Vector m_vecMaxs;
};

LINK_ENTITY_TO_CLASS( monster_hitproxy, CMonsterHitProxy )

void HitProxy_Remove( EHANDLE &hProxy )
{
	CBaseEntity *pProxy = hProxy;
	if( pProxy )
		UTIL_Remove( pProxy );
	hProxy = NULL;
}

void HitProxy_Sync( CBaseMonster *pOwner, EHANDLE &hProxy )
{
	if( pOwner->pev->deadflag != DEAD_NO || pOwner->pev->solid == SOLID_NOT )
	{
		HitProxy_Remove( hProxy );
		return;
	}

	CBaseEntity *pProxy = hProxy;
	if( !pProxy )
	{
		// the box must exist before Spawn() runs, so create it by hand
		edict_t *pent = CREATE_NAMED_ENTITY( MAKE_STRING( "monster_hitproxy" ) );
		if( FNullEnt( pent ) )
			return;
		CMonsterHitProxy *pNew = (CMonsterHitProxy *)CBaseEntity::Instance( pent );
		if( !pNew )
			return;
		pNew->pev->origin = pOwner->pev->origin;
		pNew->pev->owner = pOwner->edict();
		pNew->m_hOwner = pOwner;
		pNew->m_vecMins = pOwner->pev->mins;
		pNew->m_vecMaxs = pOwner->pev->maxs;
		DispatchSpawn( pent );
		hProxy = pNew;
		pProxy = pNew;
	}
	if( ( pProxy->pev->origin - pOwner->pev->origin ).Length() > 0.1f )
		UTIL_SetOrigin( pProxy->pev, pOwner->pev->origin );
}
