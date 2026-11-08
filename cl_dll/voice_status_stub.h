// Stub do sistema de voz (voice_status) para o build Android sem VGUI.
// O chat de voz do Xash3D e tratado pelo engine; aqui so mantemos as chamadas compilando.
#ifndef AZ_ANDROID_VOICE_STATUS_STUB_H
#define AZ_ANDROID_VOICE_STATUS_STUB_H

#include "vgui_stub.h"

// Fornecido por cada mod (hud.cpp implementa CHLVoiceStatusHelper).
class IVoiceStatusHelper
{
public:
	virtual ~IVoiceStatusHelper() {}
	virtual void GetPlayerTextColor(int entindex, int color[3]) = 0;
	virtual void UpdateCursorState() = 0;
	virtual int  GetAckIconHeight() = 0;
	virtual bool CanShowSpeakerLabels() = 0;
};

class CVoiceStatus
{
public:
	int  Init(IVoiceStatusHelper*, vgui::Panel**) { return 1; }
	int  VidInit() { return 1; }
	void Frame(double) {}
	void UpdateSpeakerStatus(int, qboolean) {}
	void CreateEntities() {}
	void HandleVoiceMaskMsg(int, void*) {}
	void HandleReqStateMsg(int, void*) {}
	void StartSquelchMode() {}
	void StopSquelchMode() {}
	bool IsInSquelchMode() { return false; }
	bool IsPlayerBlocked(int) { return false; }
	bool IsPlayerAudible(int) { return true; }
	void SetPlayerBlockedState(int, bool) {}
};

inline CVoiceStatus* GetClientVoiceMgr()
{
	static CVoiceStatus s_voice;
	return &s_voice;
}

#endif
