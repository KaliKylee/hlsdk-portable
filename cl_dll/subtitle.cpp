// subtitle.cpp - legendas das falas dos NPCs (HL:E)
//
// O servidor manda a mensagem "Subtitle" com o id da sentenca (ex.: "!SC_QUESTION0") quando uma
// sentenca e tocada perto do jogador. Aqui carregamos sound/subtitles_<idioma>.txt e desenhamos o texto.
// Cvars: cl_subtitle (liga/desliga), cl_subtitle_red/green/blue (cor do fundo), cl_subtitle_language.
//
// Formato do arquivo:
//   !SC_QUESTION0
//   {
//   Texto da legenda
//   [len:003 color:185 185 185]
//   }

// headers da STL antes dos do HLSDK (Platform.h define o macro "clamp", que quebra o <algorithm>)
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "hlext_log.h"

DECLARE_MESSAGE( m_Subtitle, Subtitle )

namespace
{
	struct SubEntry
	{
		std::string text;
		float len;
		int r, g, b;
	};

	struct SubActive
	{
		std::string text;
		float expire;
		int r, g, b;
	};

	const int MAX_ACTIVE = 3;

	std::unordered_map<std::string, SubEntry> g_Entries;
	std::vector<SubActive> g_Active;
	std::string g_LoadedLang;
	bool g_bTriedLoad = false;

	cvar_t *cl_subtitle = nullptr;
	cvar_t *cl_subtitle_red = nullptr;
	cvar_t *cl_subtitle_green = nullptr;
	cvar_t *cl_subtitle_blue = nullptr;
	cvar_t *cl_subtitle_language = nullptr;

	std::string Upper( const std::string &s )
	{
		std::string r = s;
		for ( size_t i = 0; i < r.size(); i++ )
			r[i] = (char)toupper( (unsigned char)r[i] );
		return r;
	}

	std::string Trim( const std::string &s )
	{
		size_t a = 0, b = s.size();
		while ( a < b && isspace( (unsigned char)s[a] ) ) a++;
		while ( b > a && isspace( (unsigned char)s[b - 1] ) ) b--;
		return s.substr( a, b - a );
	}

	// Le sound/subtitles_<lang>.txt para g_Entries. Retorna o numero de entradas.
	int LoadSubtitleFile( const char *lang )
	{
		char path[128];
		snprintf( path, sizeof( path ), "sound/subtitles_%s.txt", lang );

		int length = 0;
		char *pfile = (char *)gEngfuncs.COM_LoadFile( path, 5, &length );
		if ( !pfile )
			return -1;

		std::string data( pfile, length > 0 ? (size_t)length : strlen( pfile ) );
		gEngfuncs.COM_FreeFile( pfile );

		g_Entries.clear();

		// quebra em linhas (\r\n ou \n)
		std::vector<std::string> lines;
		{
			size_t start = 0;
			for ( size_t i = 0; i <= data.size(); i++ )
			{
				if ( i == data.size() || data[i] == '\n' )
				{
					lines.push_back( Trim( data.substr( start, i - start ) ) );
					start = i + 1;
				}
			}
		}

		for ( size_t i = 0; i < lines.size(); i++ )
		{
			const std::string &line = lines[i];
			if ( line.empty() || line.compare( 0, 2, "//" ) == 0 )
				continue;

			// id da sentenca: proxima linha nao vazia tem que ser "{"
			size_t j = i + 1;
			while ( j < lines.size() && lines[j].empty() ) j++;
			if ( j >= lines.size() || lines[j] != "{" )
				continue;

			SubEntry e;
			e.len = 3.0f;
			e.r = e.g = e.b = 255;
			std::string text;

			size_t k = j + 1;
			for ( ; k < lines.size() && lines[k] != "}"; k++ )
			{
				const std::string &l = lines[k];
				if ( l.empty() || l.compare( 0, 2, "//" ) == 0 )
					continue;

				if ( l[0] == '[' && l.find( "len:" ) != std::string::npos )
				{
					size_t p = l.find( "len:" );
					e.len = (float)atof( l.c_str() + p + 4 );
					p = l.find( "color:" );
					if ( p != std::string::npos )
					{
						int r = 255, g = 255, b = 255;
						if ( sscanf( l.c_str() + p + 6, "%d %d %d", &r, &g, &b ) == 3 )
						{
							e.r = r; e.g = g; e.b = b;
						}
					}
					continue;
				}

				if ( !text.empty() ) text += " ";
				text += l;
			}

			e.text = text;
			if ( line != "SubtitleFormat" && !text.empty() )
				g_Entries[Upper( line )] = e;

			i = k; // continua depois do "}"
		}

		return (int)g_Entries.size();
	}

	void EnsureLoaded()
	{
		std::string lang = ( cl_subtitle_language && cl_subtitle_language->string[0] ) ? cl_subtitle_language->string : "english";
		if ( g_bTriedLoad && lang == g_LoadedLang )
			return;

		g_bTriedLoad = true;
		g_LoadedLang = lang;

		int n = LoadSubtitleFile( lang.c_str() );
		if ( n < 0 && lang != "english" )
			n = LoadSubtitleFile( "english" );

		if ( n < 0 )
			HLEXT_Log( "legendas: sound/subtitles_%s.txt nao encontrado", lang.c_str() );
		else
			HLEXT_Log( "legendas: %d entradas carregadas (idioma %s)", n, lang.c_str() );
	}
}

int CHudSubtitle::Init()
{
	HOOK_MESSAGE( Subtitle );

	cl_subtitle = CVAR_CREATE( "cl_subtitle", "1", FCVAR_ARCHIVE );
	cl_subtitle_red = CVAR_CREATE( "cl_subtitle_red", "30", FCVAR_ARCHIVE );
	cl_subtitle_green = CVAR_CREATE( "cl_subtitle_green", "30", FCVAR_ARCHIVE );
	cl_subtitle_blue = CVAR_CREATE( "cl_subtitle_blue", "30", FCVAR_ARCHIVE );
	cl_subtitle_language = CVAR_CREATE( "cl_subtitle_language", "english", FCVAR_ARCHIVE );

	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem( this );
	return 1;
}

int CHudSubtitle::VidInit()
{
	g_Active.clear();
	return 1;
}

void CHudSubtitle::InitHUDData()
{
	g_Active.clear();
}

void CHudSubtitle::Reset()
{
	g_Active.clear();
}

int CHudSubtitle::MsgFunc_Subtitle( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	const char *pszKey = READ_STRING();

	if ( !cl_subtitle || cl_subtitle->value == 0.0f || !pszKey || !pszKey[0] )
		return 1;

	EnsureLoaded();

	auto it = g_Entries.find( Upper( pszKey ) );
	if ( it == g_Entries.end() )
	{
		static int s_iMissing = 0;
		if ( s_iMissing++ < 10 )
			HLEXT_Log( "legendas: sentenca %s sem legenda no arquivo", pszKey );
		return 1;
	}

	const SubEntry &e = it->second;
	float expire = gHUD.m_flTime + ( e.len > 0.5f ? e.len : 3.0f );

	for ( size_t i = 0; i < g_Active.size(); i++ )
	{
		if ( g_Active[i].text == e.text )
		{
			g_Active[i].expire = expire;
			return 1;
		}
	}

	SubActive a;
	a.text = e.text;
	a.expire = expire;
	a.r = e.r; a.g = e.g; a.b = e.b;
	g_Active.push_back( a );

	if ( (int)g_Active.size() > MAX_ACTIVE )
		g_Active.erase( g_Active.begin() );

	return 1;
}

int CHudSubtitle::Draw( float flTime )
{
	if ( g_Active.empty() || !cl_subtitle || cl_subtitle->value == 0.0f )
		return 1;

	// remove as que acabaram
	g_Active.erase( std::remove_if( g_Active.begin(), g_Active.end(),
		[flTime]( const SubActive &a ) { return flTime > a.expire; } ), g_Active.end() );

	if ( g_Active.empty() )
		return 1;

	const int pad = 6;
	const int maxWidth = (int)( ScreenWidth * 0.7f );

	int _w, lineH;
	GetConsoleStringSize( "Ag", &_w, &lineH );
	if ( lineH <= 0 ) lineH = 14;

	// monta as linhas (quebra por palavras) de cada legenda
	std::vector<std::vector<std::string> > blocks;
	int totalH = 0;
	for ( size_t n = 0; n < g_Active.size(); n++ )
	{
		std::vector<std::string> out;
		std::string cur, word;
		const std::string &t = g_Active[n].text;
		for ( size_t i = 0; i <= t.size(); i++ )
		{
			if ( i == t.size() || t[i] == ' ' )
			{
				if ( !word.empty() )
				{
					std::string test = cur.empty() ? word : cur + " " + word;
					if ( !cur.empty() && ConsoleStringLen( test.c_str() ) > maxWidth - 2 * pad )
					{
						out.push_back( cur );
						cur = word;
					}
					else
						cur = test;
					word.clear();
				}
			}
			else
				word += t[i];
		}
		if ( !cur.empty() ) out.push_back( cur );
		if ( out.empty() ) out.push_back( "" );

		totalH += (int)out.size() * lineH + 2 * pad + 2;
		blocks.push_back( out );
	}

	int bgR = cl_subtitle_red ? (int)cl_subtitle_red->value : 30;
	int bgG = cl_subtitle_green ? (int)cl_subtitle_green->value : 30;
	int bgB = cl_subtitle_blue ? (int)cl_subtitle_blue->value : 30;

	int y = (int)( ScreenHeight * 0.84f ) - totalH;
	if ( y < 0 ) y = 0;

	for ( size_t n = 0; n < blocks.size(); n++ )
	{
		const std::vector<std::string> &ls = blocks[n];

		int w = 0;
		for ( size_t i = 0; i < ls.size(); i++ )
			w = std::max( w, ConsoleStringLen( ls[i].c_str() ) );

		int boxW = w + 2 * pad;
		int boxH = (int)ls.size() * lineH + 2 * pad;
		int x = ( ScreenWidth - boxW ) / 2;

		FillRGBA( x, y, boxW, boxH, bgR, bgG, bgB, 170 );

		gEngfuncs.pfnDrawSetTextColor( g_Active[n].r / 255.0f, g_Active[n].g / 255.0f, g_Active[n].b / 255.0f );
		for ( size_t i = 0; i < ls.size(); i++ )
		{
			int lw = ConsoleStringLen( ls[i].c_str() );
			DrawConsoleString( ( ScreenWidth - lw ) / 2, y + pad + (int)i * lineH, ls[i].c_str() );
		}

		y += boxH + 2;
	}

	return 1;
}
