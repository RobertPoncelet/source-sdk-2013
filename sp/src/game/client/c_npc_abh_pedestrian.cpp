//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_npc_abh_pedestrian.h"
#include "flashlighteffect.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//extern ConVar abh_pedestrian_radius;
//extern ConVar abh_pedestrian_fov;

IMPLEMENT_CLIENTCLASS_DT(C_AbhPedestrian, DT_AbhPedestrian, CAbhPedestrian)
	RecvPropBool(RECVINFO(m_bIsDemon)),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
C_AbhPedestrian::C_AbhPedestrian()
{
	m_pHeadlight = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
C_AbhPedestrian::~C_AbhPedestrian()
{
	if (m_pHeadlight)
	{
		delete m_pHeadlight;
	}
}

void C_AbhPedestrian::Simulate(void)
{
	// The dim light is the flashlight.
	if (!m_bIsDemon)
	{
		if (m_pHeadlight == NULL)
		{
			// Turned on the headlight; create it.
			m_pHeadlight = new CHeadlightEffect;

			if (m_pHeadlight == NULL)
			{
				return;
			}

			FlashlightState_t state;
			state.m_bEnableShadows = false;
			state.m_fHorizontalFOVDegrees = 120.0;// abh_pedestrian_fov.GetFloat();
			state.m_fVerticalFOVDegrees = state.m_fHorizontalFOVDegrees;
			state.m_FarZ = 128;// abh_pedestrian_radius.GetFloat();

			if (m_pHeadlight->GetFlashlightHandle() == CLIENTSHADOW_INVALID_HANDLE)
			{
				m_pHeadlight->SetFlashlightHandle(g_pClientShadowMgr->CreateFlashlight(state));
			}
			else
			{
				g_pClientShadowMgr->UpdateFlashlightState(m_pHeadlight->GetFlashlightHandle(), state);
			}
			g_pClientShadowMgr->UpdateProjectedTexture(m_pHeadlight->GetFlashlightHandle(), true);

			m_pHeadlight->TurnOn();
		}

		QAngle vAngle;
		Vector vVector;
		Vector vecForward, vecRight, vecUp;

		int iAttachment = LookupAttachment("eyes");

		if (iAttachment != INVALID_PARTICLE_ATTACHMENT)
		{
			GetAttachment(iAttachment, vVector, vAngle);
			AngleVectors(vAngle, &vecForward, &vecRight, &vecUp);

			m_pHeadlight->UpdateLight(vVector, vecForward, vecRight, vecUp, 128.0);// abh_pedestrian_radius.GetFloat());
		}
	}
	else if (m_pHeadlight)
	{
		// Turned off the flashlight; delete it.
		delete m_pHeadlight;
		m_pHeadlight = NULL;
	}

	BaseClass::Simulate();
}