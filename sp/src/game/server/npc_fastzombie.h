//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef NPC_FASTZOMBIE_H
#define NPC_FASTZOMBIE_H
#ifdef _WIN32
#pragma once
#endif

#define FASTZOMBIE_IDLE_PITCH			35
#define FASTZOMBIE_MIN_PITCH			70
#define FASTZOMBIE_MAX_PITCH			130
#define FASTZOMBIE_SOUND_UPDATE_FREQ	0.5

#define FASTZOMBIE_MAXLEAP_Z		128

#define FASTZOMBIE_EXCITE_DIST 480.0

#define FASTZOMBIE_BASE_FREQ 1.5

// If flying at an enemy, and this close or closer, start playing the maul animation!!
#define FASTZOMBIE_MAUL_RANGE	300

#ifdef HL2_EPISODIC

int AE_PASSENGER_PHYSICS_PUSH;
int AE_FASTZOMBIE_VEHICLE_LEAP;
int AE_FASTZOMBIE_VEHICLE_SS_DIE;	// Killed while doing scripted sequence on vehicle

#endif // HL2_EPISODIC

enum
{
	COND_FASTZOMBIE_CLIMB_TOUCH = LAST_BASE_ZOMBIE_CONDITION,
};

envelopePoint_t envFastZombieVolumeJump[] =
{
	{ 1.0f, 1.0f,
	0.1f, 0.1f,
	},
	{ 0.0f, 0.0f,
	1.0f, 1.2f,
	},
};

envelopePoint_t envFastZombieVolumePain[] =
{
	{ 1.0f, 1.0f,
	0.1f, 0.1f,
	},
	{ 0.0f, 0.0f,
	1.0f, 1.0f,
	},
};

envelopePoint_t envFastZombieInverseVolumePain[] =
{
	{ 0.0f, 0.0f,
	0.1f, 0.1f,
	},
	{ 1.0f, 1.0f,
	1.0f, 1.0f,
	},
};

envelopePoint_t envFastZombieVolumeJumpPostApex[] =
{
	{ 1.0f, 1.0f,
	0.1f, 0.1f,
	},
	{ 0.0f, 0.0f,
	1.0f, 1.2f,
	},
};

envelopePoint_t envFastZombieVolumeClimb[] =
{
	{ 1.0f, 1.0f,
	0.1f, 0.1f,
	},
	{ 0.0f, 0.0f,
	0.2f, 0.2f,
	},
};

envelopePoint_t envFastZombieMoanVolumeFast[] =
{
	{ 1.0f, 1.0f,
	0.1f, 0.1f,
	},
	{ 0.0f, 0.0f,
	0.2f, 0.3f,
	},
};

envelopePoint_t envFastZombieMoanVolume[] =
{
	{ 1.0f, 1.0f,
	0.1f, 0.1f,
	},
	{ 1.0f, 1.0f,
	0.2f, 0.2f,
	},
	{ 0.0f, 0.0f,
	1.0f, 0.4f,
	},
};

envelopePoint_t envFastZombieFootstepVolume[] =
{
	{ 1.0f, 1.0f,
	0.1f, 0.1f,
	},
	{ 0.7f, 0.7f,
	0.2f, 0.2f,
	},
};

envelopePoint_t envFastZombieVolumeFrenzy[] =
{
	{ 1.0f, 1.0f,
	0.1f, 0.1f,
	},
	{ 0.0f, 0.0f,
	2.0f, 2.0f,
	},
};


//=========================================================
// animation events
//=========================================================
int AE_FASTZOMBIE_LEAP;
int AE_FASTZOMBIE_GALLOP_LEFT;
int AE_FASTZOMBIE_GALLOP_RIGHT;
int AE_FASTZOMBIE_CLIMB_LEFT;
int AE_FASTZOMBIE_CLIMB_RIGHT;

//=========================================================
// tasks
//=========================================================
enum
{
	TASK_FASTZOMBIE_DO_ATTACK = LAST_SHARED_TASK + 100,	// again, my !!!HACKHACK
	TASK_FASTZOMBIE_LAND_RECOVER,
	TASK_FASTZOMBIE_UNSTICK_JUMP,
	TASK_FASTZOMBIE_JUMP_BACK,
	TASK_FASTZOMBIE_VERIFY_ATTACK,
};

//=========================================================
// activities
//=========================================================
int ACT_FASTZOMBIE_LEAP_SOAR;
int ACT_FASTZOMBIE_LEAP_STRIKE;
int ACT_FASTZOMBIE_LAND_RIGHT;
int ACT_FASTZOMBIE_LAND_LEFT;
int ACT_FASTZOMBIE_FRENZY;
int ACT_FASTZOMBIE_BIG_SLASH;

//=========================================================
// schedules
//=========================================================
enum
{
	SCHED_FASTZOMBIE_RANGE_ATTACK1 = LAST_SHARED_SCHEDULE + 100, // hack to get past the base zombie's schedules
	SCHED_FASTZOMBIE_UNSTICK_JUMP,
	SCHED_FASTZOMBIE_CLIMBING_UNSTICK_JUMP,
	SCHED_FASTZOMBIE_MELEE_ATTACK1,
	SCHED_FASTZOMBIE_TORSO_MELEE_ATTACK1,
};



//=========================================================
//=========================================================
class CFastZombie : public CNPC_BaseZombie
{
	DECLARE_CLASS(CFastZombie, CNPC_BaseZombie);

public:
	void Spawn(void);
	void Precache(void);

	void SetZombieModel(void);
	bool CanSwatPhysicsObjects(void) { return false; }

	int	TranslateSchedule(int scheduleType);

	Activity NPC_TranslateActivity(Activity baseAct);

	void LeapAttackTouch(CBaseEntity *pOther);
	void ClimbTouch(CBaseEntity *pOther);

	void StartTask(const Task_t *pTask);
	void RunTask(const Task_t *pTask);
	int SelectSchedule(void);
	void OnScheduleChange(void);

	void PrescheduleThink(void);

	float InnateRange1MaxRange(void);
	int RangeAttack1Conditions(float flDot, float flDist);
	int MeleeAttack1Conditions(float flDot, float flDist);

	virtual float GetClawAttackRange() const { return 50; }

	bool ShouldPlayFootstepMoan(void) { return false; }

	void HandleAnimEvent(animevent_t *pEvent);

	void PostNPCInit(void);

	void LeapAttack(void);
	void LeapAttackSound(void);

	void BecomeTorso(const Vector &vecTorsoForce, const Vector &vecLegsForce);

	bool IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const;
	bool MovementCost(int moveType, const Vector &vecStart, const Vector &vecEnd, float *pCost);
	bool ShouldFailNav(bool bMovementFailed);

	int	SelectFailSchedule(int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode);

	const char *GetMoanSound(int nSound);

	void OnChangeActivity(Activity NewActivity);
	void OnStateChange(NPC_STATE OldState, NPC_STATE NewState);
	void Event_Killed(const CTakeDamageInfo &info);
	bool ShouldBecomeTorso(const CTakeDamageInfo &info, float flDamageThreshold);

	virtual Vector GetAutoAimCenter() { return WorldSpaceCenter() - Vector(0, 0, 12.0f); }

	void PainSound(const CTakeDamageInfo &info);
	void DeathSound(const CTakeDamageInfo &info);
	void AlertSound(void);
	void IdleSound(void);
	void AttackSound(void);
	void AttackHitSound(void);
	void AttackMissSound(void);
	void FootstepSound(bool fRightFoot);
	void FootscuffSound(bool fRightFoot) {}; // fast guy doesn't scuff
	void StopLoopingSounds(void);

	void SoundInit(void);
	void SetIdleSoundState(void);
	void SetAngrySoundState(void);

	void BuildScheduleTestBits(void);

	void BeginNavJump(void);
	void EndNavJump(void);

	bool IsNavJumping(void) { return m_fIsNavJumping; }
	void OnNavJumpHitApex(void);

	void BeginAttackJump(void);
	void EndAttackJump(void);

	float		MaxYawSpeed(void);

	virtual const char *GetHeadcrabClassname(void);
	virtual const char *GetHeadcrabModel(void);
	virtual const char *GetLegsModel(void);
	virtual const char *GetTorsoModel(void);

	//=============================================================================
#ifdef HL2_EPISODIC

public:
	virtual bool	CreateBehaviors(void);
	virtual void	VPhysicsCollision(int index, gamevcollisionevent_t *pEvent);
	virtual	void	UpdateEfficiency(bool bInPVS);
	virtual bool	IsInAVehicle(void);
	void			InputAttachToVehicle(inputdata_t &inputdata);
	void			VehicleLeapAttackTouch(CBaseEntity *pOther);

private:
	void			VehicleLeapAttack(void);
	bool			CanEnterVehicle(CPropJeepEpisodic *pVehicle);

	CAI_PassengerBehaviorZombie		m_PassengerBehavior;

#endif	// HL2_EPISODIC
	//=============================================================================

protected:

	static const char *pMoanSounds[];

	// Sound stuff
	float			m_flDistFactor;
	unsigned char	m_iClimbCount; // counts rungs climbed (for sound)
	bool			m_fIsNavJumping;
	bool			m_fIsAttackJumping;
	bool			m_fHitApex;
	mutable float	m_flJumpDist;

	bool			m_fHasScreamed;

private:
	float	m_flNextMeleeAttack;
	bool	m_fJustJumped;
	float	m_flJumpStartAltitude;
	float	m_flTimeUpdateSound;

	CSoundPatch	*m_pLayer2; // used for climbing ladders, and when jumping (pre apex)

public:
	DEFINE_CUSTOM_AI;
	DECLARE_DATADESC();
};

#endif // NPC_FASTZOMBIE_H