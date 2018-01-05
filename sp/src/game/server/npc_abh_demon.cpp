//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_motor.h"
#include "ai_memory.h"
#include "ai_route.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "entitylist.h"
#include "ai_task.h"
#include "activitylist.h"
#include "engine/IEngineSound.h"
#include "npc_BaseZombie.h"
#include "movevars_shared.h"
#include "IEffects.h"
#include "props.h"
#include "physics_npc_solver.h"
#include "physics_prop_ragdoll.h"

#ifdef HL2_EPISODIC
#include "episodic/ai_behavior_passenger_zombie.h"
#endif	// HL2_EPISODIC

#include "npc_fastzombie.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	abh_demon_shutup("abh_demon_shutup", "0");

//=========================================================
//=========================================================
class CAbhDemon : public CFastZombie
{
	DECLARE_CLASS(CAbhDemon, CFastZombie);
	DECLARE_SERVERCLASS();

public:
	void Spawn(void);
	void SetZombieModel(void);
	void Precache(void);

	virtual int TranslateSchedule(int scheduleType);
	virtual void Activate();
	virtual void ReleaseHeadcrab(const Vector &vecOrigin, const Vector &vecVelocity, bool fRemoveHead, bool fRagdollBody, bool fRagdollCrab = false);
	virtual CBaseEntity* ClawAttack(float flDist, int iDamage, QAngle &qaViewPunch, Vector &vecVelocityPunch, int BloodOrigin);

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(npc_abhdemon, CAbhDemon);
LINK_ENTITY_TO_CLASS(npc_abhdemon_torso, CAbhDemon);


BEGIN_DATADESC(CAbhDemon)

DEFINE_FIELD(m_flDistFactor, FIELD_FLOAT),
DEFINE_FIELD(m_iClimbCount, FIELD_CHARACTER),
DEFINE_FIELD(m_fIsNavJumping, FIELD_BOOLEAN),
DEFINE_FIELD(m_fIsAttackJumping, FIELD_BOOLEAN),
DEFINE_FIELD(m_fHitApex, FIELD_BOOLEAN),
DEFINE_FIELD(m_flJumpDist, FIELD_FLOAT),
DEFINE_FIELD(m_fHasScreamed, FIELD_BOOLEAN),
DEFINE_FIELD(m_flNextMeleeAttack, FIELD_TIME),
DEFINE_FIELD(m_fJustJumped, FIELD_BOOLEAN),
DEFINE_FIELD(m_flJumpStartAltitude, FIELD_FLOAT),
DEFINE_FIELD(m_flTimeUpdateSound, FIELD_TIME),

// Function Pointers
DEFINE_ENTITYFUNC(LeapAttackTouch),
DEFINE_ENTITYFUNC(ClimbTouch),
DEFINE_SOUNDPATCH(m_pLayer2),

#ifdef HL2_EPISODIC
DEFINE_ENTITYFUNC(VehicleLeapAttackTouch),
DEFINE_INPUTFUNC(FIELD_STRING, "AttachToVehicle", InputAttachToVehicle),
#endif	// HL2_EPISODIC

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CAbhDemon, DT_AbhDemon)
END_SEND_TABLE();

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CAbhDemon::Spawn(void)
{
	BaseClass::Spawn();

	// Don't attack mommy
	AddClassRelationship(CLASS_CITIZEN_PASSIVE, D_NU, 100);
}

void CAbhDemon::Precache(void)
{
	PrecacheParticleSystem("pedestrian_headx");
	BaseClass::Precache();
}

void CAbhDemon::Activate()
{
	BaseClass::Activate();
	if (abh_demon_shutup.GetBool())
	{
		StopLoopingSounds();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CAbhDemon::SetZombieModel(void)
{
	BaseClass::SetZombieModel();
	SetBodygroup(ZOMBIE_BODYGROUP_HEADCRAB, 0); // no headcrabs allowed
}

//-----------------------------------------------------------------------------
// Purpose: No headcrab, so do nothing
//-----------------------------------------------------------------------------
void CAbhDemon::ReleaseHeadcrab(const Vector &vecOrigin, const Vector &vecVelocity, bool fRemoveHead, bool fRagdollBody, bool fRagdollCrab)
{
}

CBaseEntity* CAbhDemon::ClawAttack(float flDist, int iDamage, QAngle &qaViewPunch, Vector &vecVelocityPunch, int BloodOrigin)
{
	// Always do a shitload of damage
	return BaseClass::ClawAttack(flDist, iDamage * 10, qaViewPunch, vecVelocityPunch, BloodOrigin);
}

int CAbhDemon::TranslateSchedule(int scheduleType)
{
	if (scheduleType == SCHED_RANGE_ATTACK1 && abh_demon_shutup.GetBool())
	{
		m_fHasScreamed = true;
	}
	return BaseClass::TranslateSchedule(scheduleType);
}

