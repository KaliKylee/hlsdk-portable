// Sistema de logs do Half-Force (Android).
//
//  HF_Log        grava uma linha em hf_log.txt (e no logcat, tag "HF") e guarda no historico.
//  HF_Breadcrumb guarda a linha so em memoria (barato, pode ser chamado a todo momento).
//
// Se o jogo cair (SIGSEGV, SIGBUS, SIGABRT, SIGFPE, SIGILL), hf_crash.cpp grava em hf_crash.log:
// sinal, endereco, registradores, cadeia de chamadas e as ultimas linhas do historico.
#ifndef HF_LOG_H
#define HF_LOG_H

#if defined( __GNUC__ )
void HF_Log( const char *fmt, ... ) __attribute__(( format( printf, 1, 2 ) ));
void HF_Breadcrumb( const char *fmt, ... ) __attribute__(( format( printf, 1, 2 ) ));
#else
void HF_Log( const char *fmt, ... );
void HF_Breadcrumb( const char *fmt, ... );
#endif

#endif // HF_LOG_H
