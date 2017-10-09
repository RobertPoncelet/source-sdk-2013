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

IMPLEMENT_CLIENTCLASS_DT(C_AbhPedestrian, DT_AbhPedestrian, CAbhPedestrian)
	RecvPropBool(RECVINFO(m_bIsDemon)),
	RecvPropFloat(RECVINFO(m_radius)),
	RecvPropFloat(RECVINFO(m_fov)),
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
	// get my particles
	CParticleProperty * pProp = ParticleProp();

	// The dim light is the flashlight.
	if (!m_bIsDemon)
	{
		FlashlightState_t state;
		state.m_bEnableShadows = false;
		state.m_fHorizontalFOVDegrees = m_fov;
		state.m_fVerticalFOVDegrees = state.m_fHorizontalFOVDegrees;
		state.m_FarZ = m_radius;
		state.m_NearZ = 16.0f;
		state.m_Color[0] = 0.8f;
		state.m_Color[1] = 0.1f;
		state.m_Color[2] = 0.2f;

		if (m_pHeadlight == NULL)
		{
			// Turned on the headlight; create it.
			m_pHeadlight = new CHeadlightEffect;

			if (m_pHeadlight == NULL)
			{
				return;
			}

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

			m_pHeadlight->UpdateLight(vVector, vecForward, vecRight, vecUp, m_radius, state);
		}

		if (!m_pHeadX)
		{
			m_pHeadX = pProp->Create("pedestrian_headx", PATTACH_POINT_FOLLOW, LookupAttachment("forward"));
			AssertMsg1(m_pHeadX, "Particle system couldn't make %s", "pedestrian_headx");
			if (m_pHeadX)
			{
				pProp->AddControlPoint(m_pHeadX, 1, this, PATTACH_POINT_FOLLOW, "forward");
			}
		}
	}
	else 
	{
		if (m_pHeadlight)
		{
			// Turned off the flashlight; delete it.
			delete m_pHeadlight;
			m_pHeadlight = NULL;
		}
		// squelch the prior effect if it exists
		if (m_pHeadX)
		{
			pProp->StopEmission(m_pHeadX);
			m_pHeadX = NULL;
		}
	}

	BaseClass::Simulate();
}