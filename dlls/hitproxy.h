// Thrill port: invisible hit proxy for monsters whose model hitboxes are missed by bullets.
// A model-less box entity follows the monster and forwards every hit to it.
#ifndef THRILL_HITPROXY_H
#define THRILL_HITPROXY_H

// Creates the proxy if needed and moves it onto the monster. Call from MonsterThink.
void HitProxy_Sync( CBaseMonster *pOwner, EHANDLE &hProxy );
// Removes the proxy (monster died or is being removed).
void HitProxy_Remove( EHANDLE &hProxy );

#endif
