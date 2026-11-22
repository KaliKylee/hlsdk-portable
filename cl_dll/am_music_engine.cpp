/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	Use, distribution, and modification of this source code and/or resulting
*	object code is restricted to non-commercial enhancements to products from
*	Valve LLC.  All other use, distribution, or modification is prohibited
*	without written permission from Valve LLC.
*
****/

// GoldAudio engine, Xash3D FWGS / Android back-end (see am_music_engine.h)

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "am_music_engine.h"

#include <stdio.h>
#include <string.h>

#define MUSIC_FADE_SECONDS 2.0f

CFMODEngine gFMOD;

// HUD message to play music
int __MsgFunc_FMOD( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	char *cSound = READ_STRING();
	float flVol = READ_COORD();
	int iLooped = READ_BYTE();
	gFMOD.PlayMusic( cSound, flVol, iLooped == 0 ? false : true );
	return 1;
}

void CFMODEngine::Initialize( void )
{
	if ( m_iSystemState > 0 )
		return;

	// Xash3D FWGS names it "musicvolume", GoldSrc "MP3Volume"
	m_cvVolumeMusic = gEngfuncs.pfnGetCvarPointer( "musicvolume" );
	if ( !m_cvVolumeMusic )
		m_cvVolumeMusic = gEngfuncs.pfnGetCvarPointer( "MP3Volume" );

	m_flVolumeMultiplier = 1.0f;
	m_flUserVolume = m_cvVolumeMusic ? m_cvVolumeMusic->value : 1.0f;
	m_flLastTime = 0.0f;
	m_bFadeOut = false;
	m_bPlaying = false;
	m_cPathMusic[0] = '\0';
	m_iSystemState = 1;
}

void CFMODEngine::InitializeHUD( void )
{
	HOOK_MESSAGE( FMOD );
}

void CFMODEngine::ApplyVolume( float flMultiplier )
{
	if ( !m_cvVolumeMusic )
		return;

	gEngfuncs.Cvar_SetValue( m_cvVolumeMusic->name, m_flUserVolume * flMultiplier );
}

void CFMODEngine::PlayMusic( const char *_cMusic, const float _flVol, const bool _bLooped )
{
	if ( m_iSystemState != 1 )
		return;

	// Empty name: fade the current track out
	if ( !_cMusic || !_cMusic[0] )
	{
		if ( m_bPlaying )
		{
			if ( !m_bFadeOut && m_cvVolumeMusic )
				m_flUserVolume = m_cvVolumeMusic->value;
			m_bFadeOut = true;
		}
		return;
	}

	// New track requested while a fade was running: restore the player's volume
	if ( m_bFadeOut )
	{
		m_bFadeOut = false;
		m_flVolumeMultiplier = 1.0f;
		ApplyVolume( 1.0f );
	}

	if ( m_cvVolumeMusic )
		m_flUserVolume = m_cvVolumeMusic->value;

	char cCmd[160];
	snprintf( cCmd, sizeof( cCmd ), "mp3 %s \"sound/%s\"\n", _bLooped ? "loop" : "play", _cMusic );
	gEngfuncs.pfnClientCmd( cCmd );

	strncpy( m_cPathMusic, _cMusic, sizeof( m_cPathMusic ) - 1 );
	m_cPathMusic[sizeof( m_cPathMusic ) - 1] = '\0';
	m_bPlaying = true;
}

void CFMODEngine::Shutdown( void )
{
	if ( m_iSystemState != 1 )
		return;

	if ( m_bPlaying )
		gEngfuncs.pfnClientCmd( "mp3 stop\n" );

	if ( m_bFadeOut )
		ApplyVolume( 1.0f );

	m_bPlaying = false;
	m_bFadeOut = false;
	m_iSystemState = 0;
}

void CFMODEngine::Update( void )
{
	if ( m_iSystemState != 1 )
		return;

	float flTime = gEngfuncs.GetClientTime();
	float flDelta = flTime - m_flLastTime;
	m_flLastTime = flTime;

	if ( !m_bFadeOut )
		return;

	if ( flDelta < 0.0f || flDelta > 0.5f )
		flDelta = 0.0f;

	m_flVolumeMultiplier -= flDelta / MUSIC_FADE_SECONDS;
	if ( m_flVolumeMultiplier <= 0.0f )
	{
		gEngfuncs.pfnClientCmd( "mp3 stop\n" );
		ApplyVolume( 1.0f );	// give the player's volume back
		m_flVolumeMultiplier = 1.0f;
		m_bFadeOut = false;
		m_bPlaying = false;
		m_cPathMusic[0] = '\0';
		return;
	}

	ApplyVolume( m_flVolumeMultiplier );
}
