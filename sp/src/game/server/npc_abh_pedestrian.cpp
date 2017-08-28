//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The downtrodden citizens of City 17.
//
//=============================================================================//

#include "cbase.h"

#include "npc_citizen17.h"
#include "npc_BaseZombie.h"

#include "ammodef.h"
#include "globalstate.h"
#include "soundent.h"
#include "BasePropDoor.h"
#include "weapon_rpg.h"
#include "hl2_player.h"
#include "items.h"
#include "ai_spotlight.h"


#ifdef HL2MP
#include "hl2mp/weapon_crowbar.h"
#else
#include "weapon_crowbar.h"
#endif

#include "eventqueue.h"

#include "ai_squad.h"
#include "ai_pathfinder.h"
#include "ai_route.h"
#include "ai_hint.h"
#include "ai_interactions.h"
#include "ai_looktarget.h"
#include "sceneentity.h"
#include "tier0/icommandline.h"
#include "npc_fastzombie.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define INSIGNIA_MODEL "models/chefhat.mdl"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define CIT_INSPECTED_DELAY_TIME 120  //How often I'm allowed to be inspected

extern ConVar sk_healthkit;
extern ConVar sk_healthvial;

ConVar	abh_pedestrian_radius("abh_pedestrian_radius", "128");
ConVar	abh_pedestrian_fov("abh_pedestrian_fov", "120");
ConVar	abh_pedestrian_demon_time("abh_pedestrian_demon_time", "10");

const int MAX_PLAYER_SQUAD = 4;


enum SquadSlot_T
{
	SQUAD_SLOT_CITIZEN_RPG1 = LAST_SHARED_SQUADSLOT,
	SQUAD_SLOT_CITIZEN_RPG2,
};

const float HEAL_MOVE_RANGE = 30 * 12;
const float HEAL_TARGET_RANGE = 120; // 10 feet
#ifdef HL2_EPISODIC
const float HEAL_TOSS_TARGET_RANGE = 480; // 40 feet when we are throwing medkits 
const float HEAL_TARGET_RANGE_Z = 72; // a second check that Gordon isn't too far above us -- 6 feet
#endif

// player must be at least this distance away from an enemy before we fire an RPG at him
const float RPG_SAFE_DISTANCE = CMissile::EXPLOSION_RADIUS + 64.0;

//-------------------------------------
//-------------------------------------

#define DebuggingCommanderMode() (ai_citizen_debug_commander.GetBool() && (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))

//-----------------------------------------------------------------------------
// Citizen expressions for the citizen expression types
//-----------------------------------------------------------------------------
#define STATES_WITH_EXPRESSIONS		3		// Idle, Alert, Combat
#define EXPRESSIONS_PER_STATE		1

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//---------------------------------------------------------
// Citizen models
//---------------------------------------------------------

static const char *g_ppszRandomHeads[] =
{
	"male_01.mdl",
	"male_02.mdl",
	"female_01.mdl",
	"male_03.mdl",
	"female_02.mdl",
	"male_04.mdl",
	"female_03.mdl",
	"male_05.mdl",
	"female_04.mdl",
	"male_06.mdl",
	"female_06.mdl",
	"male_07.mdl",
	"female_07.mdl",
	"male_08.mdl",
	"male_09.mdl",
};

static const char *g_ppszModelLocs[] =
{
	"Group01",
	"Group01",
	"Group02",
	"Group03%s",
};

#define IsExcludedHead( type, bMedic, iHead) false // see XBox codeline for an implementation

static const char *s_pSpotlightThinkContext = "SpotlightThink";

//---------------------------------------------------------

class CAbhPedestrian : public CNPC_Citizen
{
	DECLARE_CLASS(CAbhPedestrian, CNPC_Citizen);
	DECLARE_SERVERCLASS();

public:
	void Spawn();
	void Precache();
	void PrescheduleThink();
	void InputBecomeDemon(inputdata_t &inputData);
	void InputStopBeingDemon(inputdata_t &inputData);
	//void SpotlightCreate();
	//void SpotlightDestroy();
	//void SpotlightUpdate();
	bool CreateComponents();
	void BuildScheduleTestBits();
	void Activate();
	Class_T Classify();

private:
	CNetworkVar(bool, m_bIsDemon);
	CNetworkVar(float, m_radius);
	CNetworkVar(float, m_fov);
	float m_timeBecameDemon;
	// Don't talk to me or my demon son ever again
	EHANDLE m_demonHandle;
	//CHandle<CBeam>	m_hSpotlight;
	CAI_Spotlight	m_Spotlight;
	int m_nSpotlightAttachment;
	CBaseEntity* m_pathCorner;
	//int m_nHaloSprite;

	bool CanSeePlayer();

	// Spotlights
	void SpotlightThink();
	void SpotlightStartup();
	void SpotlightShutdown();

public:
	//DEFINE_CUSTOM_AI;
	DECLARE_DATADESC();
};

//---------------------------------------------------------

LINK_ENTITY_TO_CLASS(npc_abhpedestrian, CAbhPedestrian);

//---------------------------------------------------------

BEGIN_DATADESC(CAbhPedestrian)

DEFINE_EMBEDDED(m_Spotlight),
DEFINE_CUSTOM_FIELD(m_nInspectActivity, ActivityDataOps()),
DEFINE_FIELD(m_flNextFearSoundTime, FIELD_TIME),
DEFINE_FIELD(m_flStopManhackFlinch, FIELD_TIME),
DEFINE_FIELD(m_fNextInspectTime, FIELD_TIME),
DEFINE_FIELD(m_flPlayerHealTime, FIELD_TIME),
DEFINE_FIELD(m_flNextHealthSearchTime, FIELD_TIME),
DEFINE_FIELD(m_flAllyHealTime, FIELD_TIME),
//						gm_PlayerSquadEvaluateTimer
//						m_AssaultBehavior
//						m_FollowBehavior
//						m_StandoffBehavior
//						m_LeadBehavior
//						m_FuncTankBehavior
DEFINE_FIELD(m_flPlayerGiveAmmoTime, FIELD_TIME),
DEFINE_KEYFIELD(m_iszAmmoSupply, FIELD_STRING, "ammosupply"),
DEFINE_KEYFIELD(m_iAmmoAmount, FIELD_INTEGER, "ammoamount"),
DEFINE_FIELD(m_bRPGAvoidPlayer, FIELD_BOOLEAN),
DEFINE_FIELD(m_bShouldPatrol, FIELD_BOOLEAN),
DEFINE_FIELD(m_iszOriginalSquad, FIELD_STRING),
DEFINE_FIELD(m_flTimeJoinedPlayerSquad, FIELD_TIME),
DEFINE_FIELD(m_bWasInPlayerSquad, FIELD_BOOLEAN),
DEFINE_FIELD(m_flTimeLastCloseToPlayer, FIELD_TIME),
DEFINE_EMBEDDED(m_AutoSummonTimer),
DEFINE_FIELD(m_vAutoSummonAnchor, FIELD_POSITION_VECTOR),
DEFINE_KEYFIELD(m_Type, FIELD_INTEGER, "citizentype"),
DEFINE_KEYFIELD(m_ExpressionType, FIELD_INTEGER, "expressiontype"),
DEFINE_FIELD(m_iHead, FIELD_INTEGER),
DEFINE_FIELD(m_flTimePlayerStare, FIELD_TIME),
DEFINE_FIELD(m_flTimeNextHealStare, FIELD_TIME),
DEFINE_FIELD(m_hSavedFollowGoalEnt, FIELD_EHANDLE),
DEFINE_KEYFIELD(m_bNotifyNavFailBlocked, FIELD_BOOLEAN, "notifynavfailblocked"),
DEFINE_KEYFIELD(m_bNeverLeavePlayerSquad, FIELD_BOOLEAN, "neverleaveplayersquad"),
DEFINE_KEYFIELD(m_iszDenyCommandConcept, FIELD_STRING, "denycommandconcept"),

DEFINE_OUTPUT(m_OnJoinedPlayerSquad, "OnJoinedPlayerSquad"),
DEFINE_OUTPUT(m_OnLeftPlayerSquad, "OnLeftPlayerSquad"),
DEFINE_OUTPUT(m_OnFollowOrder, "OnFollowOrder"),
DEFINE_OUTPUT(m_OnStationOrder, "OnStationOrder"),
DEFINE_OUTPUT(m_OnPlayerUse, "OnPlayerUse"),
DEFINE_OUTPUT(m_OnNavFailBlocked, "OnNavFailBlocked"),

DEFINE_INPUTFUNC(FIELD_VOID, "RemoveFromPlayerSquad", InputRemoveFromPlayerSquad),
DEFINE_INPUTFUNC(FIELD_VOID, "StartPatrolling", InputStartPatrolling),
DEFINE_INPUTFUNC(FIELD_VOID, "StopPatrolling", InputStopPatrolling),
DEFINE_INPUTFUNC(FIELD_VOID, "SetCommandable", InputSetCommandable),
DEFINE_INPUTFUNC(FIELD_VOID, "SetMedicOn", InputSetMedicOn),
DEFINE_INPUTFUNC(FIELD_VOID, "SetMedicOff", InputSetMedicOff),
DEFINE_INPUTFUNC(FIELD_VOID, "SetAmmoResupplierOn", InputSetAmmoResupplierOn),
DEFINE_INPUTFUNC(FIELD_VOID, "SetAmmoResupplierOff", InputSetAmmoResupplierOff),
DEFINE_INPUTFUNC(FIELD_VOID, "SpeakIdleResponse", InputSpeakIdleResponse),

// New stuff
DEFINE_INPUTFUNC(FIELD_VOID, "BecomeDemon", InputBecomeDemon),
DEFINE_INPUTFUNC(FIELD_VOID, "StopBeingDemon", InputStopBeingDemon),

#if HL2_EPISODIC
DEFINE_INPUTFUNC(FIELD_VOID, "ThrowHealthKit", InputForceHealthKitToss),
#endif

DEFINE_USEFUNC(CommanderUse),
DEFINE_USEFUNC(SimpleUse),

DEFINE_THINKFUNC(SpotlightThink),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CAbhPedestrian, DT_AbhPedestrian)
	SendPropBool(SENDINFO(m_bIsDemon)),
	SendPropFloat(SENDINFO(m_radius)),
	SendPropFloat(SENDINFO(m_fov)),
END_SEND_TABLE();

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CAbhPedestrian::Spawn(void) 
{
	m_bIsDemon = false;
	m_timeBecameDemon = 0.0f;
	Precache();
	SetRenderColor(0, 0, 0);
	AddClassRelationship(CLASS_ZOMBIE, D_NU, 100);
	RegisterThinkContext(s_pSpotlightThinkContext);
	SetContextThink(&CAbhPedestrian::SpotlightThink, gpGlobals->curtime, s_pSpotlightThinkContext);
	//SpotlightCreate();
	SpotlightStartup();
	BaseClass::Spawn();
}

bool CAbhPedestrian::CreateComponents()
{
	if (!BaseClass::CreateComponents())
		return false;

	m_Spotlight.Init(this, AI_SPOTLIGHT_NO_DLIGHTS, 45.0f, 64.0f);
	return true;
}

void CAbhPedestrian::Activate(void)
{
	BaseClass::Activate();
	m_nSpotlightAttachment = LookupAttachment("eyes");
}

void CAbhPedestrian::Precache(void) 
{
	// Demon stuff
	PrecacheModel("models/zombie/fast.mdl");
	PrecacheModel("models/headcrab.mdl");
#ifdef HL2_EPISODIC
	PrecacheModel("models/zombie/Fast_torso.mdl");
	PrecacheScriptSound("NPC_FastZombie.CarEnter1");
	PrecacheScriptSound("NPC_FastZombie.CarEnter2");
	PrecacheScriptSound("NPC_FastZombie.CarEnter3");
	PrecacheScriptSound("NPC_FastZombie.CarEnter4");
	PrecacheScriptSound("NPC_FastZombie.CarScream");
#endif
	PrecacheModel("models/gibs/fast_zombie_torso.mdl");
	PrecacheModel("models/gibs/fast_zombie_legs.mdl");

	PrecacheScriptSound("NPC_FastZombie.LeapAttack");
	PrecacheScriptSound("NPC_FastZombie.FootstepRight");
	PrecacheScriptSound("NPC_FastZombie.FootstepLeft");
	PrecacheScriptSound("NPC_FastZombie.AttackHit");
	PrecacheScriptSound("NPC_FastZombie.AttackMiss");
	PrecacheScriptSound("NPC_FastZombie.LeapAttack");
	PrecacheScriptSound("NPC_FastZombie.Attack");
	PrecacheScriptSound("NPC_FastZombie.Idle");
	PrecacheScriptSound("NPC_FastZombie.AlertFar");
	PrecacheScriptSound("NPC_FastZombie.AlertNear");
	PrecacheScriptSound("NPC_FastZombie.GallopLeft");
	PrecacheScriptSound("NPC_FastZombie.GallopRight");
	PrecacheScriptSound("NPC_FastZombie.Scream");
	PrecacheScriptSound("NPC_FastZombie.RangeAttack");
	PrecacheScriptSound("NPC_FastZombie.Frenzy");
	PrecacheScriptSound("NPC_FastZombie.NoSound");
	PrecacheScriptSound("NPC_FastZombie.Die");

	PrecacheScriptSound("NPC_FastZombie.Gurgle");

	PrecacheScriptSound("NPC_FastZombie.Moan1");

	PrecacheScriptSound("E3_Phystown.Slicer");
	PrecacheScriptSound("NPC_BaseZombie.PoundDoor");
	PrecacheScriptSound("NPC_BaseZombie.Swat");

	PrecacheParticleSystem("blood_impact_zombie_01");

	// Sprites
	//m_nHaloSprite = PrecacheModel("sprites/light_glow03.vmt");
	PrecacheModel("sprites/glow_test02.vmt");

	BaseClass::Precache();
}

void CAbhPedestrian::InputBecomeDemon(inputdata_t &inputData) 
{
	m_bIsDemon = true;
	m_timeBecameDemon = gpGlobals->curtime;

	InputDisableShadow(inputdata_t());
	SetRenderMode(kRenderNone);
	SetCollisionGroup(COLLISION_GROUP_DEBRIS);

	m_pathCorner = GetGoalEnt();
	if (!IsCurSchedule(SCHED_NPC_FREEZE))
	{
		//ToggleFreeze();
		//SetCondition(COND_NPC_FREEZE);
		//SetMoveType(MOVETYPE_NONE);
		//SetGravity(0);
		//SetLocalAngularVelocity(vec3_angle);
		//SetAbsVelocity(vec3_origin);
	}

	SetNextThink(gpGlobals->curtime + abh_pedestrian_demon_time.GetFloat());

	//SpotlightDestroy();
	SpotlightShutdown();

	// Spawn the demon
	CFastZombie	*demonEnt;

	demonEnt = (CFastZombie*)CreateEntityByName("npc_abhdemon");

	if (!demonEnt)
	{
		Warning("**%s: Can't make %s!\n", GetClassname(), "npc_abhdemon");
		return;
	}

	// make me the crab's owner to avoid collision issues
	demonEnt->SetOwnerEntity(this);

	demonEnt->SetAbsOrigin(GetAbsOrigin());

	// make me face the player
	CBaseEntity *pEnemy = UTIL_GetLocalPlayer();
	Vector dir = pEnemy->GetAbsOrigin() - demonEnt->GetAbsOrigin();
	QAngle angFacing;
	VectorAngles(dir, angFacing);
	// only use the yaw
	//angFacing.x = angFacing.y = 0;
	demonEnt->SetAbsAngles(angFacing);
	DispatchSpawn(demonEnt);

	demonEnt->GetMotor()->SetIdealYaw(angFacing.y);

	demonEnt->ScheduledMoveToGoalEntity(SCHED_FASTZOMBIE_RANGE_ATTACK1, pEnemy, (Activity)ACT_FASTZOMBIE_LEAP_STRIKE);
	demonEnt->SetSchedule(SCHED_RANGE_ATTACK1);
	demonEnt->SetNextThink(gpGlobals->curtime);
	demonEnt->PhysicsSimulate();

	demonEnt->SetEnemy(pEnemy);

	demonEnt->Activate();
	
	m_demonHandle.Set(demonEnt);
}

void CAbhPedestrian::InputStopBeingDemon(inputdata_t &inputData) 
{
	m_bIsDemon = false;

	InputEnableShadow(inputdata_t());
	SetRenderMode(kRenderNormal);
	//SetCollisionGroup(COLLISION_GROUP_NPC);
	//SpotlightCreate();
	SpotlightStartup();

	if (IsCurSchedule(SCHED_NPC_FREEZE))
	{
		//ToggleFreeze();
		// Unfreeze them.
		//SetCondition(COND_NPC_UNFREEZE);

		// BUGBUG: this might not be the correct movetype!
		//SetMoveType(MOVETYPE_STEP);

		// Doesn't restore gravity to the original value, but who cares?
		//SetGravity(1);
	}

	if (m_pathCorner)
	{
		//ScheduledMoveToGoalEntity(SCHED_IDLE_WALK, m_pathCorner, ACT_WALK);
		SetSchedule(SCHED_IDLE_WALK);
	}
	else
	{
		Msg("No path_corner for this pedestrian!\n");
	}

	// stfu
	CFastZombie* demon = dynamic_cast<CFastZombie*>(m_demonHandle.Get());
	if (demon)
	{
		demon->StopLoopingSounds();
	}

	UTIL_Remove(m_demonHandle);
}

Class_T	CAbhPedestrian::Classify()
{
	return CLASS_CITIZEN_PASSIVE;
}

void CAbhPedestrian::PrescheduleThink()
{
	BaseClass::PrescheduleThink();

	if (m_lifeState != LIFE_ALIVE)
	{
		UTIL_Remove(m_demonHandle);
		SpotlightShutdown();
		return;
	}

	if (m_bIsDemon)
	{
		if (gpGlobals->curtime > m_timeBecameDemon + abh_pedestrian_demon_time.GetFloat())
		{
			InputStopBeingDemon(inputdata_t());
		}
		return;
	}

	if (!UTIL_FindClientInPVS(edict()))
	{
		return;
	}

	if (CanSeePlayer())
	{
		InputBecomeDemon(inputdata_t());
	}
}

void CAbhPedestrian::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();

	ClearCustomInterruptCondition(COND_SEE_HATE);
	ClearCustomInterruptCondition(COND_SEE_FEAR);
	ClearCustomInterruptCondition(COND_SEE_ENEMY);
	ClearCustomInterruptCondition(COND_HEAR_DANGER);
	ClearCustomInterruptCondition(COND_HEAR_COMBAT);
	ClearCustomInterruptCondition(COND_HEAR_PLAYER);
	ClearCustomInterruptCondition(COND_HEAR_BULLET_IMPACT);
	ClearCustomInterruptCondition(COND_HEAR_PHYSICS_DANGER);
	ClearCustomInterruptCondition(COND_HEAR_SPOOKY);
	ClearCustomInterruptCondition(COND_LIGHT_DAMAGE);
	ClearCustomInterruptCondition(COND_HEAVY_DAMAGE);
	ClearCustomInterruptCondition(COND_NEW_ENEMY);
	ClearCustomInterruptCondition(79); // COND_CIT_HURTBYFIRE
}

//------------------------------------------------------------------------------
// Start up spotlights
//------------------------------------------------------------------------------
void CAbhPedestrian::SpotlightStartup()
{
	Vector vecForward;
	Vector vecOrigin;
	GetAttachment(m_nSpotlightAttachment, vecOrigin, &vecForward);
	m_Spotlight.SpotlightCreate(m_nSpotlightAttachment, vecForward);
	SpotlightThink();
}


//------------------------------------------------------------------------------
// Shutdown spotlights
//------------------------------------------------------------------------------
void CAbhPedestrian::SpotlightShutdown()
{
	m_Spotlight.SpotlightDestroy();
	SetContextThink(NULL, gpGlobals->curtime, s_pSpotlightThinkContext);
}


//------------------------------------------------------------------------------
// Spotlights
//------------------------------------------------------------------------------
void CAbhPedestrian::SpotlightThink()
{
	// NOTE: This function should deal with all deactivation cases
	if (m_lifeState != LIFE_ALIVE)
	{
		SpotlightShutdown();
		return;
	}

	if (!m_bIsDemon)
	{
		Vector vecForward;
		Vector vecOrigin;
		GetAttachment(m_nSpotlightAttachment, vecOrigin, &vecForward);
		Vector velocity = GetAbsVelocity();
		float thisTime = gpGlobals->curtime - GetLastThink();
		m_Spotlight.SetSpotlightTargetDirection(vecForward);
		m_Spotlight.m_hSpotlight->PointsInit(vecOrigin + velocity * thisTime, vecOrigin + vecForward * 192.0f);
	}
	else
	{
		SpotlightShutdown();
		return;
	}

	m_Spotlight.Update();
	SetContextThink(&CAbhPedestrian::SpotlightThink, gpGlobals->curtime + TICK_INTERVAL, s_pSpotlightThinkContext);
}

bool CAbhPedestrian::CanSeePlayer()
{
	m_radius = abh_pedestrian_radius.GetFloat();
	m_fov = abh_pedestrian_fov.GetFloat();

	float flThreshold = m_radius;
	flThreshold *= flThreshold;

	// check the player.
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	if (pPlayer && !(pPlayer->GetFlags() & FL_NOTARGET))
	{
		Vector between = pPlayer->GetAbsOrigin() - GetAbsOrigin();
		float flDist = between.LengthSqr();

		if (flDist > flThreshold)
		{
			return false;
		}

		float angle = m_fov; // / 2.0;
		angle *= M_PI / 360.0f;
		float cosAngle = cos(angle);

		int eyes = LookupAttachment("eyes");
		Vector eyePos, eyeDir;
		GetAttachment(eyes, eyePos, &eyeDir);

		if (eyeDir.Dot(between.Normalized()) > cosAngle && FVisible(pPlayer, MASK_SOLID_BRUSHONLY))
		{
			return true;
		}
	}

	return false;
}