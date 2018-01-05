//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#ifndef C_ABH_DEMON_H
#define C_ABH_DEMON_H
#pragma once

#include "cbase.h"
#include "c_ai_basenpc.h"
//#include "flashlighteffect.h"

// memdbgon must be the last include file in a .cpp file!!!
//#include "tier0/memdbgon.h"

//=============================================================================
//
// Client-side demon
//
class C_AbhDemon : public C_AI_BaseNPC
{

	DECLARE_CLASS(C_AbhDemon, C_AI_BaseNPC);

public:

	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();

	C_AbhDemon();
	~C_AbhDemon();

public:

	void Simulate(void);

private:

	CNewParticleEffect *m_pHeadX;

};

#endif // C_ABH_PEDESTRIAN_H
