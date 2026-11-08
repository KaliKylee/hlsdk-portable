// Input para Android/Xash3D FWGS (substitui inputw32.cpp, que usa Win32/DirectInput).
// Gamepad e toque sao tratados pelo proprio engine (IN_EngineAppendMove), entao aqui
// fica so o mouse (USB/Bluetooth) e as funcoes que o resto do client espera.
#include "external_lib_include.h"
#include "hud.h"
#include "cl_util.h"
#include "camera.h"
#include "kbutton.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include "../engine/keydefs.h"
#include "view.h"

#define MOUSE_BUTTON_COUNT 5

int g_iVisibleMouse = 0;

extern "C"
{
	void DLLEXPORT IN_ActivateMouse( void );
	void DLLEXPORT IN_DeactivateMouse( void );
	void DLLEXPORT IN_MouseEvent (int mstate);
	void DLLEXPORT IN_Accumulate (void);
	void DLLEXPORT IN_ClearStates (void);
}

extern cl_enginefunc_t gEngfuncs;
extern int iMouseInUse;

extern kbutton_t in_strafe;
extern kbutton_t in_mlook;
extern kbutton_t in_speed;
extern kbutton_t in_jlook;

extern cvar_t *m_pitch;
extern cvar_t *m_yaw;
extern cvar_t *m_forward;
extern cvar_t *m_side;
extern cvar_t *lookstrafe;
extern cvar_t *lookspring;
extern cvar_t *cl_pitchdown;
extern cvar_t *cl_pitchup;

cvar_t *m_filter;
cvar_t *sensitivity;
cvar_t *in_joystick; // lido em input.cpp; gamepad real e tratado pelo engine

int mouse_buttons;
int mouse_oldbuttonstate;
int mouse_x, mouse_y, old_mouse_x, old_mouse_y, mx_accum, my_accum;
int mouseinitialized;

static int mouseactive;
static int s_lastX, s_lastY;
static bool s_haveLast;

// Simbolos que cl_util.cpp espera (opcao m_rawinput do mod; sem efeito no Android).
void determineMouseParams(float) {}
void onUpdateRawInput(void) {}

void Force_CenterView_f (void)
{
	vec3_t viewangles;

	if (!iMouseInUse)
	{
		gEngfuncs.GetViewAngles( (float *)viewangles );
		viewangles[PITCH] = 0;
		gEngfuncs.SetViewAngles( (float *)viewangles );
	}
}

// Delta do mouse desde a ultima leitura (o engine entrega posicao em coordenadas da janela).
static void IN_ReadMouseDelta(int *dx, int *dy)
{
	int x, y;
	gEngfuncs.GetMousePosition(&x, &y);
	if (!s_haveLast) { s_lastX = x; s_lastY = y; s_haveLast = true; }
	*dx = x - s_lastX;
	*dy = y - s_lastY;
	s_lastX = x;
	s_lastY = y;
}

void DLLEXPORT IN_ActivateMouse (void)
{
	if (mouseinitialized)
	{
		mouseactive = 1;
		s_haveLast = false;
	}
}

void DLLEXPORT IN_DeactivateMouse (void)
{
	if (mouseinitialized)
		mouseactive = 0;
}

void IN_StartupMouse (void)
{
	if ( gEngfuncs.CheckParm ("-nomouse", NULL ) )
		return;

	mouseinitialized = 1;
	mouse_buttons = MOUSE_BUTTON_COUNT;
}

void IN_Shutdown (void)
{
	IN_DeactivateMouse ();
}

void IN_GetMousePos( int *mx, int *my )
{
	gEngfuncs.GetMousePosition( mx, my );
}

void IN_ResetMouse( void )
{
	// O engine controla o cursor no Android; so reinicia a referencia do delta.
	s_haveLast = false;
}

void DLLEXPORT IN_MouseEvent (int mstate)
{
	int i;

	if ( iMouseInUse || g_iVisibleMouse )
		return;

	for (i = 0; i < mouse_buttons; i++)
	{
		if ( (mstate & (1<<i)) && !(mouse_oldbuttonstate & (1<<i)) )
			gEngfuncs.Key_Event (K_MOUSE1 + i, 1);

		if ( !(mstate & (1<<i)) && (mouse_oldbuttonstate & (1<<i)) )
			gEngfuncs.Key_Event (K_MOUSE1 + i, 0);
	}

	mouse_oldbuttonstate = mstate;
}

void IN_MouseMove ( float frametime, usercmd_t *cmd)
{
	int mx, my;
	vec3_t viewangles;

	gEngfuncs.GetViewAngles( (float *)viewangles );

	if ( in_mlook.state & 1)
		V_StopPitchDrift ();

	if ( !iMouseInUse && !g_iVisibleMouse && !gHUD.m_iIntermission )
	{
		int dx, dy;
		IN_ReadMouseDelta(&dx, &dy);

		mx = dx + mx_accum;
		my = dy + my_accum;
		mx_accum = 0;
		my_accum = 0;

		if (m_filter->value)
		{
			mouse_x = (mx + old_mouse_x) * 0.5;
			mouse_y = (my + old_mouse_y) * 0.5;
		}
		else
		{
			mouse_x = mx;
			mouse_y = my;
		}

		old_mouse_x = mx;
		old_mouse_y = my;

		if ( gHUD.GetSensitivity() != 0 )
		{
			mouse_x *= gHUD.GetSensitivity();
			mouse_y *= gHUD.GetSensitivity();
		}
		else
		{
			mouse_x *= sensitivity->value;
			mouse_y *= sensitivity->value;
		}

		if ( (in_strafe.state & 1) || (lookstrafe->value && (in_mlook.state & 1) ))
			cmd->sidemove += m_side->value * mouse_x;
		else
			viewangles[YAW] -= m_yaw->value * mouse_x;

		if ( (in_mlook.state & 1) && !(in_strafe.state & 1))
		{
			viewangles[PITCH] += m_pitch->value * mouse_y;
			if (viewangles[PITCH] > cl_pitchdown->value)
				viewangles[PITCH] = cl_pitchdown->value;
			if (viewangles[PITCH] < -cl_pitchup->value)
				viewangles[PITCH] = -cl_pitchup->value;
		}
		else
		{
			if ((in_strafe.state & 1) && gEngfuncs.IsNoClipping() )
				cmd->upmove -= m_forward->value * mouse_y;
			else
				cmd->forwardmove -= m_forward->value * mouse_y;
		}
	}

	gEngfuncs.SetViewAngles( (float *)viewangles );
}

void DLLEXPORT IN_Accumulate (void)
{
	if ( !iMouseInUse && !g_iVisibleMouse && mouseactive )
	{
		int dx, dy;
		IN_ReadMouseDelta(&dx, &dy);
		mx_accum += dx;
		my_accum += dy;
	}
}

void DLLEXPORT IN_ClearStates (void)
{
	if ( !mouseactive )
		return;

	mx_accum = 0;
	my_accum = 0;
	mouse_oldbuttonstate = 0;
	s_haveLast = false;
}

// Botoes de joystick/gamepad: tratados pelo engine.
void IN_Commands (void) {}

void IN_Move ( float frametime, usercmd_t *cmd)
{
	if ( !iMouseInUse && mouseactive )
		IN_MouseMove ( frametime, cmd);
}

void IN_Init (void)
{
	m_filter    = gEngfuncs.pfnRegisterVariable ( "m_filter","0", FCVAR_ARCHIVE );
	sensitivity = gEngfuncs.pfnRegisterVariable ( "sensitivity","3", FCVAR_ARCHIVE );

	in_joystick = gEngfuncs.pfnRegisterVariable ( "joystick","0", FCVAR_ARCHIVE );

	gEngfuncs.pfnAddCommand ("force_centerview", Force_CenterView_f);

	IN_StartupMouse ();
}
