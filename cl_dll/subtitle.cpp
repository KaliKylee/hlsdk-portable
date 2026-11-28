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
#include "subtitle_font.h"

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
		float start;
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
	cvar_t *cl_subtitle_scale = nullptr;
	cvar_t *cl_subtitle_size = nullptr;

	// atlas de fonte (sprites) carregados sob demanda; 0 = indisponivel (usa a fonte do console)
	int g_hFont[3] = { 0, 0, 0 };
	bool g_bFontTried[3] = { false, false, false };

	bool EnsureFont( int idx )
	{
		if ( !g_bFontTried[idx] )
		{
			g_bFontTried[idx] = true;
			int h = gEngfuncs.pfnSPR_Load( g_HlextFontAtlas[idx].pszPath );
			if ( h && gEngfuncs.pfnSPR_Frames( h ) >= 224 )
				g_hFont[idx] = h;
			HLEXT_Log( "legendas: fonte %s %s", g_HlextFontAtlas[idx].pszPath, g_hFont[idx] ? "carregada" : "nao encontrada (usando fonte do console)" );
		}
		return g_hFont[idx] != 0;
	}

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

			// "\n" literal (barra + n) vira espaco; a quebra de linha e feita pelo desenho
			for ( size_t pos = 0; ( pos = text.find( "\\n", pos ) ) != std::string::npos; )
				text.replace( pos, 2, " " );
			for ( size_t pos = 0; ( pos = text.find( "  ", pos ) ) != std::string::npos; )
				text.erase( pos, 1 );

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
	cl_subtitle_scale = CVAR_CREATE( "cl_subtitle_scale", "1.5", FCVAR_ARCHIVE );
	cl_subtitle_size = CVAR_CREATE( "cl_subtitle_size", "1.0", FCVAR_ARCHIVE );

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
	a.start = gHUD.m_flTime;
	a.r = e.r; a.g = e.g; a.b = e.b;
	g_Active.push_back( a );

	if ( (int)g_Active.size() > MAX_ACTIVE )
		g_Active.erase( g_Active.begin() );

	return 1;
}

namespace
{
	// Aumenta a fonte do console so enquanto desenhamos as legendas (con_fontscale do Xash).
	struct FontScaleGuard
	{
		cvar_t *p;
		float old;
		explicit FontScaleGuard( float mult )
		{
			p = gEngfuncs.pfnGetCvarPointer( "con_fontscale" );
			old = p ? p->value : 0.0f;
			if ( p && old > 0.0f && mult > 0.1f && mult < 6.0f )
				p->value = old * mult;
			else
				p = nullptr;
		}
		~FontScaleGuard()
		{
			if ( p )
				p->value = old;
		}
	};

	inline int LerpI( int a, int b, float t )
	{
		return (int)( a + ( b - a ) * t + 0.5f );
	}

	inline float Clamp01( float v )
	{
		return v < 0.0f ? 0.0f : ( v > 1.0f ? 1.0f : v );
	}

	// retangulo com cantos arredondados (3 retangulos sobrepostos)
	void FillRounded( int x, int y, int w, int h, int r, int g, int b, int a )
	{
		if ( a <= 0 ) return;
		int c = (int)( ScreenHeight * 0.008f );
		if ( c < 3 ) c = 3;
		if ( c * 2 > h ) c = h / 2;
		FillRGBA( x + c, y, w - 2 * c, h, r, g, b, a );
		FillRGBA( x, y + c, w, h - 2 * c, r, g, b, a );
		FillRGBA( x + c / 2, y + c / 2, w - c, h - c, r, g, b, a );
	}
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

	// escolhe o atlas de fonte (sprite) mais proximo do tamanho desejado
	const HlextFontAtlas_t *atl = nullptr;
	int hSpr = 0;
	{
		float flSize = cl_subtitle_size ? cl_subtitle_size->value : 1.0f;
		if ( flSize < 0.3f ) flSize = 0.3f;
		float flTarget = ScreenHeight * 0.028f * flSize;
		int order[3] = { 0, 1, 2 };
		std::sort( order, order + 3, [flTarget]( int a, int b ) {
			return fabsf( g_HlextFontAtlas[a].px - flTarget ) < fabsf( g_HlextFontAtlas[b].px - flTarget ); } );
		for ( int k = 0; k < 3; k++ )
		{
			if ( EnsureFont( order[k] ) )
			{
				atl = &g_HlextFontAtlas[ order[k] ];
				hSpr = g_hFont[ order[k] ];
				break;
			}
		}
	}
	const bool useSprite = atl != nullptr;

	// fonte do console (reserva): aumenta o con_fontscale so enquanto desenha
	FontScaleGuard fontScale( useSprite ? 0.0f : ( cl_subtitle_scale ? cl_subtitle_scale->value : 1.0f ) );

	int lineH;
	if ( useSprite )
	{
		lineH = atl->lineH;
	}
	else
	{
		int _w;
		GetConsoleStringSize( "Ag", &_w, &lineH );
		if ( lineH <= 0 ) lineH = 14;
	}

	const int pad = lineH / 2 + 2;
	const int maxWidth = (int)( ScreenWidth * 0.62f );

	// largura de cada caractere (sem kerning, entao a soma e a largura da linha)
	int charW[256];
	for ( int c = 0; c < 256; c++ )
	{
		if ( c < 32 )
			charW[c] = 0;
		else if ( useSprite )
			charW[c] = atl->advance[c - 32];
		else
		{
			char buf[2] = { (char)c, 0 };
			charW[c] = ConsoleStringLen( buf );
		}
	}
	auto strW = [&]( const std::string &t )
	{
		int w = 0;
		for ( size_t i = 0; i < t.size(); i++ ) w += charW[(unsigned char)t[i]];
		return w;
	};

	// desenha um caractere na posicao (cx, ly = topo da linha); r,g,b em 0..255
	auto drawGlyph = [&]( int cx, int ly, unsigned char ch, int r, int g, int b )
	{
		if ( ch <= 32 )
			return;
		if ( useSprite )
		{
			gEngfuncs.pfnSPR_Set( hSpr, r, g, b );
			gEngfuncs.pfnSPR_DrawAdditive( ch - 32, cx - atl->padX, ly + atl->ascent - atl->baseline, NULL );
		}
		else
		{
			char buf[2] = { (char)ch, 0 };
			gEngfuncs.pfnDrawSetTextColor( r / 255.0f, g / 255.0f, b / 255.0f );
			DrawConsoleString( cx, ly, buf );
		}
	};

	int bgR = cl_subtitle_red ? (int)cl_subtitle_red->value : 30;
	int bgG = cl_subtitle_green ? (int)cl_subtitle_green->value : 30;
	int bgB = cl_subtitle_blue ? (int)cl_subtitle_blue->value : 30;

	// quebra de linhas de cada legenda
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
					if ( !cur.empty() && strW( test ) > maxWidth - 2 * pad )
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

		totalH += (int)out.size() * lineH + 2 * pad + 4;
		blocks.push_back( out );
	}

	int y = (int)( ScreenHeight * 0.86f ) - totalH;
	if ( y < 0 ) y = 0;

	for ( size_t n = 0; n < blocks.size(); n++ )
	{
		const std::vector<std::string> &ls = blocks[n];
		const SubActive &act = g_Active[n];

		float age = flTime - act.start;
		float life = act.expire - act.start;
		if ( life < 0.5f ) life = 0.5f;

		// aparece e some suavemente
		float aOut = Clamp01( ( act.expire - flTime ) / 0.45f );
		float aBox = Clamp01( age / 0.18f ) * aOut;

		// texto "revelado" da esquerda para a direita, linha por linha
		int nChars = 0;
		for ( size_t i = 0; i < ls.size(); i++ ) nChars += (int)ls[i].size() + 1;
		const float soft = 10.0f;
		float revealDur = 0.03f * nChars;
		if ( revealDur < 0.4f ) revealDur = 0.4f;
		if ( revealDur > life * 0.55f ) revealDur = life * 0.55f;
		float pos = ( age / revealDur ) * ( nChars + soft );

		int w = 0;
		for ( size_t i = 0; i < ls.size(); i++ )
			w = std::max( w, strW( ls[i] ) );

		int boxW = w + 2 * pad;
		int boxH = (int)ls.size() * lineH + 2 * pad;
		int x = ( ScreenWidth - boxW ) / 2;

		FillRounded( x, y, boxW, boxH, bgR, bgG, bgB, (int)( 185.0f * aBox ) );

		int idx0 = 0;
		for ( size_t i = 0; i < ls.size(); i++ )
		{
			const std::string &line = ls[i];
			int lw = strW( line );
			int lx = ( ScreenWidth - lw ) / 2;
			int ly = y + pad + (int)i * lineH;

			float aFirst = Clamp01( ( pos - idx0 ) / soft );
			float aLast = Clamp01( ( pos - ( idx0 + (int)line.size() - 1 ) ) / soft );

			if ( aFirst <= 0.0f )
			{
				// ainda nao chegou nesta linha
			}
			else if ( aLast >= 1.0f )
			{
				// linha inteira revelada: uma chamada so, com a cor desta linha
				float a = aOut;
				if ( useSprite )
				{
					int tr = (int)( act.r * a ), tg = (int)( act.g * a ), tb = (int)( act.b * a );
					int cx = lx;
					for ( size_t c = 0; c < line.size(); c++ )
					{
						drawGlyph( cx, ly, (unsigned char)line[c], tr, tg, tb );
						cx += charW[(unsigned char)line[c]];
					}
				}
				else
				{
					gEngfuncs.pfnDrawSetTextColor( LerpI( bgR, act.r, a ) / 255.0f, LerpI( bgG, act.g, a ) / 255.0f, LerpI( bgB, act.b, a ) / 255.0f );
					DrawConsoleString( lx, ly, line.c_str() );
				}
			}
			else
			{
				// linha em fade: caractere por caractere (o engine reseta a cor a cada desenho)
				int cx = lx;
				for ( size_t c = 0; c < line.size(); c++ )
				{
					float a = Clamp01( ( pos - ( idx0 + (int)c ) ) / soft ) * aOut;
					unsigned char ch = (unsigned char)line[c];
					if ( a > 0.02f && ch > 32 )
					{
						if ( useSprite )
							drawGlyph( cx, ly, ch, (int)( act.r * a ), (int)( act.g * a ), (int)( act.b * a ) );
						else
							drawGlyph( cx, ly, ch, LerpI( bgR, act.r, a ), LerpI( bgG, act.g, a ), LerpI( bgB, act.b, a ) );
					}
					cx += charW[ch];
				}
			}

			idx0 += (int)line.size() + 1;
		}

		y += boxH + 4;
	}

	return 1;
}
