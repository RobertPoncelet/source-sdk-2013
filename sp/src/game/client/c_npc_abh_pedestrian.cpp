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

//ConVar r_JeepViewBlendTo("r_JeepViewBlendTo", "1", FCVAR_CHEAT);

#define JEEP_DELTA_LENGTH_MAX	12.0f			// 1 foot
#define JEEP_FRAMETIME_MIN		1e-6
#define JEEP_HEADLIGHT_DISTANCE 1000

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
	if (m_bIsDemon)
	{
		if (m_pHeadlight == NULL)
		{
			// Turned on the headlight; create it.
			m_pHeadlight = new CHeadlightEffect;

			if (m_pHeadlight == NULL)
				return;

			m_pHeadlight->TurnOn();
		}

		QAngle vAngle;
		Vector vVector;
		Vector vecForward, vecRight, vecUp;

		int iAttachment = LookupAttachment("headlight");

		if (iAttachment != INVALID_PARTICLE_ATTACHMENT)
		{
			GetAttachment(iAttachment, vVector, vAngle);
			AngleVectors(vAngle, &vecForward, &vecRight, &vecUp);

			m_pHeadlight->UpdateLight(vVector, vecForward, vecRight, vecUp, JEEP_HEADLIGHT_DISTANCE);
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