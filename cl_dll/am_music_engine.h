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

/*
======================================================================
 GoldAudio engine (Half-Rats: Parasomnia)

 Original implementation by Joel "Shepard62FR" Troch, with help from
 Alex "HAWK0044" Bakanov and Claudio "SysOp" Ficara (FMOD Studio based).

 Android / Xash3D FWGS port: the FMOD back-end was removed (FMOD is
 closed-source, desktop-only here) and music is played through the
 engine's own background-track player ("mp3 play|loop|stop").
 The public interface is unchanged, so the rest of the mod is untouched.
======================================================================
*/

#ifndef CFMODENGINE_H
#define CFMODENGINE_H

#define FOLDER_ADDON			0
#define FOLDER_HD				1

class CFMODEngine
{
public:
	void Initialize( void );
	void InitializeHUD( void );
	void PlayMusic( const char *_cMusic, const float _flVol, const bool _bLooped );
	void Shutdown( void );
	void Update( void );

	// 0 = not initialized, 1 = ready, 2 = failed
	int m_iSystemState;

private:
	void ApplyVolume( float flMultiplier );

	bool m_bFadeOut;
	bool m_bPlaying;
	char m_cPathMusic[64];
	cvar_t *m_cvVolumeMusic;
	float m_flUserVolume;			// volume the player had before a fade started
	float m_flVolumeMultiplier;		// 1.0 = full, 0.0 = silent (fade out)
	float m_flLastTime;
};

extern CFMODEngine gFMOD;

#endif // CFMODENGINE_H
