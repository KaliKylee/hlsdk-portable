// Stub: o traceroute ICMP original (Windows) foi removido no port Android/Xash3D.
// O HUD de benchmark continua funcionando; apenas o traceroute termina imediatamente.
#include "hud_benchtrace.h"

void Trace_StartTrace( int *results, int *finished, const char *server )
{
	if ( results )
		*results = 0;
	if ( finished )
		*finished = 1;
}

void Trace_Think( void )
{
}
