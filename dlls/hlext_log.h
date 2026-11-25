// Sistema de logs do Half-Life: Extended (Android).
//
//  HLEXT_Log        grava uma linha em hlext_log.txt (e no logcat, tag "HLEXT") e guarda no historico.
//  HLEXT_Breadcrumb guarda a linha so em memoria (barato, pode ser chamado a todo momento).
//
// Se o jogo cair (SIGSEGV, SIGBUS, SIGABRT, SIGFPE, SIGILL), hlext_crash.cpp grava em hlext_crash.log:
// sinal, endereco, registradores, cadeia de chamadas e as ultimas linhas do historico.
#ifndef HLEXT_LOG_H
#define HLEXT_LOG_H

#if defined( __GNUC__ )
void HLEXT_Log( const char *fmt, ... ) __attribute__(( format( printf, 1, 2 ) ));
void HLEXT_Breadcrumb( const char *fmt, ... ) __attribute__(( format( printf, 1, 2 ) ));
#else
void HLEXT_Log( const char *fmt, ... );
void HLEXT_Breadcrumb( const char *fmt, ... );
#endif

// Marcadores permanentes (arquivos vazios na pasta de trabalho do Xash, ex.: /sdcard/xash/).
int HLEXT_FlagExists( const char *name );   // 1 se o marcador existe
void HLEXT_FlagSet( const char *name );     // cria o marcador

#endif // HLEXT_LOG_H
