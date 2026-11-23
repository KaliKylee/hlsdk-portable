/***
* ThrillEX monster_m44 - reconstructed from decompiled hl.dll
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"

//=========================================================
// monster_m44 - simple destructible model entity.
// Takes no damage from attacks but plays its pain reactions;
// when killed it just removes itself.
//=========================================================
class CM44 : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void ) { return CLASS_NONE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void Killed( entvars_t *pevAttacker, int iGib ) { UTIL_Remove( this ); }
};

LINK_ENTITY_TO_CLASS( monster_m44, CM44 )

void CM44::Precache( void )
{
	PRECACHE_MODEL( "models/m44.mdl" );
}

void CM44::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/m44.mdl" );
	UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 64 ) );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	UTIL_SetOrigin( pev, pev->origin );
	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_YES;
	pev->health = 200;

	MonsterInit();
}

int CM44::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, 0, bitsDamageType );
}
