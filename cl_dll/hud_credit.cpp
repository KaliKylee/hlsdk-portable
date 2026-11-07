/***
*
*	"Port by Kali Kyle" credit with a Discord icon, drawn by the client library.
*	The icon is embedded in this file (Simple Icons "Discord" glyph, CC0), so no extra
*	assets are needed in the game folder.
*
*	Shows at the start of every level and can be repeated with the console command "discord",
*	which also tries to open the invite link on Android.
*
****/

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "mobility_int.h"
#include "hf_log.h"

#include <string.h>
#include <stdio.h>

#if defined( __ANDROID__ )
#include <jni.h>
#endif

#define HF_DISCORD_URL		"https://discord.gg/fUnXU4RZgn"
#define HF_DISCORD_SHORT	"discord.gg/fUnXU4RZgn"
#define HF_CREDIT_TEXT		"Port by Kali Kyle"
#define HF_CREDIT_FLAG		"hf_credit_done.flag" // existe = o credito ja foi mostrado

extern mobile_engfuncs_t *gMobileEngfuncs;

// Discord glyph, 40x30, 'X' = filled
static const char *g_DiscordIcon[] =
{
	"...........XXXX..........XXXX...........",
	"........XXXXXXX..........XXXXXXX........",
	"......XXXXXXXXXXXXXXXXXXXXXXXXXXXX......",
	".....XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.....",
	".....XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.....",
	"....XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX....",
	"....XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX....",
	"...XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX...",
	"...XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX...",
	"..XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX..",
	"..XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX..",
	"..XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.",
	".XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.",
	".XXXXXXXXXX.....XXXXXXXX.....XXXXXXXXXX.",
	".XXXXXXXXX......XXXXXXXX......XXXXXXXXX.",
	"XXXXXXXXXX.......XXXXXX.......XXXXXXXXXX",
	"XXXXXXXXXX.......XXXXXX.......XXXXXXXXXX",
	"XXXXXXXXXX.......XXXXXX.......XXXXXXXXXX",
	"XXXXXXXXXX......XXXXXXXX......XXXXXXXXXX",
	"XXXXXXXXXXX.....XXXXXXXX.....XXXXXXXXXXX",
	"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
	"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
	"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
	"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
	"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
	".XXXXXXXXX...XXXXXXXXXXXXXX...XXXXXXXXX.",
	"..XXXXXXXXXX......XXXX......XXXXXXXXXX..",
	"....XXXXXXXX................XXXXXXXX....",
	"......XXXXX..................XXXXX......",
	"........XXX..................XXX........"
};

#define ICON_W	40
#define ICON_H	30

#if defined( __ANDROID__ )
// Opens a URL through the Android system (ACTION_VIEW). Needs the engine to expose the JNIEnv.
static bool HF_OpenUrl( const char *url )
{
	if( !gMobileEngfuncs || !gMobileEngfuncs->pfnGetNativeObject )
		return false;

	JNIEnv *env = (JNIEnv *)gMobileEngfuncs->pfnGetNativeObject( "JNIEnv" );
	if( !env )
		return false;

	bool ok = false;

	jclass atCls = env->FindClass( "android/app/ActivityThread" );
	if( !atCls )
	{
		env->ExceptionClear();
		return false;
	}

	jmethodID curApp = env->GetStaticMethodID( atCls, "currentApplication", "()Landroid/app/Application;" );
	jobject app = curApp ? env->CallStaticObjectMethod( atCls, curApp ) : NULL;
	if( env->ExceptionCheck() )
	{
		env->ExceptionClear();
		app = NULL;
	}

	if( app )
	{
		jclass uriCls = env->FindClass( "android/net/Uri" );
		jclass intCls = env->FindClass( "android/content/Intent" );
		jclass ctxCls = env->FindClass( "android/content/Context" );

		if( uriCls && intCls && ctxCls )
		{
			jmethodID parse = env->GetStaticMethodID( uriCls, "parse", "(Ljava/lang/String;)Landroid/net/Uri;" );
			jmethodID ctor = env->GetMethodID( intCls, "<init>", "(Ljava/lang/String;Landroid/net/Uri;)V" );
			jmethodID addFlags = env->GetMethodID( intCls, "addFlags", "(I)Landroid/content/Intent;" );
			jmethodID start = env->GetMethodID( ctxCls, "startActivity", "(Landroid/content/Intent;)V" );

			if( parse && ctor && addFlags && start )
			{
				jstring jurl = env->NewStringUTF( url );
				jstring jact = env->NewStringUTF( "android.intent.action.VIEW" );
				jobject uri = env->CallStaticObjectMethod( uriCls, parse, jurl );
				jobject intent = uri ? env->NewObject( intCls, ctor, jact, uri ) : NULL;

				if( intent )
				{
					env->CallObjectMethod( intent, addFlags, (jint)0x10000000 ); // FLAG_ACTIVITY_NEW_TASK
					env->CallVoidMethod( app, start, intent );
					ok = true;
				}
			}
		}
	}

	if( env->ExceptionCheck() )
	{
		env->ExceptionClear();
		ok = false;
	}

	return ok;
}
#else
static bool HF_OpenUrl( const char *url )
{
	return false;
}
#endif

static float g_flCreditRestart = -1.0f; // set to 0 to make the credit show again (comando "discord")
static bool g_bCreditSeen = false;     // o credito ja apareceu alguma vez (arquivo) ou nesta execucao
static bool g_bCreditForced = false;   // exibicao pedida pelo comando "discord"
static bool g_bCreditActive = false;   // o credito esta na tela agora (nesta fase)

static void HF_DiscordCmd( void )
{
	gEngfuncs.pfnConsolePrint( HF_CREDIT_TEXT "\n" );
	gEngfuncs.pfnConsolePrint( "Discord: " HF_DISCORD_URL "\n" );

	if( !HF_OpenUrl( HF_DISCORD_URL ) )
		gEngfuncs.pfnConsolePrint( "(could not open the link automatically, copy it into your browser)\n" );

	g_flCreditRestart = 0.0f;
}

int CHudCredit::Init( void )
{
	m_iFlags |= HUD_ACTIVE | HUD_ALWAYSDRAW;
	m_flStart = -1.0f;
	g_bCreditSeen = HF_FlagExists( HF_CREDIT_FLAG ) != 0;

	gEngfuncs.pfnAddCommand( "discord", HF_DiscordCmd );

	gHUD.AddHudElem( this );
	return 1;
}

int CHudCredit::VidInit( void )
{
	m_iFlags |= HUD_ACTIVE | HUD_ALWAYSDRAW;
	m_flStart = -1.0f; // restart the timer on every level load
	g_bCreditForced = false;
	g_bCreditActive = false;
	return 1;
}

int CHudCredit::Draw( float flTime )
{
	if( g_flCreditRestart == 0.0f )
	{
		// pedido explicito (comando "discord"): mostra de novo mesmo que ja tenha aparecido antes
		m_flStart = flTime;
		g_flCreditRestart = -1.0f;
		g_bCreditForced = true;
	}

	// Aparece UMA vez so (no primeiro "novo jogo"): depois de visto, nunca mais sozinho
	if( g_bCreditSeen && !g_bCreditForced && !g_bCreditActive )
		return 1;

	if( m_flStart < 0.0f )
		m_flStart = flTime;

	const float flDelay = 2.0f;
	const float flShow = 8.0f;
	const float flFade = 2.0f;
	float t = flTime - m_flStart - flDelay;

	if( t < 0.0f )
		return 1;

	if( t > flShow + flFade )
	{
		g_bCreditForced = false; // terminou
		g_bCreditActive = false;
		return 1;
	}

	// ficou visivel: grava o marcador para nunca mais repetir (mesmo se o jogo fechar agora)
	if( !g_bCreditActive )
	{
		g_bCreditActive = true;
		if( !g_bCreditSeen )
		{
			g_bCreditSeen = true;
			HF_FlagSet( HF_CREDIT_FLAG );
			HF_Log( "credito exibido pela primeira vez (marcador %s criado)", HF_CREDIT_FLAG );
		}
	}

	float a = 1.0f;
	if( t > flShow )
		a = 1.0f - ( t - flShow ) / flFade;

	int s = ScreenHeight / 320;
	if( s < 1 )
		s = 1;

	const int iGap = 6 * s;
	const int iTextW = gHUD.DrawHudStringLen( (char *)HF_CREDIT_TEXT );
	const int iTotalW = ICON_W * s + iGap + iTextW;
	int x0 = ( ScreenWidth - iTotalW ) / 2;
	int y0 = ScreenHeight / 4;

	// icon (Discord blurple)
	const int ir = 88, ig = 101, ib = 242;
	const int ia = (int)( 255.0f * a );

	for( int row = 0; row < ICON_H; row++ )
	{
		const char *line = g_DiscordIcon[row];
		int col = 0;

		while( col < ICON_W )
		{
			if( line[col] != 'X' )
			{
				col++;
				continue;
			}

			int start = col;
			while( col < ICON_W && line[col] == 'X' )
				col++;

			FillRGBA( x0 + start * s, y0 + row * s, ( col - start ) * s, s, ir, ig, ib, ia );
		}
	}

	// text
	const int tx = x0 + ICON_W * s + iGap;
	const int ty = y0 + ( ICON_H * s ) / 2 - 14;
	const int tc = (int)( 255.0f * a );

	gHUD.DrawHudString( tx, ty, ScreenWidth, (char *)HF_CREDIT_TEXT, tc, tc, tc );
	gHUD.DrawHudString( tx, ty + 14, ScreenWidth, (char *)HF_DISCORD_SHORT, (int)( 160.0f * a ), (int)( 170.0f * a ), (int)( 255.0f * a ) );

	return 1;
}
