// Sistema de logs do Invasion (Android).
//
//  INV_Log        grava uma linha em invasion_log.txt (e no logcat, tag "INV") e guarda no historico.
//  INV_Breadcrumb guarda a linha so em memoria (barato, pode ser chamado a todo momento).
//
// Se o jogo cair (SIGSEGV, SIGBUS, SIGABRT, SIGFPE, SIGILL), inv_crash.cpp grava em invasion_crash.log:
// sinal, endereco, registradores, cadeia de chamadas e as ultimas linhas do historico.
#ifndef INV_LOG_H
#define INV_LOG_H

#if defined( __GNUC__ )
void INV_Log( const char *fmt, ... ) __attribute__(( format( printf, 1, 2 ) ));
void INV_Breadcrumb( const char *fmt, ... ) __attribute__(( format( printf, 1, 2 ) ));
#else
void INV_Log( const char *fmt, ... );
void INV_Breadcrumb( const char *fmt, ... );
#endif

// Marcadores permanentes (arquivos vazios na pasta de trabalho do Xash, ex.: /sdcard/xash/).
int INV_FlagExists( const char *name );   // 1 se o marcador existe
void INV_FlagSet( const char *name );     // cria o marcador

#endif // INV_LOG_H
