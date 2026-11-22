// Sistema de logs do Half-Rats: Parasomnia (Android).
//
//  HR_Log        grava uma linha em hr_log.txt (e no logcat, tag "HR") e guarda no historico.
//  HR_Breadcrumb guarda a linha so em memoria (barato, pode ser chamado a todo momento).
//
// Se o jogo cair (SIGSEGV, SIGBUS, SIGABRT, SIGFPE, SIGILL), hr_crash.cpp grava em hr_crash.log:
// sinal, endereco, registradores, cadeia de chamadas e as ultimas linhas do historico.
#ifndef HR_LOG_H
#define HR_LOG_H

#if defined( __GNUC__ )
void HR_Log( const char *fmt, ... ) __attribute__(( format( printf, 1, 2 ) ));
void HR_Breadcrumb( const char *fmt, ... ) __attribute__(( format( printf, 1, 2 ) ));
#else
void HR_Log( const char *fmt, ... );
void HR_Breadcrumb( const char *fmt, ... );
#endif

// Marcadores permanentes (arquivos vazios na pasta de trabalho do Xash, ex.: /sdcard/xash/).
int HR_FlagExists( const char *name );   // 1 se o marcador existe
void HR_FlagSet( const char *name );     // cria o marcador

#endif // HR_LOG_H
