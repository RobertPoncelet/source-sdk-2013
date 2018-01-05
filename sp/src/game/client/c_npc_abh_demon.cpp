//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_npc_abh_demon.h"
//#include "flashlighteffect.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT(C_AbhDemon, DT_AbhDemon, CAbhDemon)
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
C_AbhDemon::C_AbhDemon()
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
C_AbhDemon::~C_AbhDemon()
{
	// squelch the prior effect if it exists
	if (m_pHeadX)
	{
		// get my particles
		CParticleProperty * pProp = ParticleProp();
		pProp->StopEmission(m_pHeadX);
		m_pHeadX = NULL;
	}
}

void C_AbhDemon::Simulate(void)
{
	// get my particles
	CParticleProperty * pProp = ParticleProp();

	int iHeadAttachment = LookupAttachment("forward");

	if (!m_pHeadX)
	{
		m_pHeadX = pProp->Create("pedestrian_headx", PATTACH_CUSTOMORIGIN, iHeadAttachment);
		AssertMsg1(m_pHeadX, "Particle system couldn't make %s", "pedestrian_headx");
		/*if (m_pHeadX)
		{
		pProp->AddControlPoint(m_pHeadX, 1, this, PATTACH_POINT_FOLLOW, "forward");
		}*/
	}

	if (m_pHeadX)
	{
		Vector headPos;
		bool validPos = GetAttachment(iHeadAttachment, headPos);
		if (validPos) {
			C_BasePlayer* player = UTIL_PlayerByIndex(GetLocalPlayerIndex());
			Vector playerEye;
			QAngle tmpAng;
			float tmpFloat;
			player->CalcView(playerEye, tmpAng, tmpFloat, tmpFloat, tmpFloat);
			Vector dir = (playerEye - headPos).Normalized();
			m_pHeadX->SetControlPoint(0, headPos + (dir * 16.0f));
		}
		else
		{
			AssertMsg1(m_pHeadX, "Invalid attachment point for %s", "pedestrian_headx");
		}
	}

	BaseClass::Simulate();
}