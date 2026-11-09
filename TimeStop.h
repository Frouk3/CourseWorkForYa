#pragma once

#include <Hw.h>
#include <cLightManager.h>
#include <cRoomResourceManager.h>
#include <cSlowRateManager.h>
#include <cGame.h>

static inline constexpr float g_InterRate = 0.3f; // Interpolation rate for smooth transitions

class TimeStopEffectController : public Hw::cSingleton<TimeStopEffectController>
{
	// Snapshot of cLightDataMinimum
	struct LightSnapshot
	{
		Hw::cFcol ambient[8];
		Hw::cFcol directional[8];
		Hw::cFcol hemisphereSky[8];
		Hw::cFcol hemisphereGround[8];
		Hw::cFcol noiseCol;
		float backLightRate;
		Hw::cVec3 lightMapShadowPow;
	};

	// Snapshot of resources of the cRoomResourceManager
	struct RoomSnapshot
	{
		Hw::cFcol color;
		Hw::cFcol backgroundColor;
		Hw::cFcol glareColor;
		Hw::cFcol fogColor;
	};

	// Snapshot of parameters of the cSceneLightingParams
	struct ParamSnapshot
	{
		Hw::cFcol saido;
		Hw::cFcol color;
		Hw::cFcol bgColor;
		Hw::cFcol glareColor;
		Hw::cFcol addColor;
	};

	LightSnapshot m_LightSnapshot{};
	RoomSnapshot  m_RoomSnapshot{};
	ParamSnapshot m_ParamSnapshot{};

	bool  m_Enabled = false;
	bool  m_HasSnapshot = false;
	float m_Amount = 0.0f;

	// Interpolates between two colors based on the given factor t (0.0 to 1.0)
	static Hw::cFcol LerpColor(const Hw::cFcol& from, const Hw::cFcol& to, float t)
	{
		Hw::cFcol result;
		result.r = from.r + (to.r - from.r) * t;
		result.g = from.g + (to.g - from.g) * t;
		result.b = from.b + (to.b - from.b) * t;
		result.a = from.a + (to.a - from.a) * t;
		return result;
	}

	// Applies a blue tint to the given color based on the specified factor (0.0 to 1.0)
	static Hw::cFcol TintBlue(const Hw::cFcol& color, float factor)
	{
		float gray = color.r * 0.299f + color.g * 0.587f + color.b * 0.114f;

		Hw::cFcol blueTarget;
		blueTarget.r = gray * 0.15f;
		blueTarget.g = gray * 0.45f;
		blueTarget.b = gray * 1.15f;
		blueTarget.a = color.a;

		return LerpColor(color, blueTarget, factor);
	}

	// Takes a snapshot of the current lighting and room parameters
	void TakeSnapshot()
	{
		if (m_HasSnapshot)
			return;

		cLightDataMinimum* light = &g_LightMng.m_RoomLighting;
		for (int i = 0; i < 8; i++)
		{
			m_LightSnapshot.ambient[i] = *(Hw::cFcol*)&light->m_AmbCol[i];
			m_LightSnapshot.directional[i] = *(Hw::cFcol*)&light->m_DirCol[i];
			m_LightSnapshot.hemisphereSky[i] = light->m_HemisphereSkyCol[i];
			m_LightSnapshot.hemisphereGround[i] = *(Hw::cFcol*)&light->m_HemisphereGroundCol[i];
		}
		m_LightSnapshot.noiseCol = *(Hw::cFcol*)&light->m_noiseCol;
		m_LightSnapshot.backLightRate = light->m_backLightRate;
		m_LightSnapshot.lightMapShadowPow = light->m_LightMapShadowPow;

		m_RoomSnapshot.color = g_RoomResourceManager.m_Color;
		m_RoomSnapshot.backgroundColor = g_RoomResourceManager.m_BackgroundColor;
		m_RoomSnapshot.glareColor = g_RoomResourceManager.m_GlareColor;
		m_RoomSnapshot.fogColor = g_RoomResourceManager.m_FogColor;

		m_ParamSnapshot.saido = g_LightParams.m_Saido;
		m_ParamSnapshot.color = g_LightParams.m_Color;
		m_ParamSnapshot.bgColor = g_LightParams.m_BgColor;
		m_ParamSnapshot.glareColor = g_LightParams.m_GlareColor;
		m_ParamSnapshot.addColor = g_LightParams.m_AddColor;

		m_HasSnapshot = true;
	}

	// Restores the lighting and room parameters from the snapshot
	void ReleaseSnapshotLight()
	{
		if (!m_HasSnapshot)
			return;

		int no = g_LightMng.m_RoomLighting.m_no;
		cLightDataMinimum* light = &g_LightParams.field_200;

		for (auto& lightData : g_LightParams.field_DC)
		{
			if (lightData.m_no == no)
			{
				light = &lightData;
				break;
			}
		}

		g_LightMng.m_RoomLighting = *light;

		g_RoomResourceManager.m_Color = m_RoomSnapshot.color;
		g_RoomResourceManager.m_BackgroundColor = m_RoomSnapshot.backgroundColor;
		g_RoomResourceManager.m_GlareColor = m_RoomSnapshot.glareColor;
		g_RoomResourceManager.m_FogColor = m_RoomSnapshot.fogColor;

		g_LightParams.m_Saido = m_ParamSnapshot.saido;
		g_LightParams.m_Color = m_ParamSnapshot.color;
		g_LightParams.m_BgColor = m_ParamSnapshot.bgColor;
		g_LightParams.m_GlareColor = m_ParamSnapshot.glareColor;
		g_LightParams.m_AddColor = m_ParamSnapshot.addColor;

		m_HasSnapshot = false;
	}
	
	// Applies smoothing interpolation to the lighting and room parameters based on the current amount
	void ApplyInterpolation()
	{
		if (!m_HasSnapshot)
			return;

		cLightDataMinimum* light = &g_LightMng.m_RoomLighting;
		for (int i = 0; i < 8; i++)
		{
			*(Hw::cFcol*)&light->m_AmbCol[i] = TintBlue(m_LightSnapshot.ambient[i], m_Amount);
			*(Hw::cFcol*)&light->m_DirCol[i] = TintBlue(m_LightSnapshot.directional[i], m_Amount);
			light->m_HemisphereSkyCol[i] = TintBlue(m_LightSnapshot.hemisphereSky[i], m_Amount);
			*(Hw::cFcol*)&light->m_HemisphereGroundCol[i] = TintBlue(m_LightSnapshot.hemisphereGround[i], m_Amount);
		}
		*(Hw::cFcol*)&light->m_noiseCol = TintBlue(m_LightSnapshot.noiseCol, m_Amount);

		g_RoomResourceManager.m_Color = TintBlue(m_RoomSnapshot.color, m_Amount);
		g_RoomResourceManager.m_BackgroundColor = TintBlue(m_RoomSnapshot.backgroundColor, m_Amount);
		g_RoomResourceManager.m_GlareColor = TintBlue(m_RoomSnapshot.glareColor, m_Amount);

		Hw::cFcol targetFog(0.05f, 0.25f, 0.65f, m_RoomSnapshot.fogColor.a);
		g_RoomResourceManager.m_FogColor = LerpColor(m_RoomSnapshot.fogColor, targetFog, m_Amount);

		Hw::cFcol targetSaido(0.3f, 0.5f, 1.1f, m_ParamSnapshot.saido.a);
		g_LightParams.m_Saido = LerpColor(m_ParamSnapshot.saido, targetSaido, m_Amount);

		Hw::cFcol targetColor(0.25f, 0.45f, 0.95f, m_ParamSnapshot.color.a);
		g_LightParams.m_Color = LerpColor(m_ParamSnapshot.color, targetColor, m_Amount);

		g_LightParams.m_BgColor = TintBlue(m_ParamSnapshot.bgColor, m_Amount);
		g_LightParams.m_GlareColor = TintBlue(m_ParamSnapshot.glareColor, m_Amount);

		Hw::cFcol targetAddColor(0.0f, 0.08f, 0.25f, m_ParamSnapshot.addColor.a);
		g_LightParams.m_AddColor = LerpColor(m_ParamSnapshot.addColor, targetAddColor, m_Amount);
	}

public:
	TimeStopEffectController() : m_Enabled(false), m_HasSnapshot(false), m_Amount(0.0f) {}

	void SetEnabled(bool enabled) { m_Enabled = enabled; }
	float GetAmount() const { return m_Amount; }

	// Added on OnTickEvent
	void OnTickEvent()
	{
		if (m_Enabled && !m_HasSnapshot)
		{
			TakeSnapshot();
		}

		float targetAmount = m_Enabled ? 1.0f : 0.0f;
		m_Amount += (targetAmount - m_Amount) * g_InterRate;

		if (m_Amount < 0.01f)
		{
			m_Amount = 0.0f;
		}
		else if (m_Amount > 0.99f)
		{
			m_Amount = 1.0f;
		}

		if (m_HasSnapshot)
		{
			ApplyInterpolation();

			if (!m_Enabled && m_Amount == 0.0f) // If disabled and fully interpolated
			{
				ReleaseSnapshotLight();
			}
		}
	}
};

class TimeStopController : public Hw::cSingleton<TimeStopController>
{
	static inline constexpr float m_TimeStopScale = 0.0000001f;
	static inline constexpr float m_TimeResumeScale = 1.0f;

	float m_TimeScale;
	float m_TimeInter;
	bool m_bInter = false;
	bool m_bActive = false;

	void ApplyInterpolation()
	{
		if (!m_bInter)
			return;

		m_TimeInter += (m_TimeScale - m_TimeInter) * g_InterRate;

		if (m_TimeInter < 0.0001f)
		{
			OnInterpolationFinish(true);
			m_TimeInter = m_TimeStopScale;
			m_bInter = false;
		}
		else if (m_TimeInter > 0.999f)
		{
			OnInterpolationFinish(false);
			m_TimeInter = m_TimeResumeScale;
			m_bInter = false;
		}
	}

	// Apply stop flags
	void OnInterpolationFinish(bool toggle)
	{
		g_StpFlags.STP_EM_BRAIN = toggle;
		g_StpFlags.STP_SCENARIO = toggle;
		g_StpFlags.STP_TRIGGER = toggle;
	}

public:
	TimeStopController() : m_TimeScale(1.0f), m_TimeInter(1.0f), m_bActive(false) {}

	float GetTimeScale() const { return m_TimeScale; }
	float GetTimeInter() const { return m_TimeInter; }
	bool IsActive() const { return m_bActive; }
	bool IsInterpolating() const { return m_bInter; }

	void SetEnable(bool enable)
	{
		m_bActive = enable;
		m_TimeScale = enable ? m_TimeStopScale : m_TimeResumeScale;
		m_bInter = true;

		// Immediately clear stop flags when initiating resumption
		if (!enable)
		{
			OnInterpolationFinish(false);
		}
	}

	void OnTickEvent()
	{
		ApplyInterpolation();

		// Applying slow rate from time interpolation
		g_RateMan.setRate(eRateType_Em, m_TimeInter);
		g_RateMan.setRate(eRateType_Pl, 1.0f);
		g_RateMan.setRate(eRateType_Scene, m_TimeInter);
		g_RateMan.setRate(eRateType_Etc, m_TimeInter);
	}
};