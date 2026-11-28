/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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

===== monsters.cpp ========================================================

  Monster-related utility code

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "animation.h"
#include "saverestore.h"
#include "studio.h"
#include "hlext_log.h"
#include <string.h>

TYPEDESCRIPTION	CBaseAnimating::m_SaveData[] = 
{
	DEFINE_FIELD( CBaseMonster, m_flFrameRate, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_flGroundSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_flLastEventCheck, FIELD_TIME ),
	DEFINE_FIELD( CBaseMonster, m_fSequenceFinished, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseMonster, m_fSequenceLoops, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CBaseAnimating, CBaseDelay );


//=========================================================
// StudioFrameAdvance - advance the animation frame up to the current time
// if an flInterval is passed in, only advance animation that number of seconds
//=========================================================
float CBaseAnimating :: StudioFrameAdvance ( float flInterval )
{
	if (flInterval == 0.0)
	{
		flInterval = (gpGlobals->time - pev->animtime);
		if (flInterval <= 0.001)
		{
			pev->animtime = gpGlobals->time;
			return 0.0;
		}
	}
	if (! pev->animtime)
		flInterval = 0.0;
	
	pev->frame += flInterval * m_flFrameRate * pev->framerate;
	pev->animtime = gpGlobals->time;

	if (pev->frame < 0.0 || pev->frame >= 256.0) 
	{
		if (m_fSequenceLoops)
			pev->frame -= (int)(pev->frame / 256.0) * 256.0;
		else
			pev->frame = (pev->frame < 0.0) ? 0 : 255;
		m_fSequenceFinished = TRUE;	// just in case it wasn't caught in GetEvents
	}

	return flInterval;
}

//=========================================================
// LookupActivity
//=========================================================
int CBaseAnimating :: LookupActivity ( int activity )
{
	ASSERT( activity != 0 );
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	return ::LookupActivity( pmodel, pev, activity );
}

//=========================================================
// LookupActivityHeaviest
//
// Get activity with highest 'weight'
//
//=========================================================
int CBaseAnimating :: LookupActivityHeaviest ( int activity )
{
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	return ::LookupActivityHeaviest( pmodel, pev, activity );
}

//=========================================================
//=========================================================
int CBaseAnimating :: LookupSequence ( const char *label )
{
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	return ::LookupSequence( pmodel, label );
}


//=========================================================
//=========================================================
void CBaseAnimating :: ResetSequenceInfo ( )
{
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	GetSequenceInfo( pmodel, pev, &m_flFrameRate, &m_flGroundSpeed );
	m_fSequenceLoops = ((GetSequenceFlags() & STUDIO_LOOPING) != 0);
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;
	m_fSequenceFinished = FALSE;
	m_flLastEventCheck = gpGlobals->time;
}



//=========================================================
//=========================================================
BOOL CBaseAnimating :: GetSequenceFlags( )
{
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	return ::GetSequenceFlags( pmodel, pev );
}

//=========================================================
// DispatchAnimEvents
//=========================================================
void CBaseAnimating :: DispatchAnimEvents ( float flInterval )
{
	MonsterEvent_t	event;

	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	if ( !pmodel )
	{
		ALERT( at_aiconsole, "Gibbed monster is thinking!\n" );
		return;
	}

	// FIXME: I have to do this or some events get missed, and this is probably causing the problem below
	flInterval = 0.1;

	// FIX: this still sometimes hits events twice
	float flStart = pev->frame + (m_flLastEventCheck - pev->animtime) * m_flFrameRate * pev->framerate;
	float flEnd = pev->frame + flInterval * m_flFrameRate * pev->framerate;
	m_flLastEventCheck = pev->animtime + flInterval;

	m_fSequenceFinished = FALSE;
	if (flEnd >= 256 || flEnd <= 0.0) 
		m_fSequenceFinished = TRUE;

	int index = 0;

	while ( (index = GetAnimationEvent( pmodel, pev, &event, flStart, flEnd, index ) ) != 0 )
	{
		HandleAnimEvent( &event );
	}
}


//=========================================================
//=========================================================
float CBaseAnimating :: SetBoneController ( int iController, float flValue )
{
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	return SetController( pmodel, pev, iController, flValue );
}

//=========================================================
//=========================================================
void CBaseAnimating :: InitBoneControllers ()
{
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	SetController( pmodel, pev, 0, 0.0 );
	SetController( pmodel, pev, 1, 0.0 );
	SetController( pmodel, pev, 2, 0.0 );
	SetController( pmodel, pev, 3, 0.0 );
}

//=========================================================
//=========================================================
float CBaseAnimating :: SetBlending ( int iBlender, float flValue )
{
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	return ::SetBlending( pmodel, pev, iBlender, flValue );
}

//=========================================================
//=========================================================
void CBaseAnimating :: GetBonePosition ( int iBone, Vector &origin, Vector &angles )
{
	GET_BONE_POSITION( ENT(pev), iBone, origin, angles );
}

//=========================================================
//=========================================================
void CBaseAnimating :: GetAttachment ( int iAttachment, Vector &origin, Vector &angles )
{
	GET_ATTACHMENT( ENT(pev), iAttachment, origin, angles );
}

//=========================================================
//=========================================================
int CBaseAnimating :: FindTransition( int iEndingSequence, int iGoalSequence, int *piDir )
{
	void *pmodel = GET_MODEL_PTR( ENT(pev) );
	
	if (piDir == NULL)
	{
		int iDir;
		int sequence = ::FindTransition( pmodel, iEndingSequence, iGoalSequence, &iDir );
		if (iDir != 1)
			return -1;
		else
			return sequence;
	}

	return ::FindTransition( pmodel, iEndingSequence, iGoalSequence, piDir );
}

//=========================================================
//=========================================================
void CBaseAnimating :: GetAutomovement( Vector &origin, Vector &angles, float flInterval )
{

}

//=========================================================
// Compatibilidade de bodygroups (HL:E Android)
//
// O codigo deste projeto foi escrito para os modelos do SDK 1.1.3, mas o jogo usa modelos mais
// novos em que a ORDEM dos bodygroups mudou (ex.: scientist.mdl: heads/body/hand_items virou
// body/hand_items/heads). Com a ordem errada, "hands 4" (o jornal) caia no grupo de cabecas e o
// NPC ficava sem o objeto na mao. Aqui convertemos o indice do codigo (ordem antiga) para o
// indice real do modelo carregado, procurando o grupo pelo NOME. Desligue com "hlext_bodyfix 0".
//=========================================================
struct HlextBodyLayout_t
{
	const char *pszModel;
	const char *rgpszOldGroups[8]; // nomes dos grupos na ordem usada pelo codigo (SDK 1.1.3)
};

static const HlextBodyLayout_t g_HlextBodyLayouts[] =
{
	{ "models/scientist.mdl",            { "heads", "body", "hand_items", NULL } },
	{ "models/scientist_female.mdl",     { "studio", "heads", "needle", NULL } },
	{ "models/scientist_hev.mdl",        { "studio", "heads", "needle", "suits", NULL } },
	{ "models/cleansuit_scientist.mdl",  { "studio", "heads", "needle", NULL } },
	{ "models/barney.mdl",               { "heads", "helmet", "body", "glasses", "gun", NULL } },
	{ "models/barney_hev.mdl",           { "studio", "heads", "helmet", "gun", "glasses", NULL } },
	{ "models/otis.mdl",                 { "heads", "gun", "body", "helmet", "glasses", NULL } },
	// operarios: Head=0 Hat=2 Hands=3 no codigo (SDK 1.1.3). No modelo novo: body/heads/hand_items/hat.
	{ "models/worker.mdl",               { "heads", "clothing", "helmet", "items", NULL } },
	// Head=1 Hands=2 Helm=3 Gloves=4 no codigo. O modelo novo nao tem luvas (grupo some).
	{ "models/construction.mdl",         { "studio", "heads", "items", "helmet", "hands", NULL } },
	// Head=0 Clothing=1 Hat=2 no codigo. O modelo novo (body/heads/hand_items) so tem cabecas em comum.
	{ "models/chef.mdl",                 { "heads", "clothing", "hat", NULL } },
};

static bool HlextNamesExact( const char *pszOld, const char *pszNew )
{
	return !stricmp( pszOld, pszNew );
}

static bool HlextNamesMatch( const char *pszOld, const char *pszNew )
{
	if ( !stricmp( pszOld, pszNew ) )
		return true;
	// nomes que mudaram de um modelo para outro
	if ( ( !stricmp( pszOld, "heads" ) && !stricmp( pszNew, "head" ) ) ||
		 ( !stricmp( pszOld, "studio" ) && !stricmp( pszNew, "body" ) ) ||
		 ( !stricmp( pszOld, "needle" ) && !stricmp( pszNew, "hand_items" ) ) ||
		 ( !stricmp( pszOld, "items" ) && !stricmp( pszNew, "hand_items" ) ) ||
		 ( !stricmp( pszOld, "helmet" ) && !stricmp( pszNew, "hat" ) ) )
		return true;
	return false;
}

// Retorna o indice real do grupo; -1 = esse grupo nao existe no modelo novo (ignorar);
// iGroup sem mudanca quando nao ha o que converter.
static int HlextRemapBodygroup( void *pmodel, entvars_t *pev, int iGroup )
{
	extern cvar_t hlext_bodyfix;
	if ( !pmodel || !pev || hlext_bodyfix.value == 0 )
		return iGroup;

	const char *pszModel = STRING( pev->model );
	if ( !pszModel || !pszModel[0] )
		return iGroup;

	for ( size_t i = 0; i < sizeof( g_HlextBodyLayouts ) / sizeof( g_HlextBodyLayouts[0] ); i++ )
	{
		const HlextBodyLayout_t &layout = g_HlextBodyLayouts[i];
		if ( stricmp( pszModel, layout.pszModel ) )
			continue;

		int nOld = 0;
		while ( nOld < 8 && layout.rgpszOldGroups[nOld] )
			nOld++;
		if ( iGroup < 0 || iGroup >= nOld )
			return iGroup;

		studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
		if ( pstudiohdr->numbodyparts <= 0 )
			return iGroup;
		mstudiobodyparts_t *pbodypart = (mstudiobodyparts_t *)( (byte *)pstudiohdr + pstudiohdr->bodypartindex );

		// se o modelo ja esta na ordem antiga (SDK 1.1.3), nao mexe em nada
		bool fSameOrder = pstudiohdr->numbodyparts == nOld;
		for ( int g = 0; fSameOrder && g < nOld; g++ )
			if ( !HlextNamesMatch( layout.rgpszOldGroups[g], pbodypart[g].name ) )
				fSameOrder = false;
		if ( fSameOrder )
			return iGroup;

		for ( int pass = 0; pass < 2; pass++ )
		{
			for ( int p = 0; p < pstudiohdr->numbodyparts; p++ )
			{
				bool fMatch = pass == 0 ? HlextNamesExact( layout.rgpszOldGroups[iGroup], pbodypart[p].name )
										: HlextNamesMatch( layout.rgpszOldGroups[iGroup], pbodypart[p].name );
				if ( fMatch )
				{
					static int s_iLogged = 0;
					if ( s_iLogged++ < 40 )
						HLEXT_Log( "bodygroups: %s grupo %d (%s) -> %d (%s)", pszModel, iGroup, layout.rgpszOldGroups[iGroup], p, pbodypart[p].name );
					return p;
				}
			}
		}
		HLEXT_Log( "bodygroups: %s grupo %d (%s) nao existe no modelo novo; ignorado", pszModel, iGroup, layout.rgpszOldGroups[iGroup] );
		return -1;
	}

	return iGroup;
}

void CBaseAnimating :: SetBodygroup( int iGroup, int iValue )
{
	void *pmodel = GET_MODEL_PTR( ENT(pev) );
	iGroup = HlextRemapBodygroup( pmodel, pev, iGroup );
	if ( iGroup < 0 )
		return;
	::SetBodygroup( pmodel, pev, iGroup, iValue );
}

int CBaseAnimating :: GetBodygroup( int iGroup )
{
	void *pmodel = GET_MODEL_PTR( ENT(pev) );
	iGroup = HlextRemapBodygroup( pmodel, pev, iGroup );
	if ( iGroup < 0 )
		return 0;
	return ::GetBodygroup( pmodel, pev, iGroup );
}


int CBaseAnimating :: ExtractBbox( int sequence, float *mins, float *maxs )
{
	return ::ExtractBbox( GET_MODEL_PTR( ENT(pev) ), sequence, mins, maxs );
}

//=========================================================
//=========================================================

void CBaseAnimating :: SetSequenceBox()
{
	Vector mins, maxs;

	// Get sequence bbox
	if ( ExtractBbox( pev->sequence, mins, maxs ) )
	{
		// expand box for rotation
		// find min / max for rotations
		float yaw = pev->angles.y * (M_PI / 180.0);
		
		Vector xvector, yvector;
		xvector.x = cos(yaw);
		xvector.y = sin(yaw);
		yvector.x = -sin(yaw);
		yvector.y = cos(yaw);
		Vector bounds[2];

		bounds[0] = mins;
		bounds[1] = maxs;
		
		Vector rmin( 9999, 9999, 9999 );
		Vector rmax( -9999, -9999, -9999 );
		Vector base, transformed;

		for (int i = 0; i <= 1; i++ )
		{
			base.x = bounds[i].x;
			for ( int j = 0; j <= 1; j++ )
			{
				base.y = bounds[j].y;
				for ( int k = 0; k <= 1; k++ )
				{
					base.z = bounds[k].z;
					
				// transform the point
					transformed.x = xvector.x*base.x + yvector.x*base.y;
					transformed.y = xvector.y*base.x + yvector.y*base.y;
					transformed.z = base.z;
					
					if (transformed.x < rmin.x)
						rmin.x = transformed.x;
					if (transformed.x > rmax.x)
						rmax.x = transformed.x;
					if (transformed.y < rmin.y)
						rmin.y = transformed.y;
					if (transformed.y > rmax.y)
						rmax.y = transformed.y;
					if (transformed.z < rmin.z)
						rmin.z = transformed.z;
					if (transformed.z > rmax.z)
						rmax.z = transformed.z;
				}
			}
		}
		rmin.z = 0;
		rmax.z = rmin.z + 1;
		UTIL_SetSize( pev, rmin, rmax );
	}
}

