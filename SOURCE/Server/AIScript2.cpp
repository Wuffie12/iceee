#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "AIScript2.h"
#include "FileReader.h"
#include "StringList.h"
#include "InstanceScript.h"
#include "Instance.h"
#include "Creature.h"
#include "Ability2.h"
#include "Util.h"
#include "DirectoryAccess.h"
#include "Report.h"
#include "Config.h"
#include "DirectoryAccess.h"

const int USE_FAIL_DELAY = 250; //Milliseconds to wait before retrying a failed script "use" command.

AINutManager aiNutManager;

AINutDef::~AINutDef() {
}

UseCallback::UseCallback(AINutPlayer *aiNut, int abilityID) {
	mAiNut = aiNut;
	mAbilityID = abilityID;
}
UseCallback::~UseCallback() {
}

bool UseCallback::Execute() {
	mAiNut->Use(mAbilityID);
	return true;
}

AINutPlayer::AINutPlayer() {
	attachedCreature = NULL;
}

AINutPlayer::~AINutPlayer() {
}

void AINutPlayer::Initialize(CreatureInstance *creature, AINutDef *defPtr,
		std::string &errors) {
	attachedCreature = creature;
	SetInstancePointer(attachedCreature->actInst);
	NutPlayer::Initialize(defPtr, errors);
}

void AINutPlayer::HaltedDerived() {}
void AINutPlayer::HaltDerivedExecution() {}

bool AINutPlayer::HasTarget() {
	return attachedCreature->CurrentTarget.targ != NULL;
}

void AINutPlayer::Use(int abilityID) {

	if (attachedCreature->ab[0].bPending == false) {
		//DEBUG OUTPUT
		if (g_Config.DebugLogAIScriptUse == true) {
			const Ability2::AbilityEntry2* abptr =
					g_AbilityManager.GetAbilityPtrByID(abilityID);
			g_Log.AddMessageFormat("Using: %s",
					abptr->GetRowAsCString(Ability2::ABROW::NAME));
		}
		//END DEBUG OUTPUT

		int r = attachedCreature->CallAbilityEvent(abilityID,
				EventType::onRequest);
		if (r != 0) {
			//Notify the creature we failed, may need a distance check.
			//The script should wait and retry soon.
			attachedCreature->AICheckAbilityFailure(r);

			if (g_Config.DebugLogAIScriptUse == true) {
				const Ability2::AbilityEntry2* abptr =
						g_AbilityManager.GetAbilityPtrByID(abilityID);
				g_Log.AddMessageFormat("Using: %s   Failed: %d",
						abptr->GetRowAsCString(Ability2::ABROW::NAME),
						g_AbilityManager.GetAbilityErrorCode(r));
			}

			if (attachedCreature->AIAbilityFailureAllowRetry(r) == true)
				QueueAdd(new ScriptCore::NutScriptEvent(
							new ScriptCore::TimeCondition(USE_FAIL_DELAY),
							new UseCallback(this, abilityID)));
		}
	}
}

short AINutPlayer::GetWill() {
	return attachedCreature->css.will;
}

short AINutPlayer::GetWillCharge() {
	return attachedCreature->css.will_charges;
}

short AINutPlayer::GetMight() {
	return attachedCreature->css.might;
}

short AINutPlayer::GetMightCharge() {
	return attachedCreature->css.might_charges;
}

short AINutPlayer::GetLevel() {
	return attachedCreature->css.level;
}

bool AINutPlayer::IsOnCooldown(const char *category) {
	int cooldownID = g_AbilityManager.ResolveCooldownCategoryID(category);
	return attachedCreature->HasCooldown(cooldownID);
}

bool AINutPlayer::IsBusy() {
	return attachedCreature->AICheckIfAbilityBusy();
}

short AINutPlayer::CountEnemyNear(int range) {
	return attachedCreature->AICountEnemyNear(range,
			(float) attachedCreature->CurrentX,
			(float) attachedCreature->CurrentZ);
}

short AINutPlayer::CountEnemyAt(int range) {
	float x = (float) attachedCreature->CurrentX;
	float z = (float) attachedCreature->CurrentZ;
	if (attachedCreature->CurrentTarget.targ != NULL) {
		x = (float) attachedCreature->CurrentTarget.targ->CurrentX;
		z = (float) attachedCreature->CurrentTarget.targ->CurrentZ;
	}
	return attachedCreature->AICountEnemyNear(range, x, z);
}

short AINutPlayer::HealthPercent() {
	return static_cast<short>(attachedCreature->GetHealthRatio() * 100.0F);
}

short AINutPlayer::TargetHealthPercent() {
	short health = 0;
	if (attachedCreature->CurrentTarget.targ != NULL)
		health =
				static_cast<int>(attachedCreature->CurrentTarget.targ->GetHealthRatio()
						* 100.0F);
	return health;
}

void AINutPlayer::VisualEffect(const char *effect) {
	attachedCreature->SendEffect(effect, 0);
}

void AINutPlayer::TargetVisualEffect(const char *effect) {
	int targID = 0;
	if (attachedCreature->CurrentTarget.targ != NULL)
		targID = attachedCreature->CurrentTarget.targ->CreatureID;
	attachedCreature->SendEffect(effect, targID);
}

void AINutPlayer::Say(const char *text) {
	attachedCreature->SendSay(text);
}

void AINutPlayer::InstanceCall(const char *text) {
	attachedCreature->actInst->ScriptCall(text);
}

int AINutPlayer::GetIdleMob(int CDefID) {
	return attachedCreature->AIGetIdleMob(CDefID);
}

int AINutPlayer::GetTarget() {
	return attachedCreature->CurrentTarget.targ != NULL ?
			attachedCreature->CurrentTarget.targ->CreatureID : 0;
}

int AINutPlayer::GetSelf() {
	return attachedCreature->CreatureID;
}

void AINutPlayer::SetOtherTarget(int CID, int targetCID) {
	attachedCreature->AIOtherSetTarget(CID, targetCID);
}

bool AINutPlayer::IsTargetEnemy() {
	return attachedCreature->AIIsTargetEnemy();
}

bool AINutPlayer::IsTargetFriendly() {
	return attachedCreature->AIIsTargetFriend();
}

void AINutPlayer::SetSpeed(int speed) {
	attachedCreature->Speed = speed;
}

int AINutPlayer::GetTargetCDefID() {
	return attachedCreature->CurrentTarget.targ != NULL ?
			attachedCreature->CurrentTarget.targ->CreatureDefID : 0;
}

int AINutPlayer::GetProperty(const char *name) {
	return static_cast<int>(attachedCreature->AIGetProperty(name, false));
}

int AINutPlayer::GetTargetProperty(const char *name) {
	return static_cast<int>(attachedCreature->AIGetProperty(name, true));
}

void AINutPlayer::DispelTargetProperty(const char *name, int sign) {
	attachedCreature->AIDispelTargetProperty(name, sign);
}

void AINutPlayer::PlaySound(const char *name) {
	STRINGLIST sub;
	Util::Split(name, "|", sub);
	while (sub.size() < 2) {
		sub.push_back("");
	}
	attachedCreature->SendPlaySound(sub[0].c_str(), sub[1].c_str());
}

int AINutPlayer::GetBuffTier(int abilityGroupID) {
	return attachedCreature->AIGetBuffTier(abilityGroupID, false);
}

int AINutPlayer::GetTargetBuffTier(int abilityGroupID) {
	return attachedCreature->AIGetBuffTier(abilityGroupID, true);
}

bool AINutPlayer::IsTargetInRange(float distance) {
	return attachedCreature->InRange_Target(distance);
}

int AINutPlayer::GetTargetRange() {
	return attachedCreature->AIGetTargetRange();
}

void AINutPlayer::SetGTAE() {
	attachedCreature->AISetGTAE();
}

void AINutPlayer::Speak(const char *message) {
	CreatureChat(attachedCreature->CreatureID, "s/", message);
}

int AINutPlayer::GetSpeed(int CID) {
	CreatureInstance *targ = ResolveCreatureInstance(CID);
	return targ == NULL ? 0 : targ->Speed;
}

bool AINutPlayer::IsCIDBusy(int CID) {
	CreatureInstance *targ = ResolveCreatureInstance(CID);
	return targ == NULL ? 0 : targ->AICheckIfAbilityBusy();
}

void AINutPlayer::RegisterFunctions() {
	// AI scripts should by default run when idle

	Sqrat::Class<NutPlayer> nutClass(vm, _SC("Core"), true);
	Sqrat::RootTable(vm).Bind(_SC("Core"), nutClass);
	RegisterCoreFunctions(this, &nutClass);
	Sqrat::DerivedClass<InstanceScript::AbstractInstanceNutPlayer, NutPlayer> abstractInstanceClass(vm, _SC("AbstractInstance"));
	Sqrat::DerivedClass<InstanceScript::InstanceNutPlayer, AbstractInstanceNutPlayer> instanceClass(vm, _SC("Instance"));
	Sqrat::DerivedClass<AINutPlayer, InstanceScript::InstanceNutPlayer> aiClass(vm, _SC("AI"));
	Sqrat::RootTable(vm).Bind(_SC("AI"), aiClass);
	RegisterAIFunctions(this, &aiClass);
	Sqrat::RootTable(vm).SetInstance(_SC("ai"), this);

	if(attachedCreature != NULL && attachedCreature->actInst != NULL) {
//		InstanceScript::InstanceNutPlayer *player = &attachedCreature->actInst->nutScriptPlayer;
//		if(player->active) {
//			Sqrat::RootTable rt = Sqrat::RootTable(player->vm);
//			Sqrat::DerivedClass<InstanceScript::InstanceNutPlayer, NutPlayer> instanceClass(vm, _SC("Instance"));
//			player->RegisterInstanceFunctions(player, &instanceClass);
////			Sqrat::RootTable(vm).SetInstance(_SC("inst"), player);
//			Sqrat::RootTable(vm).Bind(_SC("inst"), rt);
//		}
	}

}

void AINutPlayer::RegisterAIFunctions(NutPlayer *instance,
		Sqrat::DerivedClass<AINutPlayer, InstanceScript::InstanceNutPlayer> *clazz) {
	clazz->Func(_SC("has_target"), &AINutPlayer::HasTarget);
	clazz->Func(_SC("use"), &AINutPlayer::Use);
	clazz->Func(_SC("get_will"), &AINutPlayer::GetWill);
	clazz->Func(_SC("get_will_charge"), &AINutPlayer::GetWillCharge);
	clazz->Func(_SC("get_might"), &AINutPlayer::GetMight);
	clazz->Func(_SC("get_might_charge"), &AINutPlayer::GetMightCharge);
	clazz->Func(_SC("get_level"), &AINutPlayer::GetLevel);
	clazz->Func(_SC("is_on_cooldown"), &AINutPlayer::IsOnCooldown);
	clazz->Func(_SC("is_busy"), &AINutPlayer::IsBusy);
	clazz->Func(_SC("count_enemy_at"), &AINutPlayer::CountEnemyAt);
	clazz->Func(_SC("count_enemy_near"), &AINutPlayer::CountEnemyNear);
	clazz->Func(_SC("health_pc"), &AINutPlayer::HealthPercent);
	clazz->Func(_SC("target_health_pc"), &AINutPlayer::TargetHealthPercent);
	clazz->Func(_SC("play_sound"), &AINutPlayer::PlaySound);
	clazz->Func(_SC("visual_effect"), &AINutPlayer::VisualEffect);
	clazz->Func(_SC("target_visual_effect"), &AINutPlayer::TargetVisualEffect);
	clazz->Func(_SC("say"), &AINutPlayer::Say);
	clazz->Func(_SC("instance_call"), &AINutPlayer::InstanceCall);
	clazz->Func(_SC("get_idle_mob"), &AINutPlayer::GetIdleMob);
	clazz->Func(_SC("get_target"), &AINutPlayer::GetTarget);
	clazz->Func(_SC("get_self"), &AINutPlayer::GetSelf);
	clazz->Func(_SC("set_other_target"), &AINutPlayer::SetOtherTarget);
	clazz->Func(_SC("is_target_enemy"), &AINutPlayer::IsTargetEnemy);
	clazz->Func(_SC("is_target_friendly"), &AINutPlayer::IsTargetFriendly);
	clazz->Func(_SC("set_speed"), &AINutPlayer::SetSpeed);
	clazz->Func(_SC("get_target_cdef_id"), &AINutPlayer::GetTargetCDefID);
	clazz->Func(_SC("get_target"), &AINutPlayer::GetProperty);
	clazz->Func(_SC("get_target_property"), &AINutPlayer::GetTargetProperty);
	clazz->Func(_SC("dispel_target_property"),
			&AINutPlayer::DispelTargetProperty);
	clazz->Func(_SC("get_buff_tier"), &AINutPlayer::GetBuffTier);
	clazz->Func(_SC("get_target_buff_tier"), &AINutPlayer::GetTargetBuffTier);
	clazz->Func(_SC("is_target_in_range"), &AINutPlayer::IsTargetInRange);
	clazz->Func(_SC("get_target_range"), &AINutPlayer::GetTargetRange);
	clazz->Func(_SC("set_gtae"), &AINutPlayer::SetGTAE);
	clazz->Func(_SC("get_speed"), &AINutPlayer::GetSpeed);
	clazz->Func(_SC("is_cid_busy"), &AINutPlayer::IsCIDBusy);
	clazz->Func(_SC("get_npc_id"), &AINutPlayer::GetNPCID);
	clazz->Func(_SC("speak"), &AINutPlayer::Speak);

	// Common instance functions (TODO register in abstract class somehow)
	clazz->Func(_SC("broadcast"), &AINutPlayer::Broadcast);
	clazz->Func(_SC("local_broadcast"), &AINutPlayer::LocalBroadcast);
	clazz->Func(_SC("info"), &AINutPlayer::Info);
	clazz->Func(_SC("message"), &AINutPlayer::Message);
	clazz->Func(_SC("error"), &AINutPlayer::Error);
	clazz->Func(_SC("get_cid_for_prop"), &AINutPlayer::GetCIDForPropID);
	clazz->Func(_SC("chat"), &AINutPlayer::Chat);
	clazz->Func(_SC("creature_chat"), &AINutPlayer::CreatureChat);

	// Functions that return arrays or tables have to be dealt with differently
	clazz->SquirrelFunc(_SC("cids"), &AINutPlayer::CIDs);

}

CreatureInstance* AINutPlayer::ResolveCreatureInstance(int CreatureInstanceID) {
	if (attachedCreature == NULL)
		return NULL;
	if (attachedCreature->actInst == NULL)
		return NULL;
	return attachedCreature->actInst->GetInstanceByCID(CreatureInstanceID);
}

void AINutPlayer::DebugGenerateReport(ReportBuffer &report) {
	if (def != NULL) {
		report.AddLine("Name:%s", def->scriptName.c_str());
	}
	report.AddLine("active:%d", static_cast<int>(mActive));
	report.AddLine(NULL);
}

AINutManager::AINutManager() {
}

AINutManager::~AINutManager() {
	aiDef.clear();
	aiAct.clear();
}

int AINutManager::LoadScripts(void) {
	Platform_DirectoryReader r;
	string dir = r.GetDirectory();
	r.SetDirectory("AIScript");
	r.ReadFiles();
	r.SetDirectory(dir.c_str());

	vector<std::string>::iterator it;
	for (it = r.fileList.begin(); it != r.fileList.end(); ++it) {
		std::string p = *it;
		if (Util::HasEnding(p, ".nut")) {
			// TODO delete this when finished with
			AINutDef *def = new AINutDef();
			aiDef.push_back(def);
			char buf[128];
			Util::SafeFormat(buf, sizeof(buf), "AIScript/%s", p.c_str());
			Platform::FixPaths(buf);
			def->Initialize(buf);
		}
	}

	return 0;
}

AINutDef * AINutManager::GetScriptByName(const char *name) {
	if (aiDef.size() == 0)
		return NULL;
	list<AINutDef*>::iterator it;
	for (it = aiDef.begin(); it != aiDef.end(); ++it) {
		AINutDef *ai = *it;
		if (ai->scriptName.compare(name) == 0)
			return *it;
	}
	return NULL;
}

//void AINutPlayer::Queue(Sqrat::Function function, int fireDelay) {
//
//	DoQueue(new ScriptCore::NutScriptEvent(
//				new ScriptCore::TimeCondition(fireDelay),
//				new ScriptCore::SquirrelFunctionCallback(this, function)));
//}

AINutPlayer * AINutManager::AddActiveScript(CreatureInstance *creature,
		AINutDef *def, std::vector<std::string> args, std::string &errors) {
	AINutPlayer * player = new AINutPlayer();
	player->mArgs = args;
	player->Initialize(creature, def, errors);
	if (errors.length() > 0)
		g_Log.AddMessageFormat("Failed to compile %s. %s",
				def->scriptName.c_str(), errors.c_str());
	aiAct.push_back(player);
	return player;
}

void AINutManager::RemoveActiveScript(AINutPlayer *registeredPtr) {
	list<AINutPlayer*>::iterator it;
	for (it = aiAct.begin(); it != aiAct.end(); ++it) {
		if (*it == registeredPtr) {
			delete (*it);
			aiAct.erase(it);
			break;
		}
	}
}

