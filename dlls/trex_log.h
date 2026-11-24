// Sistema de logs do ThrillEX (Android).
//
//  TREX_Log        grava uma linha em trex_log.txt (e no logcat, tag "TREX") e guarda no historico.
//  TREX_Breadcrumb guarda a linha so em memoria (barato, pode ser chamado a todo momento).
//
// Se o jogo cair (SIGSEGV, SIGBUS, SIGABRT, SIGFPE, SIGILL), trex_crash.cpp grava em trex_crash.log:
// sinal, endereco, registradores, cadeia de chamadas e as ultimas linhas do historico.
#ifndef TREX_LOG_H
#define TREX_LOG_H

#if defined( __GNUC__ )
void TREX_Log( const char *fmt, ... ) __attribute__(( format( printf, 1, 2 ) ));
void TREX_Breadcrumb( const char *fmt, ... ) __attribute__(( format( printf, 1, 2 ) ));
#else
void TREX_Log( const char *fmt, ... );
void TREX_Breadcrumb( const char *fmt, ... );
#endif

// Marcadores permanentes (arquivos vazios na pasta de trabalho do Xash, ex.: /sdcard/xash/).
int TREX_FlagExists( const char *name );   // 1 se o marcador existe
void TREX_FlagSet( const char *name );     // cria o marcador

#endif // TREX_LOG_H
