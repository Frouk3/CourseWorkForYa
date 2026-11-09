#include <Events.h>
#include <Hooks.h>
#include <Hw.h>
#include <Entity.h>
#include <cSlowRateManager.h>
#include "TimeStop.h"
#include <Behavior.h>
#include <Fw.h>
#include <cScene.h>

#define _DEBUGMODE 1
#if _DEBUGMODE
#pragma comment(lib, "d3dx9.lib")
LPD3DXFONT g_pFont = nullptr;

// Draws text on the screen at the specified position with given color
// NOTE: should've made a ellipsis version of this func
void DrawTextOnScreen(const char* text, int x, int y, D3DCOLOR color)
{
	if (!g_pFont)
		return;

	RECT rect;
	rect.left = x;
	rect.top = y;

	g_pFont->DrawTextA(nullptr, text, -1, &rect, DT_NOCLIP, color);
}

#endif

class TimeStopPlugin
{
	bool m_bActive = false;
public:
	bool IsActive() const { return m_bActive; }

	TimeStopPlugin()
	{
#if _DEBUGMODE
		// create font for debugging
		Events::OnGameStartupEvent += [this](cGame*)
		{
			HRESULT hr = D3DXCreateFontA(
				Hw::GraphicDevice::m_pDevice,
				20, 0, FW_NORMAL, 1, FALSE, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
				"Arial", &g_pFont);

			if (FAILED(hr))
			{
				MessageBoxA(nullptr, "Failed to create font.", "Error", MB_OK | MB_ICONERROR);
			}
		};

		// on device reset, release it first
		Events::OnDeviceReset.before += [this]()
		{
			if (g_pFont)
			{
				g_pFont->OnLostDevice();
			}
		};

		// on device reset after, reset it
		Events::OnDeviceReset.after += [this]()
		{
			if (g_pFont)
			{
				g_pFont->OnResetDevice();
			}
		};

		// end scene, draw text on the screen
		Events::OnEndScene += [this]()
		{
			if (g_pFont)
			{
				char buffer[256] = { 0 };
				// formats the buffer in old C style
				sprintf_s(buffer, "Time Stop Active: %s", m_bActive ? "Yes" : "No");

				DrawTextOnScreen(buffer, 10, 400, D3DCOLOR_ARGB(255, 255, 255, 255));

				sprintf_s(buffer, "Time Scale: %f", TimeStopController::GetInstance().GetTimeInter());

				DrawTextOnScreen(buffer, 10, 430, D3DCOLOR_ARGB(255, 255, 255, 255));
			}
		};

#endif

		// On each in game tick, no process on paused state
		Events::OnTickEvent += [this]()
		{
			if (!g_Scene.m_pPlayer) // Do not process if the player is not initialized
				return;

			TimeStopController* timeStopController = &TimeStopController::GetInstance();
			TimeStopEffectController* timeStopEffectController = &TimeStopEffectController::GetInstance();

			if (g_Keyboard.trig('T'))
			{
				m_bActive ^= true;

				timeStopController->SetEnable(m_bActive);
				timeStopEffectController->SetEnabled(m_bActive);
			}

			timeStopEffectController->OnTickEvent();
			timeStopController->OnTickEvent();
		};

		// On scene cleanup, reset the time stop state
		Events::OnSceneCleanupEvent += [this](void *)
		{
			TimeStopController* timeStopController = &TimeStopController::GetInstance();
			TimeStopEffectController* timeStopEffectController = &TimeStopEffectController::GetInstance();

			m_bActive = false;

			timeStopController->SetEnable(false);
			timeStopEffectController->SetEnabled(false);
		};
	}
} TimeStop;

// Even though this method is whacky, it works
CREATE_THISCALL(false, shared::base + 0x68BF50, int, Behavior_ckMove, Behavior*)
{
	if (TimeStop.IsActive() && (pThis->m_ObjId >> 16) != 1) // get category from object, and check if it's not a player object
		return 0;

	return original(pThis);
}