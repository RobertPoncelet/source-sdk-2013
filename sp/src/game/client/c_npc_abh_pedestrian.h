//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#ifndef C_ABH_PEDESTRIAN_H
#define C_ABH_PEDESTRIAN_H
#pragma once

#include "cbase.h"
#include "c_ai_basenpc.h"
#include "flashlighteffect.h"

// memdbgon must be the last include file in a .cpp file!!!
//#include "tier0/memdbgon.h"

//=============================================================================
//
// Client-side pedestrian
//
class C_AbhPedestrian : public C_AI_BaseNPC
{

	DECLARE_CLASS(C_AbhPedestrian, C_AI_BaseNPC);

public:

	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();

	C_AbhPedestrian();
	~C_AbhPedestrian();

public:

	void Simulate(void);
	void SpotlightCreate();
	void SpotlightDestroy();
	void SpotlightUpdate();

private:

	CHeadlightEffect *m_pHeadlight;
	bool m_bIsDemon;
	float m_radius;
	float m_fov;
};

#endif // C_ABH_PEDESTRIAN_H
