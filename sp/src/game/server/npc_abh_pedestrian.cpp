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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define INSIGNIA_MODEL "models/chefhat.mdl"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define CIT_INSPECTED_DELAY_TIME 120  //How often I'm allowed to be inspected

extern ConVar sk_healthkit;
extern ConVar sk_healthvial;

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

ConVar	abh_pedestrian_radius("abh_pedestrian_radius", "128");

//---------------------------------------------------------

class CAbhPedestrian : public CNPC_Citizen
{
	DECLARE_CLASS(CAbhPedestrian, CNPC_Citizen);

public:
	void Spawn();
	void Precache();
	void PrescheduleThink();
	void InputBecomeDemon(inputdata_t &inputData);
	void InputStopBeingDemon(inputdata_t &inputData);
	Class_T Classify();

private:
	bool bIsDemon;
	// Don't talk to me or my demon son ever again
	EHANDLE m_demonHandle;

public:
	//DEFINE_CUSTOM_AI;
	DECLARE_DATADESC();
};

//---------------------------------------------------------

LINK_ENTITY_TO_CLASS(npc_abhpedestrian, CAbhPedestrian);

//---------------------------------------------------------

BEGIN_DATADESC(CAbhPedestrian)

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

END_DATADESC()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CAbhPedestrian::Spawn(void) 
{
	bIsDemon = false;
	Precache();
	SetRenderColor(0, 0, 0);
	BaseClass::Spawn();
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

	BaseClass::Precache();
}

void CAbhPedestrian::InputBecomeDemon(inputdata_t &inputData) 
{
	bIsDemon = true;

	SetRenderMode(kRenderNone);
	SetCollisionGroup(COLLISION_GROUP_NONE);
	if (!IsCurSchedule(SCHED_NPC_FREEZE))
	{
		ToggleFreeze();
	}

	// Spawn the demon
	CAI_BaseNPC	*demonEnt;

	demonEnt = (CAI_BaseNPC*)CreateEntityByName("npc_abhdemon");

	if (!demonEnt)
	{
		Warning("**%s: Can't make %s!\n", GetClassname(), "npc_abhdemon");
		return;
	}

	// Stick the crab in whatever squad the zombie was in.
	//demonEnt->SetSquadName(m_SquadName);

	// don't pop to floor, fall
	demonEnt->AddSpawnFlags(SF_NPC_FALL_TO_GROUND);

	// make me the crab's owner to avoid collision issues
	demonEnt->SetOwnerEntity(this);

	demonEnt->SetAbsOrigin(GetAbsOrigin());
	demonEnt->SetAbsAngles(GetAbsAngles());
	DispatchSpawn(demonEnt);

	demonEnt->GetMotor()->SetIdealYaw(GetAbsAngles().y);

	demonEnt->SetActivity(ACT_IDLE);
	demonEnt->SetNextThink(gpGlobals->curtime);
	demonEnt->PhysicsSimulate();
	//demonEnt->SetAbsVelocity(vecVelocity);

	// if I have an enemy, stuff that to the headcrab.
	CBaseEntity *pEnemy;
	pEnemy = GetEnemy();

	demonEnt->m_flNextAttack = gpGlobals->curtime + 1.0f;

	demonEnt->Activate();
	
	m_demonHandle.Set(demonEnt);
}

void CAbhPedestrian::InputStopBeingDemon(inputdata_t &inputData) 
{
	bIsDemon = false;

	SetRenderMode(kRenderNormal);
	SetCollisionGroup(COLLISION_GROUP_NPC);
	if (IsCurSchedule(SCHED_NPC_FREEZE))
	{
		ToggleFreeze();
	}

	// stfu
	CAI_BaseNPC* demon = (CAI_BaseNPC*)m_demonHandle.Get();
	CTakeDamageInfo info;
	demon->Event_Killed(info);

	//UTIL_Remove(m_demonHandle);
}

Class_T	CAbhPedestrian::Classify()
{
	return CLASS_CITIZEN_PASSIVE;
}

void CAbhPedestrian::PrescheduleThink()
{
	BaseClass::PrescheduleThink();

	if (bIsDemon || !UTIL_FindClientInPVS(edict()))
	{
		return;
	}
 
	float flThreshold = abh_pedestrian_radius.GetFloat();
	flThreshold *= flThreshold;

	// check the player.
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	if (pPlayer && !(pPlayer->GetFlags() & FL_NOTARGET))
	{
		float flDist = (pPlayer->GetAbsOrigin() - GetAbsOrigin()).LengthSqr();

		if (flDist < flThreshold && FVisible(pPlayer, MASK_SOLID_BRUSHONLY))
		{
			inputdata_t data;
			InputBecomeDemon(data);
		}
	}
}