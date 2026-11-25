// ThrillEX HUD (port of the original client's hud_style 2/3 drawing code)
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "ammohistory.h"
#include <string.h>

extern WeaponsResource gWR;

static cvar_t *g_pThrillViewSize = NULL;

int CHudThrill::Init( void )
{
	m_bActive = false;
	m_fHealthFade = 0.0f;
	m_iLastHealth = -1;
	m_pStyle = CVAR_CREATE( "hud_style", "3", FCVAR_ARCHIVE );
	g_pThrillViewSize = CVAR_CREATE( "scr_viewsize", "120", FCVAR_ARCHIVE );
	// ThrillEX draws a plain white crosshair; Xash3D paints it orange/translucent by default
	gEngfuncs.pfnClientCmd( "cl_crosshair_color \"255 255 255\"\n" );
	gEngfuncs.pfnClientCmd( "cl_crosshair_translucent 0\n" );
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem( this );
	return 1;
}

int CHudThrill::VidInit( void )
{
	// style 2 uses Hud1_* sprites, style 3 (default) uses Hud2_*
	m_iNum[0]      = gHUD.GetSpriteIndex( "Hud1_Num0" );
	m_iNum[1]      = gHUD.GetSpriteIndex( "Hud2_Num0" );
	m_iANum[0]     = gHUD.GetSpriteIndex( "Hud1_ANum0" );
	m_iANum[1]     = gHUD.GetSpriteIndex( "Hud2_ANum0" );
	m_iBatEmpty[0] = gHUD.GetSpriteIndex( "Hud1_Battery_Empty" );
	m_iBatEmpty[1] = gHUD.GetSpriteIndex( "Hud2_Battery_Empty" );
	m_iBatFull[0]  = gHUD.GetSpriteIndex( "Hud1_Battery_Full" );
	m_iBatFull[1]  = gHUD.GetSpriteIndex( "Hud2_Battery_Full" );
	m_iDivider[0]  = gHUD.GetSpriteIndex( "Hud1_Divider" );
	m_iDivider[1]  = gHUD.GetSpriteIndex( "Hud2_Divider" );

	// only take over the HUD if the mod's sprites are really there
	m_bActive = m_iNum[1] >= 0 && m_iANum[1] >= 0 && m_iBatEmpty[0] >= 0 && m_iBatFull[0] >= 0;
	m_fHealthFade = 0.0f;
	m_iLastHealth = -1;
	return 1;
}

// DHN flags: 1 = draw zero, 4 = at least 3 digits. Leading/zero digits are drawn at half brightness.
int CHudThrill::DrawNumber( int x, int y, int flags, int value, int r, int g, int b, int alpha, int idx )
{
	int w = gHUD.GetSpriteRect( idx ).right - gHUD.GetSpriteRect( idx ).left;
	int br = r, bg = g, bb = b;
	int dr = r, dg = g, db = b;
	ScaleColors( br, bg, bb, alpha );
	ScaleColors( dr, dg, db, alpha / 2 );

#define THR_DRAW( digit, cr, cg, cb ) \
	do { \
		SPR_Set( gHUD.GetSprite( idx + ( digit ) ), cr, cg, cb ); \
		SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( idx + ( digit ) ) ); \
	} while( 0 )

	if( value > 0 )
	{
		if( value >= 100 )
		{
			THR_DRAW( value / 100, br, bg, bb );
			x += w;
		}
		else if( flags & 4 )
		{
			THR_DRAW( 0, dr, dg, db );
			x += w;
		}

		if( value < 10 )
		{
			if( flags & 6 )
			{
				THR_DRAW( 0, dr, dg, db );
				x += w;
			}
		}
		else
		{
			THR_DRAW( ( value % 100 ) / 10, br, bg, bb );
			x += w;
		}

		THR_DRAW( value % 10, br, bg, bb );
		return x + w;
	}

	if( !( flags & 1 ) )
		return x;

	if( flags & 4 )
	{
		THR_DRAW( 0, dr, dg, db );
		x += w;
	}
	if( flags & 6 )
	{
		THR_DRAW( 0, dr, dg, db );
		x += w;
	}
	THR_DRAW( 0, dr, dg, db );
	return x + w;
#undef THR_DRAW
}

int CHudThrill::Draw( float flTime )
{
	if( !m_bActive || gEngfuncs.IsSpectateOnly() )
		return 1;

	int style = m_pStyle ? (int)m_pStyle->value : 3;
	int s = ( style == 2 ) ? 0 : 1; // 0 = Hud1 sprites, 1 = Hud2 sprites
	int viewsize = 120;
	if( g_pThrillViewSize )
		viewsize = (int)g_pThrillViewSize->value;

	// --- health + battery ---
	if( !( gHUD.m_iHideHUDDisplay & ( HIDEHUD_HEALTH | HIDEHUD_ALL ) ) )
	{
		int health = gHUD.m_Health.m_iHealth;
		int armor = gHUD.m_Battery.GetBat();
		if( armor > 100 )
			armor = 100;
		if( armor < 0 )
			armor = 0;

		if( m_iLastHealth != health )
		{
			if( m_iLastHealth >= 0 )
				m_fHealthFade = 255.0f;
			m_iLastHealth = health;
		}

		int alpha = (int)m_fHealthFade;
		if( alpha < ( s ? 128 : 144 ) )
			alpha = s ? 128 : 144;
		m_fHealthFade -= (float)gHUD.m_flTimeDelta * 100.0f;
		if( m_fHealthFade < 0.0f )
			m_fHealthFade = 0.0f;

		// health color: red when low, towards green when healthy
		int hr, hg, hb = 0;
		int v = health;
		if( v <= 25 )
		{
			hr = 250;
			hg = 0;
		}
		else
		{
			hg = 255 * ( v - 25 ) / 100;
			hr = 255 - hg;
		}

		int numH = gHUD.GetSpriteRect( m_iNum[s] ).bottom - gHUD.GetSpriteRect( m_iNum[s] ).top;
		int y = ScreenHeight - numH - 20;

		if( viewsize < 150 )
			DrawNumber( 14, y, 5, health, hr, hg, hb, alpha, m_iNum[s] );

		if( viewsize < 130 && ( gHUD.m_iWeaponBits & ( 1 << WEAPON_SUIT ) ) )
		{
			const wrect_t &emp = gHUD.GetSpriteRect( m_iBatEmpty[0] );
			int batW = emp.right - emp.left;
			int batH = emp.bottom - emp.top;
			int by = y - 12 - batH;

			int cr = hr, cg = hg, cb = hb;
			ScaleColors( cr, cg, cb, alpha );

			SPR_Set( gHUD.GetSprite( m_iBatEmpty[s] ), cr, cg, cb );
			SPR_DrawAdditive( 0, 14, by, &gHUD.GetSpriteRect( m_iBatEmpty[s] ) );

			wrect_t full = gHUD.GetSpriteRect( m_iBatFull[0] );
			full.right = (int)( (double)full.right - (double)( 100 - armor ) * 0.01 * (double)batW );
			if( full.right > full.left )
			{
				SPR_Set( gHUD.GetSprite( m_iBatFull[s] ), cr, cg, cb );
				SPR_DrawAdditive( 0, s ? 19 : 18, by + ( s ? 5 : 4 ), &full );
			}
		}
	}

	// --- ammo ---
	if( viewsize < 140 && !gHUD.m_fPlayerDead && !( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) ) )
	{
		WEAPON *pw = gHUD.m_Ammo.GetCurrentWeapon();
		if( pw )
		{
			const wrect_t &an = gHUD.GetSpriteRect( m_iANum[1] );
			int aw = an.right - an.left;
			int ah = an.bottom - an.top;
			int x = ScreenWidth - 3 * aw - 16;
			int y = ScreenHeight - ah - 33;
			int alpha = 224;

			if( pw->iAmmoType >= 0 )
			{
				int ammo = gWR.CountAmmo( pw->iAmmoType );
				if( pw->iClip < 0 )
				{
					DrawNumber( x, y, 5, ammo, 5, 125, 5, alpha, m_iANum[s] );
				}
				else
				{
					DrawNumber( x, y, 5, ammo, 5, 125, 5, alpha, m_iANum[s] );
					int dx = x - aw;
					int cr = 5, cg = 125, cb = 5;
					ScaleColors( cr, cg, cb, alpha );
					if( m_iDivider[s] >= 0 )
					{
						SPR_Set( gHUD.GetSprite( m_iDivider[s] ), cr, cg, cb );
						SPR_DrawAdditive( 0, dx, y, &gHUD.GetSpriteRect( m_iDivider[s] ) );
					}
					DrawNumber( dx - 3 * aw, y, 5, pw->iClip, 5, 125, 5, alpha, m_iANum[s] );
				}
			}

			if( pw->iAmmo2Type > 0 && gWR.CountAmmo( pw->iAmmo2Type ) >= 0 )
			{
				DrawNumber( ScreenWidth - 3 * aw - 32, ScreenHeight - ah - 6, 5, gWR.CountAmmo( pw->iAmmo2Type ), 5, 125, 5, alpha, m_iANum[s] );
			}
		}
	}

	return 1;
}
