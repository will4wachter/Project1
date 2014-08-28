/*
===========================================================================
Copyright (c) 2010-2012 Darkstar Dev Teams
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program. If not, see http://www.gnu.org/licenses/
This file is part of DarkStar-server source code.
===========================================================================
*/
#include "../../common/timer.h"
#include "../../common/utils.h"
#include <math.h>
#include <string.h>
#include <algorithm>
#include "../packets/char_health.h"
#include "../packets/char_update.h"
#include "../packets/entity_update.h"
#include "../packets/message_basic.h"
#include "../packets/inventory_finish.h"
#include "../lua/luautils.h"
#include "../ability.h"
#include "../modifier.h"
#include "charutils.h"
#include "battleutils.h"
#include "attackutils.h"
#include "../attack.h"
#include "../map.h"
#include "../party.h"
#include "../alliance.h"
#include "../spell.h"
#include "../trait.h"
#include "../weapon_skill.h"
#include "../mobskill.h"
#include "../entities/mobentity.h"
#include "../entities/petentity.h"
#include "../enmity_container.h"
#include "../items.h"
#include "../packets/pet_sync.h"
#include "../packets/char_sync.h"
#include "../packets/position.h"
#include "../packets/lock_on.h"
#include "../ai/ai_pet_dummy.h"
#include "zoneutils.h"
/************************************************************************
* lists used in battleutils *
************************************************************************/
uint16 g_SkillTable[100][13];	// All Skills by level/skilltype
uint8 g_EnmityTable[100][2];	// Holds Enmity Modifier Values
uint8 g_SkillRanks[MAX_SKILLTYPE][MAX_JOBTYPE];	// Holds skill ranks by skilltype and job
uint16 g_SkillChainDamageModifiers[MAX_SKILLCHAIN_LEVEL + 1][MAX_SKILLCHAIN_COUNT + 1]; // Holds damage modifiers for skill chains [chain level][chain count]
CWeaponSkill* g_PWeaponSkillList[MAX_WEAPONSKILL_ID];	// Holds all Weapon skills
CMobSkill* g_PMobSkillList[MAX_MOBSKILL_ID];	// List of mob skills
std::list<CWeaponSkill*> g_PWeaponSkillsList[MAX_SKILLTYPE];	// Holds Weapon skills by type
std::vector<CMobSkill*> g_PMobFamilySkills[MAX_MOB_FAMILY];	// Mob Skills By Family
/************************************************************************
* battleutils *
************************************************************************/
namespace battleutils
{
	/************************************************************************
	* *
	* Generate Enmity Table *
	* *
	************************************************************************/
	void LoadEnmityTable()
	{
		for (uint32 x = 0; x < 100; ++x)
		{
			g_EnmityTable[x][0] = (uint8)abs(0.5441*x + 13.191); // cmod
			g_EnmityTable[x][1] = (uint8)abs(0.6216*x + 5.4363); // dmod
		}
	}
	/************************************************************************
	* *
	* *
	* *
	************************************************************************/
	void LoadSkillTable()
	{
		memset(g_SkillTable, 0, sizeof(g_SkillTable));
		memset(g_SkillRanks, 0, sizeof(g_SkillRanks));
		const int8* fmtQuery = "SELECT r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13 \
							   FROM skill_caps \
							   ORDER BY level \
							   LIMIT 100";
		int32 ret = Sql_Query(SqlHandle, fmtQuery);
		if (ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0)
		{
			for (uint32 x = 0; x < 100 && Sql_NextRow(SqlHandle) == SQL_SUCCESS; ++x)
			{
				for (uint32 y = 0; y < 14; ++y)
				{
					g_SkillTable[x][y] = (uint16)Sql_GetIntData(SqlHandle, y);
				}
			}
		}
		fmtQuery = "SELECT skillid,war,mnk,whm,blm,rdm,thf,pld,drk,bst,brd,rng,sam,nin,drg,smn,blu,cor,pup,dnc,sch,geo,run \
				   FROM skill_ranks \
				   LIMIT 64";
		ret = Sql_Query(SqlHandle, fmtQuery);
		if (ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0)
		{
			for (uint32 x = 0; x < MAX_SKILLTYPE && Sql_NextRow(SqlHandle) == SQL_SUCCESS; ++x)
			{
				uint8 SkillID = dsp_cap((uint8)Sql_GetIntData(SqlHandle, 0), 0, MAX_SKILLTYPE - 1);
				for (uint32 y = 1; y < MAX_JOBTYPE; ++y)
				{
					g_SkillRanks[SkillID][y] = dsp_cap((uint16)Sql_GetIntData(SqlHandle, y), 0, 11);
				}
			}
		}
	}
	/************************************************************************
	* Load Weapon Skills from database *
	************************************************************************/
	void LoadWeaponSkillsList()
	{
		memset(g_PWeaponSkillList, 0, sizeof(g_PWeaponSkillList));
		const int8* fmtQuery = "SELECT weaponskillid, name, jobs, type, skilllevel, element, animation, `range`, aoe, primary_sc, secondary_sc, tertiary_sc, main_only \
							   FROM weapon_skills \
							   WHERE weaponskillid < %u \
							   ORDER BY type, skilllevel ASC";
		int32 ret = Sql_Query(SqlHandle, fmtQuery, MAX_WEAPONSKILL_ID);
		if (ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0)
		{
			while (Sql_NextRow(SqlHandle) == SQL_SUCCESS)
			{
				CWeaponSkill* PWeaponSkill = new CWeaponSkill(Sql_GetIntData(SqlHandle, 0));
				PWeaponSkill->setName(Sql_GetData(SqlHandle, 1));
				PWeaponSkill->setJob(Sql_GetData(SqlHandle, 2));
				PWeaponSkill->setType(Sql_GetIntData(SqlHandle, 3));
				PWeaponSkill->setSkillLevel(Sql_GetIntData(SqlHandle, 4));
				PWeaponSkill->setElement(Sql_GetIntData(SqlHandle, 5));
				PWeaponSkill->setAnimationId(Sql_GetIntData(SqlHandle, 6));
				PWeaponSkill->setRange(Sql_GetIntData(SqlHandle, 7));
				PWeaponSkill->setAoe(Sql_GetIntData(SqlHandle, 8));
				PWeaponSkill->setPrimarySkillchain(Sql_GetIntData(SqlHandle, 9));
				PWeaponSkill->setSecondarySkillchain(Sql_GetIntData(SqlHandle, 10));
				PWeaponSkill->setTertiarySkillchain(Sql_GetIntData(SqlHandle, 11));
				PWeaponSkill->setMainOnly(Sql_GetIntData(SqlHandle, 12));
				g_PWeaponSkillList[PWeaponSkill->getID()] = PWeaponSkill;
				g_PWeaponSkillsList[PWeaponSkill->getType()].push_back(PWeaponSkill);
			}
		}
	}
	/************************************************************************
	* *
	* Load Mob Skills from database *
	* *
	************************************************************************/
	void LoadMobSkillsList()
	{
		memset(g_PMobSkillList, 0, sizeof(g_PMobSkillList));
		const int8* fmtQuery = "SELECT mob_skill_id, family_id, mob_anim_id, mob_skill_name, \
							   mob_skill_aoe, mob_skill_distance, mob_anim_time, mob_prepare_time, \
							   mob_valid_targets, mob_skill_flag, mob_skill_param, knockback \
							   FROM mob_skill \
							   WHERE mob_skill_id < %u \
							   ORDER BY family_Id, mob_skill_id ASC";
		int32 ret = Sql_Query(SqlHandle, fmtQuery, MAX_MOBSKILL_ID);
		if (ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0)
		{
			while (Sql_NextRow(SqlHandle) == SQL_SUCCESS)
			{
				CMobSkill* PMobSkill = new CMobSkill(Sql_GetIntData(SqlHandle, 0));
				PMobSkill->setfamilyID(Sql_GetIntData(SqlHandle, 1));
				PMobSkill->setAnimationID(Sql_GetIntData(SqlHandle, 2));
				PMobSkill->setName(Sql_GetData(SqlHandle, 3));
				PMobSkill->setAoe(Sql_GetIntData(SqlHandle, 4));
				PMobSkill->setDistance(Sql_GetFloatData(SqlHandle, 5));
				PMobSkill->setAnimationTime(Sql_GetIntData(SqlHandle, 6));
				PMobSkill->setActivationTime(Sql_GetIntData(SqlHandle, 7));
				PMobSkill->setValidTargets(Sql_GetIntData(SqlHandle, 8));
				PMobSkill->setFlag(Sql_GetIntData(SqlHandle, 9));
				PMobSkill->setParam(Sql_GetIntData(SqlHandle, 10));
				PMobSkill->setKnockback(Sql_GetUIntData(SqlHandle, 11));
				PMobSkill->setMsg(185); //standard damage message. Scripters will change this.
				g_PMobSkillList[PMobSkill->getID()] = PMobSkill;
				if (PMobSkill->getfamilyID() >= MAX_MOB_FAMILY)
				{
					ShowError("battleutils::LoadMobSkillsList Defined skill (%d) is out of range of (%d)\n", PMobSkill->getfamilyID(), MAX_MOB_FAMILY);
				}
				g_PMobFamilySkills[PMobSkill->getfamilyID()].push_back(PMobSkill);
			}
		}
	}
	void LoadSkillChainDamageModifiers()
	{
		memset(g_SkillChainDamageModifiers, 0, sizeof(g_SkillChainDamageModifiers));
		const int8* fmtQuery = "SELECT chain_level, chain_count, initial_modifier, magic_burst_modifier \
							   FROM skillchain_damage_modifiers \
							   ORDER BY chain_level, chain_count";
		int32 ret = Sql_Query(SqlHandle, fmtQuery);
		if (ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0)
		{
			for (uint32 x = 0; Sql_NextRow(SqlHandle) == SQL_SUCCESS; ++x)
			{
				uint16 level = (uint16)Sql_GetIntData(SqlHandle, 0);
				uint16 count = (uint16)Sql_GetIntData(SqlHandle, 1);
				uint16 value = (uint16)Sql_GetIntData(SqlHandle, 2);
				g_SkillChainDamageModifiers[level][count] = value;
			}
		}
		return;
	}
	/************************************************************************
	* Clear Weapon Skills List *
	************************************************************************/
	void FreeWeaponSkillsList()
	{
		for (int32 SkillId = 0; SkillId < MAX_WEAPONSKILL_ID; ++SkillId)
		{
			delete g_PWeaponSkillList[SkillId];
		}
	}
	/************************************************************************
	* Clear Mob Skills List *
	************************************************************************/
	void FreeMobSkillList()
	{
		for (int32 SkillID = 0; SkillID < MAX_MOBSKILL_ID; ++SkillID)
		{
			delete g_PMobSkillList[SkillID];
		}
	}
	void FreeSkillChainDamageModifiers()
	{
		// These aren't dynamicly allocated at this point so no need to free them.
	}
	/************************************************************************
	* Get Skill Rank By SkillId and JobId *
	************************************************************************/
	uint8 GetSkillRank(SKILLTYPE SkillID, JOBTYPE JobID)
	{
		return g_SkillRanks[SkillID][JobID];
	}
	/************************************************************************
	* Return Max Skill by SkillType, JobType, and level *
	************************************************************************/
	uint16 GetMaxSkill(SKILLTYPE SkillID, JOBTYPE JobID, uint8 level)
	{
		return g_SkillTable[level][g_SkillRanks[SkillID][JobID]];
	}
	uint16 GetMaxSkill(uint8 rank, uint8 level)
	{
		return g_SkillTable[level][rank];
	}
	bool isValidSelfTargetWeaponskill(int wsid){
		switch (wsid){
		case 163: //starlight
		case 164: //moonlight
		case 173: //dagan
		case 190: //myrkr
			return true;
		}
		return false;
	}
	/************************************************************************
	* *
	* Get Enmity Modifier *
	* *
	************************************************************************/
	uint8 GetEnmityMod(uint8 level, uint8 modType)
	{
		DSP_DEBUG_BREAK_IF(modType >= 2);
		if (level >= 100) { level = 99; }
		return g_EnmityTable[level][modType];
	}
	/************************************************************************
	* *
	* Get Weapon Skill by ID *
	* *
	************************************************************************/
	CWeaponSkill* GetWeaponSkill(uint16 WSkillID)
	{
		DSP_DEBUG_BREAK_IF(WSkillID >= MAX_WEAPONSKILL_ID);
		return g_PWeaponSkillList[WSkillID];
	}
	/************************************************************************
	* *
	* Get List of Weapon Skills from skill type *
	* *
	************************************************************************/
	std::list<CWeaponSkill*> GetWeaponSkills(uint8 skill)
	{
		DSP_DEBUG_BREAK_IF(skill >= MAX_SKILLTYPE);
		return g_PWeaponSkillsList[skill];
	}
	/************************************************************************
	* *
	* Get Mob Skill by Id *
	* *
	************************************************************************/
	CMobSkill* GetMobSkill(uint16 SkillID)
	{
		DSP_DEBUG_BREAK_IF(SkillID >= MAX_MOBSKILL_ID);
		return g_PMobSkillList[SkillID];
	}
	/************************************************************************
	* *
	* Get Mob Skills by family id *
	* *
	************************************************************************/
	std::vector<CMobSkill*> GetMobSkillsByFamily(uint16 FamilyID)
	{
		DSP_DEBUG_BREAK_IF(FamilyID >= sizeof(g_PMobFamilySkills));
		return g_PMobFamilySkills[FamilyID];
	}
	int32 CalculateEnspellDamage(CBattleEntity* PAttacker, CBattleEntity* PDefender, uint8 Tier, uint8 element){
		int32 damage = 0;
		//Tier 1 enspells have their damaged pre-calculated AT CAST TIME and is stored in MOD_ENSPELL_DMG
		if (Tier == 1)
		{
			damage = PAttacker->getMod(MOD_ENSPELL_DMG) + PAttacker->getMod(MOD_ENSPELL_DMG_BONUS);
		}
		else
		{
			//Tier 2 enspells calculate the damage on each hit and increment the potency in MOD_ENSPELL_DMG per hit
			uint16 skill = PAttacker->GetSkill(SKILL_ENH);
			uint16 cap = 3 + ((6 * skill) / 100);
			if (skill>200){
				cap = 5 + ((5 * skill) / 100);
			}
			cap *= 2;
			if (PAttacker->getMod(MOD_ENSPELL_DMG) > cap)
			{
				PAttacker->setModifier(MOD_ENSPELL_DMG, cap);
				damage = cap;
			}
			else if (PAttacker->getMod(MOD_ENSPELL_DMG) == cap)
			{
				damage = cap;
			}
			else if (PAttacker->getMod(MOD_ENSPELL_DMG) < cap)
			{
				PAttacker->addModifier(MOD_ENSPELL_DMG, 1);
				damage = PAttacker->getMod(MOD_ENSPELL_DMG) - 1;
			}
			damage += PAttacker->getMod(MOD_ENSPELL_DMG_BONUS);
		}
		//matching day 10% bonus, matching weather 10% or 25% for double weather
		float dBonus = 1.0;
		float resist = 1.0;
		uint32 WeekDay = CVanaTime::getInstance()->getWeekday();
		WEATHER weather = GetWeather(PAttacker, false);
		DAYTYPE strongDay[8] = { FIRESDAY, EARTHSDAY, WATERSDAY, WINDSDAY, ICEDAY, LIGHTNINGDAY, LIGHTSDAY, DARKSDAY };
		DAYTYPE weakDay[8] = { WATERSDAY, WINDSDAY, LIGHTNINGDAY, ICEDAY, FIRESDAY, EARTHSDAY, DARKSDAY, LIGHTSDAY };
		WEATHER strongWeatherSingle[8] = { WEATHER_HOT_SPELL, WEATHER_DUST_STORM, WEATHER_RAIN, WEATHER_WIND, WEATHER_SNOW, WEATHER_THUNDER, WEATHER_AURORAS, WEATHER_GLOOM };
		WEATHER strongWeatherDouble[8] = { WEATHER_HEAT_WAVE, WEATHER_SAND_STORM, WEATHER_SQUALL, WEATHER_GALES, WEATHER_BLIZZARDS, WEATHER_THUNDERSTORMS, WEATHER_STELLAR_GLARE, WEATHER_DARKNESS };
		WEATHER weakWeatherSingle[8] = { WEATHER_RAIN, WEATHER_WIND, WEATHER_THUNDER, WEATHER_SNOW, WEATHER_HOT_SPELL, WEATHER_DUST_STORM, WEATHER_GLOOM, WEATHER_AURORAS };
		WEATHER weakWeatherDouble[8] = { WEATHER_SQUALL, WEATHER_GALES, WEATHER_THUNDERSTORMS, WEATHER_BLIZZARDS, WEATHER_HEAT_WAVE, WEATHER_SAND_STORM, WEATHER_DARKNESS, WEATHER_STELLAR_GLARE };
		uint32 obi[8] = { 15435, 15438, 15440, 15437, 15436, 15439, 15441, 15442 };
		MODIFIER absorb[8] = { MOD_FIRE_ABSORB, MOD_EARTH_ABSORB, MOD_WATER_ABSORB, MOD_WIND_ABSORB, MOD_ICE_ABSORB, MOD_LTNG_ABSORB, MOD_LIGHT_ABSORB, MOD_DARK_ABSORB };
		MODIFIER nullarray[8] = { MOD_FIRE_NULL, MOD_EARTH_NULL, MOD_WATER_NULL, MOD_WIND_NULL, MOD_ICE_NULL, MOD_LTNG_NULL, MOD_LIGHT_NULL, MOD_DARK_NULL };
		MODIFIER resistarray[8] = { MOD_FIRERES, MOD_EARTHRES, MOD_WATERRES, MOD_WINDRES, MOD_ICERES, MOD_THUNDERRES, MOD_LIGHTRES, MOD_DARKRES };
		bool obiBonus = false;
		double half = (double)(PDefender->getMod(resistarray[element])) / 100;
		double quart = pow(half, 2);
		double eighth = pow(half, 3);
		double sixteenth = pow(half, 4);
		double resvar = WELL512::drand();
		// Determine resist based on which thresholds have been crossed.
		if (resvar <= sixteenth)
			resist = 0.0625;
		else if (resvar <= eighth)
			resist = 0.125;
		else if (resvar <= quart)
			resist = 0.25;
		else if (resvar <= half)
			resist = 0.5;
		if (PAttacker->objtype == TYPE_PC)
		{
			CItemArmor* waist = ((CCharEntity*)PAttacker)->getEquip(SLOT_WAIST);
			if (waist && waist->getID() == obi[element])
			{
				obiBonus = true;
			}
		}
		else
		{
			// mobs random multiplier
			dBonus += (WELL512::irand() % 101) / 1000.0f;
		}
		if (WeekDay == strongDay[element] && (obiBonus || WELL512::irand() % 100 < 33))
			dBonus += 0.1;
		else if (WeekDay == weakDay[element] && (obiBonus || WELL512::irand() % 100 < 33))
			dBonus -= 0.1;
		if (weather == strongWeatherSingle[element] && (obiBonus || WELL512::irand() % 100 < 33))
			dBonus += 0.1;
		else if (weather == strongWeatherDouble[element] && (obiBonus || WELL512::irand() % 100 < 33))
			dBonus += 0.25;
		else if (weather == weakWeatherSingle[element] && (obiBonus || WELL512::irand() % 100 < 33))
			dBonus -= 0.1;
		else if (weather == weakWeatherDouble[element] && (obiBonus || WELL512::irand() % 100 < 33))
			dBonus -= 0.25;
		damage = (damage * (float)resist);
		damage = (damage * (float)dBonus);
		damage = MagicDmgTaken(PDefender, damage);
		damage = damage - PDefender->getMod(MOD_PHALANX);
		damage = dsp_cap(damage, 0, 99999);
		if (WELL512::irand() % 100 < PDefender->getMod(absorb[element]) || WELL512::irand() % 100 < PDefender->getMod(MOD_MAGIC_ABSORB))
			damage = -damage;
		else if (WELL512::irand() % 100 < PDefender->getMod(nullarray[element]) || WELL512::irand() % 100 < PDefender->getMod(MOD_MAGIC_NULL))
			damage = 0;
		else
			damage = HandleStoneskin(PDefender, damage);
		return damage;
	}
	/************************************************************************
	* *
	* Calculates Spike Damage *
	* *
	************************************************************************/
	uint16 CalculateSpikeDamage(CBattleEntity* PAttacker, CBattleEntity* PDefender, uint8 spikesType, uint16 damageTaken)
	{
		uint16 damage = PDefender->getMod(MOD_SPIKES_DMG);
		int16 intStat = PDefender->INT();
		int16 mattStat = PDefender->getMod(MOD_MATT);
		switch (spikesType){
		case SPIKE_DREAD:
			// drain same as damage taken
			damage = damageTaken;
			break;
		case SPIKE_REPRISAL:
			damage += (float)damageTaken*0.3;
			if (PDefender->StatusEffectContainer->HasStatusEffect(EFFECT_SENTINEL)){
				// bonus
				damage *= WELL512::irand() % 2 + 1;
			}
			break;
		default:
			break;
		}
		return damage;
	}
	bool HandleSpikesDamage(CBattleEntity* PAttacker, CBattleEntity* PDefender, apAction_t* Action, uint32 damage)
	{
		uint16 spikes = PDefender->getMod(MOD_SPIKES);
		Action->spikesMessage = 44;
		if (spikes)
		{
			Action->spikesParam = HandleStoneskin(PAttacker, CalculateSpikeDamage(PAttacker, PDefender, spikes, damage));
			// handle level diff
			int lvlDiff = dsp_cap((PDefender->GetMLevel() - PAttacker->GetMLevel()), -5, 5) * 2;
			switch (spikes){
			case SPIKE_BLAZE:
				Action->spikesEffect = SUBEFFECT_BLAZE_SPIKES;
				PAttacker->addHP(-Action->spikesParam);
				break;
			case SPIKE_ICE:
				Action->spikesEffect = SUBEFFECT_ICE_SPIKES;
				PAttacker->addHP(-Action->spikesParam);
				break;
			case SPIKE_SHOCK:
				Action->spikesEffect = SUBEFFECT_SHOCK_SPIKES;
				PAttacker->addHP(-Action->spikesParam);
				break;
			case SPIKE_DREAD:
				if (PAttacker->m_EcoSystem == SYSTEM_UNDEAD){
					// is undead no effect
					return false;
				}
				else {
					Action->addEffectMessage = 132;
					PAttacker->addHP(-Action->spikesParam);
					PDefender->addHP(Action->spikesParam);
					if (PDefender->objtype == TYPE_PC){
						charutils::UpdateHealth((CCharEntity*)PDefender);
					}
					Action->spikesEffect = SUBEFFECT_DREAD_SPIKES;
				}
				break;
			case SPIKE_REPRISAL:
				if (Action->reaction == REACTION_BLOCK){
					Action->spikesEffect = SUBEFFECT_REPRISAL;
					PAttacker->addHP(-Action->spikesParam);
				}
				else {
					// only works on shield blocks
					return false;
				}
				break;
			}
			if (PAttacker->objtype == TYPE_PC){
				charutils::UpdateHealth((CCharEntity*)PAttacker);
			}
			return true;
		}
		else if (PDefender->objtype == TYPE_PC){
			CCharEntity* PCharDef = (CCharEntity*)PDefender;
			bool activate = false;
			uint8 chance;
			SUBEFFECT spikesEffect = SUBEFFECT_NONE;
			uint8 damage;
			// SHIELD
			CItem* PItem = PCharDef->getEquip(SLOT_SUB);
			if (PItem){
				switch (PItem->getID())
				{
				case 16169: //collaber shield
					spikesEffect = SUBEFFECT_BLAZE_SPIKES;
					damage = 28;
					chance = 25;
					break;
				case 12305: // ice shield
					spikesEffect = SUBEFFECT_ICE_SPIKES;
					damage = 20;
					chance = 25;
					break;
				case 12357: // ice shield +1
					spikesEffect = SUBEFFECT_ICE_SPIKES;
					damage = 25;
					chance = 30;
					break;
				}
				if (spikesEffect && HandleSpikesEquip(PAttacker, PDefender, Action, damage, spikesEffect, chance)){
					return true;
				}
			}
			// BODY
			// deal with spikesEffect effect gear
			PItem = PCharDef->getEquip(SLOT_BODY);
			if (PItem){
				spikesEffect = (SUBEFFECT)0;
				switch (PItem->getID())
				{
				case 13782: // ninja chainmail
					spikesEffect = SUBEFFECT_BLAZE_SPIKES;
					damage = 25;
					chance = 25;
					break;
				case 14485: // ninja chainmail +1
					spikesEffect = SUBEFFECT_SHOCK_SPIKES;
					damage = 25;
					chance = 25;
					break;
				case 14376: // rasetsu samue
					spikesEffect = SUBEFFECT_SHOCK_SPIKES;
					damage = 15;
					chance = 25;
					break;
				case 14377: // rasetsu samue +1
					spikesEffect = SUBEFFECT_SHOCK_SPIKES;
					damage = 20;
					chance = 30;
					break;
				case 13705: //ogre jerkin
					spikesEffect = SUBEFFECT_ICE_SPIKES;
					damage = 20;
					chance = 25;
					break;
				case 14366: //ogre jerkin +1
					spikesEffect = SUBEFFECT_ICE_SPIKES;
					damage = 25;
					chance = 30;
					break;
				case 12597: // war aketon
					spikesEffect = SUBEFFECT_BLAZE_SPIKES;
					damage = 25;
					chance = 25;
					break;
				case 13771: // war aketon +1
					spikesEffect = SUBEFFECT_BLAZE_SPIKES;
					damage = 30;
					chance = 30;
					break;
				case 14420: // igqira_weskit
					spikesEffect = SUBEFFECT_CURSE_SPIKES;
					damage = 0;
					chance = 30;
					break;
				case 14421: // genie_weskit
					spikesEffect = SUBEFFECT_CURSE_SPIKES;
					damage = 0;
					chance = 35;
					break;
				}
				if (spikesEffect && HandleSpikesEquip(PAttacker, PDefender, Action, damage, spikesEffect, chance)){
					//body activated return
					return true;
				}
			}
			// LEGS
			PItem = PCharDef->getEquip(SLOT_LEGS);
			if (PItem){
				spikesEffect = (SUBEFFECT)0;
				switch (PItem->getID())
				{
				case 12880: // ogre trousers
					spikesEffect = SUBEFFECT_ICE_SPIKES;
					damage = 15;
					chance = 20;
					break;
				case 14279: // ogre trousers +1
					spikesEffect = SUBEFFECT_ICE_SPIKES;
					damage = 20;
					chance = 25;
					break;
				case 14299: // rasetsu hakama
					spikesEffect = SUBEFFECT_ICE_SPIKES;
					damage = 15;
					chance = 20;
					break;
				case 14300: // rasetsu hakama +1
					spikesEffect = SUBEFFECT_ICE_SPIKES;
					damage = 20;
					chance = 25;
					break;
				case 12853: // war brais
					spikesEffect = SUBEFFECT_BLAZE_SPIKES;
					damage = 20;
					chance = 20;
					break;
				case 14238: // war brais +1
					spikesEffect = SUBEFFECT_BLAZE_SPIKES;
					damage = 25;
					chance = 25;
					break;
				case 14321: // igqira_lappas
					spikesEffect = SUBEFFECT_CURSE_SPIKES;
					damage = 0;
					chance = 25;
					break;
				case 14322: // genie_lappas
					spikesEffect = SUBEFFECT_CURSE_SPIKES;
					damage = 0;
					chance = 30;
					break;
				}
				if (spikesEffect && HandleSpikesEquip(PAttacker, PDefender, Action, damage, spikesEffect, chance)){
					return true;
				}
			}
			// HEAD
			PItem = PCharDef->getEquip(SLOT_HEAD);
			if (PItem){
				spikesEffect = (SUBEFFECT)0;
				switch (PItem->getID())
				{
				case 16125: // breeder mask
					spikesEffect = SUBEFFECT_BLAZE_SPIKES;
					damage = 20;
					chance = 15;
					break;
				case 13925: // rasetsu_jinpachi
					spikesEffect = SUBEFFECT_BLAZE_SPIKES;
					damage = 20;
					chance = 15;
					break;
				case 13926: // rasetsu_jinpachi +1
					spikesEffect = SUBEFFECT_BLAZE_SPIKES;
					damage = 25;
					chance = 20;
					break;
				case 13920: // wyvern helm
					spikesEffect = SUBEFFECT_BLAZE_SPIKES;
					damage = 20;
					chance = 15;
					break;
				case 13921: //wyvern helm +1
					spikesEffect = SUBEFFECT_BLAZE_SPIKES;
					damage = 25;
					chance = 20;
					break;
				case 15159: //igqira_tiara
					spikesEffect = SUBEFFECT_CURSE_SPIKES;
					damage = 0;
					chance = 20;
					break;
				case 15160: //genie_tiara
					spikesEffect = SUBEFFECT_CURSE_SPIKES;
					damage = 0;
					chance = 25;
					break;
				}
				if (spikesEffect && HandleSpikesEquip(PAttacker, PDefender, Action, damage, spikesEffect, chance)){
					return true;
				}
			}
			// HANDS
			PItem = PCharDef->getEquip(SLOT_HANDS);
			if (PItem){
				spikesEffect = (SUBEFFECT)0;
				switch (PItem->getID())
				{
				case 15042: // gothic gauntlets
				case 14952: // ice gauntlets
					spikesEffect = SUBEFFECT_ICE_SPIKES;
					damage = 15;
					chance = 15;
					break;
				case 14852: // igqira_manillas
					spikesEffect = SUBEFFECT_CURSE_SPIKES;
					damage = 0;
					chance = 20;
					break;
				case 14853: // genie_manillas
					spikesEffect = SUBEFFECT_CURSE_SPIKES;
					damage = 0;
					chance = 25;
					break;
				}
				if (spikesEffect && HandleSpikesEquip(PAttacker, PDefender, Action, damage, spikesEffect, chance)){
					return true;
				}
			}
			// FEET
			PItem = PCharDef->getEquip(SLOT_FEET);
			if (PItem){
				spikesEffect = (SUBEFFECT)0;
				switch (PItem->getID())
				{
				case 11402: // gothic sabatons
					spikesEffect = SUBEFFECT_ICE_SPIKES;
					damage = 15;
					chance = 15;
					break;
				case 12946: // suzakus sun-ate
					spikesEffect = SUBEFFECT_BLAZE_SPIKES;
					damage = 20;
					chance = 20;
					break;
				case 11392: // koschei crackows
					spikesEffect = SUBEFFECT_CURSE_SPIKES;
					damage = 0;
					chance = 20;
					break;
				}
				if (spikesEffect && HandleSpikesEquip(PAttacker, PDefender, Action, damage, spikesEffect, chance)){
					return true;
				}
			}
		}
		return false;
	}
	bool HandleSpikesEquip(CBattleEntity* PAttacker, CBattleEntity* PDefender, apAction_t* Action, uint8 damage, SUBEFFECT spikesType, uint8 chance)
	{
		int lvlDiff = dsp_cap((PDefender->GetMLevel() - PAttacker->GetMLevel()), -5, 5) * 2;
		if (WELL512::irand() % 100 <= chance + lvlDiff){
			// spikes landed
			if (spikesType == SUBEFFECT_CURSE_SPIKES){
				Action->spikesMessage = 0; // log says nothing?
				Action->spikesParam = EFFECT_CURSE;
			}
			else {
				uint8 ratio = (float)damage / 4;
				Action->spikesParam = HandleStoneskin(PAttacker, damage - WELL512::irand() % ratio + WELL512::irand() % ratio);
				PAttacker->addHP(-Action->spikesParam);
			}
			return true;
		}
		return false;
	}
	void HandleSpikesStatusEffect(CBattleEntity* PAttacker, apAction_t* Action)
	{
		int lvlDiff = 0;
		if (Action->ActionTarget){
			lvlDiff = dsp_cap((Action->ActionTarget->GetMLevel() - PAttacker->GetMLevel()), -5, 5) * 2;
		}
		switch (Action->spikesEffect)
		{
		case SUBEFFECT_CURSE_SPIKES:
			if (PAttacker->StatusEffectContainer->HasStatusEffect(EFFECT_CURSE) == false){
				PAttacker->StatusEffectContainer->AddStatusEffect(new CStatusEffect(EFFECT_CURSE, EFFECT_CURSE, 15, 0, 180));
			}
			break;
		case SUBEFFECT_ICE_SPIKES:
		{
			if (WELL512::irand() % 100 <= 20 + lvlDiff && PAttacker->StatusEffectContainer->HasStatusEffect(EFFECT_PARALYSIS) == false){
				PAttacker->StatusEffectContainer->AddStatusEffect(new CStatusEffect(EFFECT_PARALYSIS, EFFECT_PARALYSIS, 20, 0, 30));
			}
			break;
		}
		case SUBEFFECT_SHOCK_SPIKES:
		{
			if (WELL512::irand() % 100 <= 30 + lvlDiff && PAttacker->StatusEffectContainer->HasStatusEffect(EFFECT_STUN) == false){
				PAttacker->StatusEffectContainer->AddStatusEffect(new CStatusEffect(EFFECT_STUN, EFFECT_STUN, 1, 0, 3));
			}
			break;
		}
		default:
			break;
		}
	}
	/************************************************************************
	* *
	* Handles Enspell effect and damage *
	* *
	************************************************************************/
	void HandleEnspell(CBattleEntity* PAttacker, CBattleEntity* PDefender, apAction_t* Action, uint8 hitNumber, CItemWeapon* weapon, uint32 finaldamage)
	{
		CCharEntity* PChar = NULL;
		if (PAttacker->objtype == TYPE_PC)
		{
			PChar = (CCharEntity*)PAttacker;
		}
		EFFECT daze = EFFECT_NONE;
		uint16 power = 0;
		if (PAttacker->StatusEffectContainer->HasStatusEffect(EFFECT_DRAIN_SAMBA))
		{
			daze = EFFECT_DRAIN_DAZE;
			power = PAttacker->StatusEffectContainer->GetStatusEffect(EFFECT_DRAIN_SAMBA)->GetPower();
		}
		else if (PAttacker->StatusEffectContainer->HasStatusEffect(EFFECT_ASPIR_SAMBA))
		{
			daze = EFFECT_ASPIR_DAZE;
			power = PAttacker->StatusEffectContainer->GetStatusEffect(EFFECT_ASPIR_SAMBA)->GetPower();
		}
		else if (PAttacker->StatusEffectContainer->HasStatusEffect(EFFECT_HASTE_SAMBA))
		{
			daze = EFFECT_HASTE_DAZE;
			power = PAttacker->StatusEffectContainer->GetStatusEffect(EFFECT_HASTE_SAMBA)->GetPower();
		}
		if (daze != EFFECT_NONE)
		{
			if (PAttacker->PParty != NULL)
			{
				for (uint8 i = 0; i < PAttacker->PParty->members.size(); i++)
				{
					PDefender->StatusEffectContainer->DelStatusEffect(EFFECT_DRAIN_DAZE, PAttacker->PParty->members[i]->id);
					PDefender->StatusEffectContainer->DelStatusEffect(EFFECT_HASTE_DAZE, PAttacker->PParty->members[i]->id);
					PDefender->StatusEffectContainer->DelStatusEffect(EFFECT_ASPIR_DAZE, PAttacker->PParty->members[i]->id);
				}
			}
			else
			{
				PDefender->StatusEffectContainer->DelStatusEffect(EFFECT_DRAIN_DAZE, PAttacker->id);
				PDefender->StatusEffectContainer->DelStatusEffect(EFFECT_HASTE_DAZE, PAttacker->id);
				PDefender->StatusEffectContainer->DelStatusEffect(EFFECT_ASPIR_DAZE, PAttacker->id);
			}
			if ((PDefender->m_EcoSystem != SYSTEM_UNDEAD) || (daze == EFFECT_HASTE_DAZE))
			{
				PDefender->StatusEffectContainer->AddStatusEffect(new CStatusEffect(daze,
					0, power,
					0, 10, PAttacker->id), true);
			}
		}
		// Enspell overwrites weapon effects
		if (PAttacker->getMod(MOD_ENSPELL) > 0)
		{
			SUBEFFECT subeffects[8] = { SUBEFFECT_LIGHT_DAMAGE, SUBEFFECT_DARKNESS_DAMAGE, SUBEFFECT_FIRE_DAMAGE, SUBEFFECT_EARTH_DAMAGE,
				SUBEFFECT_WATER_DAMAGE, SUBEFFECT_WIND_DAMAGE, SUBEFFECT_ICE_DAMAGE, SUBEFFECT_LIGHTNING_DAMAGE };
			int16 enspell = PAttacker->getMod(MOD_ENSPELL);
			if (enspell > 0 && enspell <= 6)
			{
				Action->additionalEffect = subeffects[enspell + 1];
				Action->addEffectMessage = 163;
				Action->addEffectParam = CalculateEnspellDamage(PAttacker, PDefender, 1, enspell - 1);
				if (Action->addEffectParam < 0)
				{
					Action->addEffectParam = -Action->addEffectParam;
					Action->addEffectMessage = 384;
				}
				PDefender->addHP(-Action->addEffectParam);
			}
			else if ((enspell > 6 && enspell <= 8) || (enspell > 8 && enspell <= 14 && hitNumber == 0))
			{
				Action->additionalEffect = subeffects[enspell - 7];
				Action->addEffectMessage = 163;
				Action->addEffectParam = CalculateEnspellDamage(PAttacker, PDefender, 2, enspell > 8 ? enspell - 9 : enspell - 1);
				if (Action->addEffectParam < 0)
				{
					Action->addEffectParam = -Action->addEffectParam;
					Action->addEffectMessage = 384;
				}
				PDefender->addHP(-Action->addEffectParam);
			}
			else if (enspell == ENSPELL_BLOOD_WEAPON)
			{
				Action->additionalEffect = SUBEFFECT_HP_DRAIN;
				Action->addEffectMessage = 161;
				Action->addEffectParam = PAttacker->addHP(Action->param);
				if (PChar != NULL){
					charutils::UpdateHealth(PChar);
				}
			}
			else if (enspell == ENSPELL_AUSPICE && hitNumber == 0){
				Action->additionalEffect = SUBEFFECT_LIGHT_DAMAGE;
				Action->addEffectMessage = 163;
				Action->addEffectParam = CalculateEnspellDamage(PAttacker, PDefender, 2, 7);
				if (Action->addEffectParam < 0)
				{
					Action->addEffectParam = -Action->addEffectParam;
					Action->addEffectMessage = 384;
				}
				PDefender->addHP(-Action->addEffectParam);
			}
		}
		//check weapon for additional effects
		else if (PAttacker->objtype == TYPE_PC && weapon->getModifier(MOD_ADDITIONAL_EFFECT) > 0)
		{
			luautils::OnAdditionalEffect(PAttacker, PDefender, weapon, Action, finaldamage);
			if (Action->addEffectMessage == 163 && Action->addEffectParam < 0)
			{
				Action->addEffectMessage = 384;
			}
		}
		else if (PAttacker->objtype == TYPE_MOB && ((CMobEntity*)PAttacker)->getMobMod(MOBMOD_ADD_EFFECT) > 0)
		{
			luautils::OnAdditionalEffect(PAttacker, PDefender, weapon, Action, finaldamage);
		}
		else
		{
			// Generic drain for anyone able to do melee damage to a dazed target
			// TODO: ignore dazes from dancers outside party
			int16 delay = PAttacker->GetWeaponDelay(false) / 10;
			if (PAttacker->PMaster == NULL)
			{
				EFFECT daze = EFFECT_NONE;
				uint16 power = 0;
				if (PAttacker->PParty != NULL && PAttacker->objtype == TYPE_PC)
				{
					for (uint8 i = 0; i < PAttacker->PParty->members.size(); i++)
					{
						if (PDefender->StatusEffectContainer->HasStatusEffect(EFFECT_DRAIN_DAZE, PAttacker->PParty->members[i]->id))
						{
							daze = EFFECT_DRAIN_DAZE;
							power = PDefender->StatusEffectContainer->GetStatusEffect(EFFECT_DRAIN_DAZE, PAttacker->PParty->members[i]->id)->GetPower();
							break;
						}
						if (PDefender->StatusEffectContainer->HasStatusEffect(EFFECT_HASTE_DAZE, PAttacker->PParty->members[i]->id))
						{
							daze = EFFECT_HASTE_DAZE;
							power = PDefender->StatusEffectContainer->GetStatusEffect(EFFECT_HASTE_DAZE, PAttacker->PParty->members[i]->id)->GetPower();
							break;
						}
						if (PDefender->StatusEffectContainer->HasStatusEffect(EFFECT_ASPIR_DAZE, PAttacker->PParty->members[i]->id))
						{
							daze = EFFECT_ASPIR_DAZE;
							power = PDefender->StatusEffectContainer->GetStatusEffect(EFFECT_ASPIR_DAZE, PAttacker->PParty->members[i]->id)->GetPower();
							break;
						}
					}
				}
				else
				{
					if (PDefender->StatusEffectContainer->HasStatusEffect(EFFECT_DRAIN_DAZE, PAttacker->id))
					{
						daze = EFFECT_DRAIN_DAZE;
						power = PDefender->StatusEffectContainer->GetStatusEffect(EFFECT_DRAIN_DAZE, PAttacker->id)->GetPower();
					}
					if (PDefender->StatusEffectContainer->HasStatusEffect(EFFECT_HASTE_DAZE, PAttacker->id))
					{
						daze = EFFECT_HASTE_DAZE;
						power = PDefender->StatusEffectContainer->GetStatusEffect(EFFECT_HASTE_DAZE, PAttacker->id)->GetPower();
					}
					if (PDefender->StatusEffectContainer->HasStatusEffect(EFFECT_ASPIR_DAZE, PAttacker->id))
					{
						daze = EFFECT_ASPIR_DAZE;
						power = PDefender->StatusEffectContainer->GetStatusEffect(EFFECT_ASPIR_DAZE, PAttacker->id)->GetPower();
					}
				}
				if (daze == EFFECT_DRAIN_DAZE)
				{
					uint16 multiplier = 3 + (5.5f * power - 1);
					int8 Samba = WELL512::irand() % ((delay * multiplier) / 100) + 1;
					// vary damage based on lvl diff
					int8 lvlDiff = (PDefender->GetMLevel() - PAttacker->GetMLevel()) / 2;
					if (lvlDiff < -5){
						lvlDiff = -5;
					}
					Samba -= lvlDiff;
					if (Samba > (finaldamage / 2)){
						Samba = finaldamage / 2;
					}
					if (finaldamage <= 2){
						Samba = 0;
					}
					if (Samba < 0)
					{
						Samba = 0;
					}
					Action->additionalEffect = SUBEFFECT_HP_DRAIN;
					Action->addEffectMessage = 161;
					Action->addEffectParam = Samba;
					PAttacker->addHP(Samba);	// does not do any additional drain to targets HP, only a portion of it
					if (PChar != NULL){
						charutils::UpdateHealth(PChar);
					}
				}
				else if (daze == EFFECT_ASPIR_DAZE)
				{
					uint16 multiplier = 1 + (2 * power - 1);
					int8 Samba = WELL512::irand() % ((delay * multiplier) / 100) + 1;
					if (Samba >= finaldamage / 4) { Samba = finaldamage / 4; }
					if (finaldamage <= 2) { Samba = 0; }
					if (Samba < 0) { Samba = 0; }
					Action->additionalEffect = SUBEFFECT_HP_DRAIN;
					Action->addEffectMessage = 162;
					int16 mpDrained = PDefender->addMP(-Samba);
					PAttacker->addMP(mpDrained);
					Action->addEffectParam = mpDrained;
					if (PChar != NULL){
						charutils::UpdateHealth(PChar);
					}
				}
				else if (daze == EFFECT_HASTE_DAZE)
				{
					Action->additionalEffect = SUBEFFECT_HASTE;
					// Ability haste added in scripts\globals\effects\haste_samba_haste_effect.lua
					PAttacker->StatusEffectContainer->AddStatusEffect(new CStatusEffect(EFFECT_HASTE_SAMBA_HASTE, 0, power, 0, 10));
					// Status effect removed in CAttackRound constructor (i.e. after next attack round is calculated)
				}
			}
		}
	}
/************************************************************************
* *
*				 Handles Weapon's additional effects
* *
************************************************************************/
// TODO: remove function, move additional effects into items script files (deleting from switch as they get done)

void HandleAdditionalEffect(CCharEntity* PAttacker, CBattleEntity* PDefender, apAction_t* Action, uint32 finaldamage)
{		
CItemWeapon* PWeapon = (CItemWeapon*)PAttacker->getStorage(LOC_INVENTORY)->GetItem(PAttacker->equip[SLOT_MAIN]); 
CItemWeapon* PAmmo = (CItemWeapon*)PAttacker->getStorage(LOC_INVENTORY)->GetItem(PAttacker->equip[SLOT_AMMO]);
		int ammoUse = 0;
		int tizonaChance = 0;
		int tizonaPercent = 0;

		if (Action->animation == 1)
			PWeapon = (CItemWeapon*)PAttacker->getStorage(LOC_INVENTORY)->GetItem(PAttacker->equip[SLOT_SUB]);
		if(PWeapon != NULL)
		{
		EFFECT dispelled;
		uint8 chance = 0;
		uint8 effect = 0;
		switch (PWeapon->getID())
		{
			// Additional Effect vs. Vermin: HP Drain																						
		case 18428:   // Kogitsunemaru																						
			if (PDefender->m_EcoSystem == SYSTEM_VERMIN)
			{
				effect = 1;
			}
			return;
			//Additional Effect: HP Drain Weapons																						
		case 16827:   // Carnage Blade																						
		case 16528:   // Bloody Rapier																						
		case 16824:   // Carnage Rapier																						
		case 17651:   // Dainslaif																						
		case 16556:   // Bloody Blade																						
		case 16609:   // Bloody Sword																						
		case 16580:   // Bloodsword																						
		case 17646:   // Carnage Sword																						
		case 16777:   // Death Scythe																						
		case 16791:   // Death Scythe +1																						
		case 16846:   // Bloody Lance																						
		case 16881:   // Carnage Lance																						
		case 17561:   // Revenging Staff																						
		case 17562:   // Revenging Staff +1																						
		case 17778:   // Muketsu																						
		case 17779:   // Muketsu +1																						
		case 17576:   // Grim Staff																						
		case 17510:   // Vampiric Claws																						
		case 18810:   // Cadushi Grip																						
		case 16582:   // Vetala Sword																						
		case 18859:   // Kerykeion																						
		case 18785:   // Grotesque Cesti																						
		case 18908:   // Dhampyr Sword																						
		case 18856:   // Deae Gratia																						
		{
			effect = 1;
		}
			break;

			//Additional Effect: MP Recover																						
		case 18324:   // Mjollnir Lvl 75																						
		case 18325:   // Mjollnir Lvl 80																						
		case 18647:   // Mjollnir Lvl 85																						
		case 18661:   // Mjollnir Lvl 90																						
		case 18675:   // Mjollnir Lvl 95																						
		case 19756:   // Mjollnir Lvl 99																						
		case 19849:   // Mjollnir Lvl 99 - 2																						
		{
			effect = 2;
		}
			break;

			//Additional Effect: TP drain Weapons																						
		case 18904:   // Ephemeron																						
		case 19795:   // Ereptor Lance																						
		case 19160:   // Estramacon																						
		case 18437:   // Namikirimaru																						
		case 17823:   // Shinsoku																						
		{
			effect = 3;
		}
			break;

			//Additional Effect: MP drain Weapons																						
		case 17574:   // Archalaus's Pole																						
		case 16509:   // Aspir Knife																						
		case 18948:   // Enforcer																						
		case 17430:   // Fey Wand																						
		case 18047:   // Havoc Scythe																						
		case 17718:   // Leech Scimitar																						
		case 18941:   // Naigama																						
		case 18942:   // Naigama +1																						
		case 18599:   // Pestle																						
		{
			chance = 10;
			effect = 4;
		}
			break;
		case 17072:   // Lilith's Rod																						
		{
			chance = 20;
			chance += rand() % 11;
			effect = 4;
		}
			break;

			//Additional Effect: HP or MP drain Weapons																						
		case 17745:   // Hofud																						
		{
			effect = 5;
		}
			break;

			//Additional Effect: HP, MP, or TP drain Weapons																			
		case 19132:   // Twilight Knife																						
		{
			if (rand() % 100 <= 30)
			{
				int randomEffect = rand() % 3;
				if (randomEffect == 0)
				{
					effect = 1;
				}
				else if (randomEffect == 1)
				{
					chance = 100;
					effect = 4;
				}
				else
				{
					effect = 3;
				}
			}
		}
			break;
			// Additional Effect vs. Vermin: Stun																						
		case 18040:   // Webcutter																						
			if (PDefender->m_EcoSystem == SYSTEM_VERMIN)
			{
				effect = 6;
			}
			break;
			//Additional Effect: Stun Weapons (5% chance guestimate)																						
		case 19177:   // Etourdissante																						
		case 19178:   // Etourdissante +1																						
		case 16910:   // Mamushito																						
		case 17770:   // Mamushito +1																						
		case 19019:   // Platinum Grip																						
		case 19020:   // Platinum Grip +1																						
		case 17464:   // Purgatory Mace																						
		case 16431:   // Stun Claws																						
		case 17486:   // Stun Claws +1																						
		case 16432:   // Stun Jamadhars																						
		case 17484:   // Stun Jamadhars +1																						
		case 16503:   // Stun Knife																						
		case 17600:   // Stun Knife +1																						
		case 16506:   // Stun Kukri																						
		case 17614:   // Stun Kukri +1																						
		{
			effect = 6;
		}
			break;

			// Additional Effect vs. Tigers: Paralysis																						
		case 16672:   // Tigerhunter																						
			if (((CMobEntity*)PDefender)->m_Family == 242)
			{
				effect = 7;
			}
			break;
			//Additional Effect: Paralysis Weapons (5% chance guestimate)																						
		case 17737:   // Corsair's Scimitar																						
		case 18762:   // Custodes																						
		case 17648:   // Dragvandil																						
		case 18211:   // Gawain's Axe																						
		case 18312:   // Kikoku Lvl 75																						
		case 18313:   // Kikoku Lvl 80																						
		case 18645:   // Kikoku Lvl 85																						
		case 18659:   // Kikoku Lvl 90																						
		case 18673:   // Kikoku Lvl 95																						
		case 19754:   // Kikoku Lvl 99																						
		case 19847:   // Kikoku Lvl 99 - 2																						
		case 19796:   // Rosschinder																						
		case 19797:   // Rosschinder +1																						
		case 17492:   // Shiva's Claws																						
		{
			effect = 7;
		}
			break;

			// Additional Effect: Petrify (5% chance guestimate)																						
		case 18454:   // Ishikirimaru																						
		{
			effect = 8;
		}
			break;

			// Additional Effect: Terror (5% chance guestimate)																						
		case 19163:   // Nightfall																						
		{
			effect = 9;
		}
			break;

			// Additional Effect: Addle (5% chance guestimate)																						
		case 19316:   // Fetter Lance																						
		{
			effect = 10;
		}
			break;

			// Additional Effect: Choke (5% chance guestimate)																						
		case 18288:   // Guttler Lvl 75																						
		case 18289:   // Guttler Lvl 80																						
		case 18641:   // Guttler Lvl 85																						
		case 18655:   // Guttler Lvl 90																						
		case 18669:   // Guttler Lvl 95																						
		case 19750:   // Guttler Lvl 99																						
		case 19843:   // Guttler Lvl 99 - 2																						
		{
			effect = 11;
		}
			break;

			// Additional Effect vs. Plantoids: Weight																						
		case 16785:   // Harvester																						
			if (PDefender->m_EcoSystem == SYSTEM_PLANTOID)
			{
				effect = 12;
			}
			break;
			// Additional Effect: Weight (5% chance guestimate)																						
		case 18096:   // Heavy Lance																						
		case 18026:   // Magnet Knife																						
		case 19140:   // Mantodea Harpe																						
		{
			effect = 12;
		}
			break;

			// Additional Effect: Amnesia (5% chance guestimate)																						
		case 18510:   // Vermeil Bhuj																						
		{
			effect = 13;
		}
			break;

			// Additional Effect vs. Plantoids: Bind																						
		case 16783:   // Plantreaper																						
			if (PDefender->m_EcoSystem == SYSTEM_PLANTOID)
			{
				effect = 14;
			}
			break;
			// Additional Effect vs. Birds: Bind																						
		case 18044:   // Scarecrow Scythe																						
			if (PDefender->m_EcoSystem == SYSTEM_BIRD)
			{
				effect = 14;
			}
			break;
			// Additional Effect vs. Arcana: Bind																						
		case 17058:   // Caduceus																						
			if (PDefender->m_EcoSystem == SYSTEM_ARCANA)
			{
				effect = 14;
			}
			break;
			// Additional Effect: Bind (5% chance guestimate)																						
		case 16897:   // Kageboshi																						
		{
			effect = 14;
		}
			break;

			// Additional Effect: Slow (5% chance guestimate)																						
		case 17083:   // Time Hammer																						
		{
			effect = 15;
		}
			break;

			// Additional Effect: Flash (5% chance guestimate)																						
		case 17069:   // Moepapa Mace																						
		{
			effect = 16;
		}
			break;

			// Additional Effect: Reduce Enemy TP (2% chance guestimate)																						
		case 19298:   // Ikarigiri																						
		{
			effect = 17;
		}
			break;

			// Additional Effect: STR Boost 10% ??																						
		case 17563:   // Power Staff																						
		{
			effect = 18;
		}
			break;

			// Additional Effect: Haste (7% chance guestimate)																						
		case 16504:   // Oynos Knife																						
		{
			effect = 19;
		}
			break;

			// Additional Effect: Blink (15% chance guestimate)																						
		case 18830:   // Gusterion																						
		{
			effect = 20;
		}
			break;

			//Additional Effect: Poison Weapons (5% chance guestimate)																						
		case 16907:   // Busuto																						
		case 16927:   // Busuto +1																						
		case 16525:   // Hornet Fleuret																						
		case 16909:   // Kororito																						
		case 17769:   // Kororito +1																						
		case 17937:   // Leopard Axe																						
		case 18270:   // Mandau Lvl 75																						
		case 18271:   // Mandau Lvl 80																						
		case 18638:   // Mandau Lvl 85																						
		case 18652:   // Mandau Lvl 90																						
		case 18666:   // Mandau Lvl 95																						
		case 19747:   // Mandau Lvl 99																						
		case 19840:   // Mandau Lvl 99 - 2																						
		case 17650:   // Nadrs																						
		case 16458:   // Poison Baselard																						
		case 16387:   // Poison Cesti																						
		case 16700:   // Poison Cesti +1																						
		case 16417:   // Poison Claws																						
		case 16439:   // Poison Claws +1																						
		case 16496:   // Poison Dagger																						
		case 16741:   // Poison Dagger +1																						
		case 16403:   // Poison Katars																						
		case 16693:   // Poison Katars +1																						
		case 16472:   // Poison Knife																						
		case 16742:   // Poison Knife +1																						
		case 16478:   // Poison Kukri																						
		case 16488:   // Poison Kukri +1																						
		case 16410:   // Poison Baghnakhs																						
		case 16692:   // Poison Baghnakhs +1																						
		case 16743:   // Python Baselard																						
		case 16507:   // Venom Baselard																						
		case 16510:   // Venom Baselard +1																						
		case 16418:   // Venom Claws																						
		case 16425:   // Venom Claws +1																						
		case 16404:   // Venom Katars																						
		case 17483:   // Venom Katars +1																						
		case 16502:   // Venom Knife																						
		case 16762:   // Venom Knife +1																						
		case 16499:   // Venom Kris																						
		case 16761:   // Venom Kris +1																						
		case 16505:   // Venom Kukri																						
		case 17604:   // Venom Kukri +1																						
		case 17959:   // Viperine Pick																						
		case 17634:   // Wasp Fleuret																						
		{
			effect = 21;
		}
			break;

			// Additional Effect: Poisone, Paralysis, or Bind (5% chance guestimate)																						
		case 18784:   // Metasoma Katars																						
			if (rand() % 100 <= 5)
			{
				int metasomaEffect = rand() % 3;
				if (metasomaEffect == 0)
				{
					effect = 21;
				}
				else if (metasomaEffect == 1)
				{
					effect = 7;
				}
				else
				{
					effect = 14;
				}
			}
			break;

			// Additional Effect: Blind (5% chance guestimate)																						
		case 18306:   // Apocalypse Lvl 75																						
		case 18307:   // Apocalypse Lvl 80																						
		case 18644:   // Apocalypse Lvl 85																						
		case 18658:   // Apocalypse Lvl 90																						
		case 18672:   // Apocalypse Lvl 95																						
		case 19753:   // Apocalypse Lvl 99 - 1																						
		case 19846:   // Apocalypse Lvl 99 - 2																						
		case 16454:   // Blind Dagger																						
		case 16493:   // Blind Dagger +1																						
		case 16471:   // Blind Knife																						
		case 16490:   // Blind Knife +1																						
		case 16905:   // Bokuto																						
		case 16926:   // Bokuto +1																						
		{
			effect = 22;
		}
			break;

			// Additional Effect: Silence (5% chance guestimate)																						
		case 17627:   // Garuda's Dagger																						
		case 18355:   // Hushed Baghnakhs																						
		case 18008:   // Hushed Dagger																						
		case 16906:   // Mokuto																						
		case 16925:   // Mokuto +1																						
		case 17797:   // Seito																						
		case 16495:   // Silence Dagger																						
		case 16508:   // Silence Dagger +1																						
		case 16429:   // Silence Baghnakhs																						
		case 16438:   // Silence Baghnakhs +1																						
		{
			effect = 23;
		}
			break;

			// Additional Effect: Curse (5% chance guestimate)																						
		case 16533:   // Ancient Sword																						
		case 17116:   // Misery Staff																						
		case 18813:   // Ultio Grip																						
		{
			effect = 24;
		}
			break;

			// Additional Effect: Death (2% chance guestimate)																						
		case 18916:   // Heimdall's Doom																						
		case 18551:   // Twilight Scythe																						
		{
			effect = 25;
		}
			break;

			// Additional Effect: Sleep (5% chance guestimate)																						
		case 17117:   // Hypno Staff																						
		case 16497:   // Sleep Dagger																						
		case 17575:   // Somnus Signa																						
		{
			effect = 26;
		}
			break;


			// Additional Effect: Impairs Evasion (5% chance guestimate)																						
		case 18032:   // Adder Jambiya																						
		case 18033:   // Adder Jambiya +1																						
		case 17695:   // Bayard's Sword																						
		case 18294:   // Bravura Lvl 75																						
		case 18295:   // Bravura Lvl 80																						
		case 18642:   // Bravura Lvl 85																						
		case 18656:   // Bravura Lvl 90																						
		case 18670:   // Bravura Lvl 95																						
		case 19751:   // Bravura Lvl 99																						
		case 19844:   // Bravura Lvl 99 - 2																						
		case 16773:   // Cruel Scythe																						
		case 16863:   // Cruel Spear																						
		case 18118:   // Dark Mezraq																						
		case 18119:   // Dark Mezraq +1																						
		case 16974:   // Dotanuki																						
		case 18110:   // Mezraq																						
		case 18111:   // Mezraq +1																						
		case 18123:   // Thalassocrat																						
		case 18124:   // Thalassocrat +1																						
		{
			effect = 27;
		}
			break;

			// Additional Effect vs. Dolls: Weakens Defense																						
		case 16674:   // Emeth Pick																						
			if ((((CMobEntity*)PDefender)->m_Family == 83) || ((CMobEntity*)PDefender)->m_Family == 84
				|| ((CMobEntity*)PDefender)->m_Family == 85)
			{
				effect = 28;
			}
			break;

			// Additional Effect vs. Arcana Weakens Defense																						
		case 17416:   // Arcana Breaker																						
			if (PDefender->m_EcoSystem == SYSTEM_ARCANA)
			{
				effect = 28;
			}
			break;

			// Additional Effect Weakens Defense																						
		case 16430:	  // Acid Claws																					
		case 16459:   // Acid Baselard																						
		case 17605:   // Acid Dagger																						
		case 16501:   // Acid Knife																						
		case 16479:   // Acid Kukri																						
		case 18794:	  // Blutklauen																					
		case 18793:   // Blutkrallen																						
		case 17487:	  // Corrosive Claws																					
		case 17607:   // Corrosive Baselard																						
		case 17606:   // Corrosive Dagger																						
		case 17608:   // Corrosive Knife																						
		case 16494:   // Corrosive Kukri																						
		case 17694:   // Guespiere																						
		case 18300:   // Gungnir Lvl 75																						
		case 18301:   // Gungnir Lvl 80																						
		case 18643:   // Gungnir Lvl 85																						
		case 18657:   // Gungnir Lvl 90																						
		case 18671:   // Gungnir Lvl 95																						
		case 19752:   // Gungnir Lvl 99																						
		case 19845:   // Gungnir Lvl 99 - 2																						
		case 18357:	  // Melt Claws																					
		case 18012:   // Melt Baselard																						
		case 18010:   // Melt Dagger																						
		case 18011:   // Melt Knife																						
		case 18013:   // Melt Kukri																						
		case 18410:   // Melt Katana																						
		case 20639:   // Oxidant Baselard																						
		case 20640:   // Nitric Baselard																						
		case 21009:   // Nakajimarai																						
		case 21010:   // Nakajimarai +1																						
		case 17106:   // Rockmasher																						
		case 16908:   // Yoto																						
		case 17768:   // Yoto +1																						
		{
			effect = 28;
		}
			break;

			// Additional Effect: Weakens Attacks 5% ??																						
		case 18318:   // Amanomurakumo Lvl 75																						
		case 18319:   // Amanomurakumo Lvl 80																						
		case 18646:   // Amanomurakumo Lvl 85																						
		case 18660:   // Amanomurakumo Lvl 90																						
		case 18674:   // Amanomurakumo Lvl 95																						
		case 19775:   // Amanomurakumo Lvl 99																						
		case 19848:   // Amanomurakumo Lvl 99 - 2																						
		case 18088:   // Dreizack																						
		{
			effect = 29;
		}
			break;

			// Additional Effect vs. Beasts: Darkness Damage																						
		case 18041:   // A L'Outrance																						
			if (PDefender->m_EcoSystem == SYSTEM_BEAST)
			{
				chance = 100;
				effect = 30;
			}
			break;
			// Additional Effect: Darkness Damage (5% chance guestimate)																						
		case 18961:   // Plaga Scythe																						
		case 16515:   // Colichemarde																						
		case 17632:   // Colichemarde +1																						
		case 16787:   // Demon Slicer																						
		case 17559:   // Dark Staff																						
		case 17560:   // Pluto's Staff																						
		case 16936:   // Demonic Sword																						
		{
			chance = 5;
			effect = 30;
		}
			break;

			// Additional Effect: Earth Damage (5% chance guestimate)																						
		case 17552:   // Terra Staff																						
		case 18501:   // Seismic Axe																						
		case 17807:   // Gold Musketeers Uchigatana																						
		case 19114:   // Galkan Dagger																						
		case 17076:   // Earth Wand																						
		case 17551:   // Earth Staff																						
		case 17438:   // Titan's Cudgel																						
		{
			effect = 31;
		}
			break;

			// Additional Effect vs. Undead: Fire Damage																						
		case 18042:   // Ascention																						
			if (PDefender->m_EcoSystem == SYSTEM_UNDEAD)
			{
				effect = 32;
			}
			break;
			// Additional Effect vs. Plantoids: Fire Damage																						
		case 16720:   // Plantbane																						
			if (PDefender->m_EcoSystem == SYSTEM_PLANTOID)
			{
				effect = 32;
			}
			break;
			// Additional Effect: Fire Damage (5% chance guestimate)																						
		case 18385:   // Agol																						
		case 19151:   // Bahdur																						
		case 19152:   // Bahdur +1																						
		case 19296:   // Ban																						
		case 16398:   // Burning Cesti																						
		case 16929:   // Burning Claymore																						
		case 16416:   // Dragon Claws																						
		case 17485:   // Dragon Claws +1																						
		case 17545:   // Fire Staff																						
		case 16543:   // Fire Sword																						
		case 16564:   // Flame Blade																						
		case 16804:   // Flame Blade +1																						
		case 16588:   // Flame Claymore																						
		case 16522:   // Flame Degen																						
		case 16823:   // Flame Degen +1																						
		case 16621:	  // Flame Sword																					
		case 18625:   // Gerra's Staff																						
		case 16713:   // Hellfire Axe																						
		case 16928:   // Hellfire Sword																						
		case 16973:   // Homura																						
		case 16986:   // Homura +1																						
		case 17665:   // Ifrit's Blade																						
		case 16709:   // Inferno Axe																						
		case 16594:   // Inferno Sword																						
		case 17828:   // Koen																						
		case 18220:   // Prominence Axe																						
		case 18381:   // Prominence Sword																						
		case 18043:   // Suzaku's Scythe																						
		case 17706:   // Vulcan Blade																						
		case 18379:   // Vulcan Claymore																						
		case 17705:   // Vulcan Degen																						
		case 17704:   // Vulcan Sword																						
		case 17546:   // Vulcan's Staff																						
		{
			effect = 32;
		}
			break;

			// Additional Effect vs. Slimes: Light Damage																						
		case 17981:   // Ban Del Sol																						
			if ((((CMobEntity*)PDefender)->m_Family == 228) || ((CMobEntity*)PDefender)->m_Family == 229
				|| ((CMobEntity*)PDefender)->m_Family == 230)
			{
				effect = 33;
			}
			break;
			// Additional Effect vs. Orcs: Light Damage																						
		case 16867:   // Orc Piercer																						
			if ((((CMobEntity*)PDefender)->m_Family == 189) || ((CMobEntity*)PDefender)->m_Family == 190
				|| ((CMobEntity*)PDefender)->m_Family == 248 || ((CMobEntity*)PDefender)->m_Family == 334)
			{
				effect = 33;
			}
			break;
			// Additional Effect vs. Gigas: Light Damage																						
		case 16956:   // Skofnung																						
			if (((((CMobEntity*)PDefender)->m_Family >= 126) && (((CMobEntity*)PDefender)->m_Family <= 130))
				|| ((CMobEntity*)PDefender)->m_Family == 328)
			{
				chance = 100;
				effect = 33;
			}
			break;
			// Additional Effect vs. Dragons: Light Damage																						
		case 16943:   // Ascalon																						
			if (PDefender->m_EcoSystem == SYSTEM_DRAGON)
			{
				chance = 100;
				effect = 33;
			}
			break;
			// Additional Effect Night Time: Light Damage																						
		case 17774:   // Shiranui																						
			if (CVanaTime::getInstance()->GetCurrentTOTD() == TIME_NIGHT)
			{
				chance = 20;
				effect = 33;
			}
			break;
			// Additional Effect: Light Damage (5% chance guestimate)																						
		case 17558:   // Apollo's Staff																						
		case 18882:   // Beaivi's Scepter																						
		case 16549:   // Divine Sword																						
		case 16826:   // Divine Sword +1																						
		case 16938:   // Glorious Sword																						
		case 16550:   // Hallowed Sword																						
		case 17738:   // Hauteclaire																						
		case 16523:   // Holy Degen																						
		case 16817:   // Holy Degen +1																						
		case 16860:   // Holy Lance																						
		case 16880:   // Holy Lance +1																						
		case 17041:   // Holy Mace																						
		case 17411:   // Holy Mace +1																						
		case 17080:   // Holy Maul																						
		case 17114:   // Holy Maul +1																						
		case 16581:   // Holy Sword																						
		case 16816:   // Holy Sword +1																						
		case 17085:   // Holy Wand																						
		case 17434:   // Holy Wand +1																						
		case 17557:   // Light Staff																						
		case 17414:   // Pixie Mace																						
		case 17683:   // Sacred Degen																						
		case 16858:   // Sacred Lance																						
		case 18391:   // Sacred Mace																						
		case 18392:   // Sacred Maul																						
		case 17682:   // Sacred Sword																						
		case 18393:   // Sacred Wand																						
		case 18853:   // Spirit Maul																						
		case 16613:   // Spirit Sword																						
		{
			chance = 5;
			effect = 33;
		}
			break;

			// Additional Effect vs. Undead: Water Damage																						
		case 16884:   // Narval																						
			if (PDefender->m_EcoSystem == SYSTEM_UNDEAD)
			{
				effect = 34;
			}
			break;
			// Additional Effect: Water Damage with Pump																						
		case 17512:   // Hydro Baghnakhs																						
		case 17513:   // Hydro Baghnakhs +1																						
		case 18200:   // Hydro Axe																						
		case 18201:   // Hydro Baghnakhs +1																						
		case 18204:   // Hydro Chopper																						
		case 18205:   // Hydro Chopper +1																						
		case 17514:   // Hydro Claws																						
		case 17515:   // Hydro Claws +1																						
		case 18202:   // Hydro Cutter																						
		case 18203:   // Hydro Cutter +1																						
		case 17516:   // Hydro Patas																						
		case 17517:   // Hydro Patas +1																						
			if (PAmmo->getID() == 18232 || PAmmo->getID() == 18233 || PAmmo->getID() == 18234)
			{
				chance = 10;
				chance += rand() % 6;
				ammoUse = 1;
				effect = 36;
			}
			break;

			// Additional Effect: Water Damage (5% chance guestimate)																						
		case 17750:   // Anthos Xiphos																						
		case 16945:   // Arondight																						
		case 18953:   // Beluga																						
		case 17623:   // Bmt. Knife																						
		case 16548:   // Coral Sword																						
		case 17134:   // Dolphin Staff																						
		case 18109:   // Leviathan's Couse																						
		case 17439:   // Leviathan's Rod																						
		case 16620:   // Merman's Sword																						
		case 16961:   // Murasame																						
		case 17556:   // Neptune's Staff																						
		case 17659:   // Seiryu's Sword																						
		case 17555:   // Water Staff																						
		case 17533:   // Whale Staff																						
		case 17534:   // Whale Staff +1																						
		{
			chance = 5;
			effect = 34;
		}
			break;

			// Additional Effect vs. Lizards: Ice Damage																						
		case 16853:   // Lizard Piercer																						
			if (PDefender->m_EcoSystem == SYSTEM_LIZARD)
			{
				effect = 35;
			}
			break;
			// Additional Effect vs. Hounds																						
		case 18199:   // Ulfhedinn Axe																						
			if (((CMobEntity*)PDefender)->m_Family == 142 || ((CMobEntity*)PDefender)->m_Family == 143)
			{
				effect = 35;
			}
			break;
			// Additional Effect: Ice Damage (5% chance guestimate)																						
		case 17548:   // Aquilo's Staff																						
		case 16702:   // Cougar Baghnakhs																						
		case 16784:   // Frostreaper																						
		case 16937:   // Ice Brand																						
		case 16861:   // Ice Lance																						
		case 16895:   // Ice Lance +1																						
		case 17547:   // Ice Staff																						
		case 18503:   // Mammut																						
		case 17711:   // Shiva's Shotel																						
		case 17135:   // Walrus Staff																						
		{
			effect = 35;
		}
			break;

			// Additional Effect: Lightning Damage with Battery																						
		case 17986:   // Spark Baselard																						
		case 17987:   // Spark Baselard +1																						
		case 17686:   // Spark Bilbo																						
		case 17687:   // Spark Bilbo +1																						
		case 17984:   // Spark Dagger																						
		case 17985:   // Spark Dagger +1																						
		case 17688:   // Spark Degen																						
		case 17689:   // Spark Degen +1																						
		case 18080:   // Spark Fork																						
		case 18081:   // Spark Fork +1																						
		case 17988:   // Spark Kris																						
		case 17989:   // Spark Kris +1																						
		case 18078:   // Spark Lance																						
		case 18079:   // Spark Lance +1																						
		case 17690:   // Spark Rapier																						
		case 17691:   // Spark Rapier +1																						
		case 18076:   // Spark Spear																						
		case 18077:   // Spark Spear +1																						
			if (PAmmo->getID() == 18228 || PAmmo->getID() == 18229 || PAmmo->getID() == 18230)
			{
				chance = 10;
				chance += rand() % 6;
				ammoUse = 1;
				effect = 36;
			}
			break;

			// Additional Effect vs. Quadav: Lightning Damage																						
		case 17415:   // Shellbuster																						
			if ((((CMobEntity*)PDefender)->m_Family == 200) || ((CMobEntity*)PDefender)->m_Family == 201
				|| ((CMobEntity*)PDefender)->m_Family == 202 || ((CMobEntity*)PDefender)->m_Family == 249
				|| ((CMobEntity*)PDefender)->m_Family == 337)
			{
				chance = 10;
				effect = 36;
			}
			break;
			// Additional Effect: Lightning Damage (5% chance guestimate)																						
		case 16888:   // Battle Fork																						
		case 16889:   // Battle Fork +1																						
		case 18129:   // Dabo																						
		case 18130:   // Dabo +1																						
		case 18104:   // Fuscina																						
		case 17071:   // Heat Rod																						
		case 17511:   // Indra Katars																						
		case 17554:   // Jupiter's Staff																						
		case 18210:   // Raifu																						
		case 18404:   // Ramuh's Mace																						
		case 17531:   // Ramuh's Staff																						
		case 17553:   // Thunder Staff																						
		case 16869:   // Thundercloud																						
		case 16837:   // Trident																						
		{
			chance = 5;
			effect = 36;
		}
			break;

			// Additional Effect: Wind Damage with Fan																						
		case 18366:   // Gust Claymore																						
		case 18367:   // Gust Claymore +1																						
		case 18368:   // Gust Sword																						
		case 18369:   // Gust Sword +1																						
		case 18370:   // Gust Tongue																						
		case 18371:   // Gust Tongue +1																						
		case 17784:   // Keppu																						
		case 17785:   // Keppu +1																						
		case 17780:   // Kyofu																						
		case 17781:   // Kyofu +1																						
		case 17782:   // Reppu																						
		case 17783:   // Reppu +1																						
			if (PAmmo->getID() == 18236 || PAmmo->getID() == 18238 || PAmmo->getID() == 18237)
			{
				chance = 10;
				chance += rand() % 6;
				ammoUse = 1;
				effect = 37;
			}
			break;
			// Additional Effect: Wind Damage (5% chance guestimate)																						
		case 17550:   // Auster's Staff																						
		case 18198:   // Byakko's Axe																						
		case 16726:   // Forseti's Axe																						
		case 18451:   // Mokusa																						
		case 18027:   // Palladium Dagger																						
		case 18598:   // Prester																						
		case 17965:   // Sirocco Axe																						
		case 16434:   // Sonic Knuckles																						
		case 16675:   // Storm Axe																						
		case 16857:   // Wind Spear																						
		case 17549:   // Wind Staff																						
		case 16542:   // Wing Sword																						
		case 17637:   // Wing Sword +1																						
		{
			chance = 5;
			effect = 37;
		}
			break;
		case 18359:   // Boreas Cesti																						
		case 18018:   // Sirocco Kukri																						
		{
			chance = 100;
			effect = 37;
		}
			break;

			// Additional Effect: Flee (0-75% chance guestimate)																						
		case 17615:   // Chicken Knife																						
		{
			effect = 38;
		}
			break;

			//Additional Effect: Dispel Weapons (10% chance needs verifying)																						
		case 16942:  // Balmung																						
		case 16944:  // Lockheart																						
		case 16950:  // Mithril Heart																						
		case 16951:  // Mithril Heart +1																						
		case 18330:  // Claustrum - 75																						
		case 18331:  // Claustrum - 80																						
		case 18648:  // Claustrum - 85																						
		case 18662:  // Claustrum - 90																						
		case 18676:  // Claustrum - 95																						
		case 19757:  // Claustrum - 99																						
		case 19850:  // Claustrum - 99 - 2																						
		{
			effect = 39;
		}
			break;

			// Additional Effect Regen (7% Guestimate)																						
		case 18887: // Dukkha																						
		case 18525: // Himthige																						
		{
			effect = 40;
		}
			break;

			// Additional Effect: Ice Spikes (7% chance guestimate)																						
		case 17564:   // Cocytus Pole																						
		{
			effect = 41;
		}
			break;
			// Additional Effect: Convert Damage to MP - Tizona																						
		case 19006:   // Tizona 75																						
		{
			tizonaChance = rand() % 2 + 10;
			tizonaPercent = rand() % 25 + 10;
			effect = 42;
		}
			break;
		case 19075:   // Tizona 80																						
		{
			tizonaChance = 15;
			tizonaPercent = rand() % 10 + 10;
			effect = 42;
		}
			break;
		case 19095:   // Tizona 85																						
		{
			tizonaChance = 20;
			tizonaPercent = rand() % 10 + 10;
			effect = 42;
		}
			break;
		case 19627:   // Tizona 90																						
		case 19725:   // Tizona 95																						
		{
			tizonaChance = 25;
			tizonaPercent = rand() % 10 + 10;
			effect = 42;
		}
			break;
		case 19834:   // Tizona 99																						
		case 19963:   // Tizona 99-2																						
		{
			tizonaChance = 30;
			tizonaPercent = rand() % 10 + 10;
			effect = 42;
		}
			break;
			// Additional Effect: Slashing Dmg based on HP Excaliburs																						
		case 18276:   // Excalibur 75																						
		case 18277:   // Excalibur 80																						
		case 18639:   // Excalibur 85																						
		case 18653:   // Excalibur 90																						
		case 18667:   // Excalibur 95																						
		case 19748:   // Excalibur 99																						
		case 19841:   // Excalibur 99-2																						
		{
			effect = 43;
		}
			break;
			// Additional Effect: Damage varies with MP																						
		case 16540:   // Tyrfing																						
		{
			effect = 44;
		}
			break;
			// Additional Effect: Damage varies with TP																						
		case 19315:   // Erebus's Lance																						
			if (PDefender->m_EcoSystem == SYSTEM_EMPTY)
			{
				effect = 45;
			}
			return;

			// Additional Effect: HP Recover																						
		case 17429:   // Dominion Mace																						
		{
			effect = 46;
		}
			return;

		default:
			break;
		}
		switch (effect)
		{
		case 1:  // HP Drain																						
			//30 % chance to drain, will heal 30% of damage done																					
			if (rand() % 100 >= 30 || PWeapon == NULL) return;
			Action->addEffectParam = (float)(Action->param * 0.3f);
			PAttacker->addHP(Action->addEffectParam);
			if (PAttacker->objtype == TYPE_PC)
			{
				charutils::UpdateHealth(PAttacker);
			}
			return;
		case 2:  // MP Recover																						
			//30 % chance to Recover MP, 4-16 MP																					
			if (rand() % 100 <= 30)
			{
				Action->addEffectMessage = 152;
				int mpRecover = 4;
				mpRecover = dsp_cap(mpRecover, 4, 16);
				mpRecover += rand() % 13; //4~16 randomised																				
				Action->addEffectParam = mpRecover;
				PAttacker->addMP(Action->addEffectParam);

				if (PAttacker->objtype == TYPE_PC){
					charutils::UpdateHealth(PAttacker);
				}
			}
			return;
		case 3: // TP Drain																						
			//10 % chance to drain, will restore TP 10% of damage done																					
			if (rand() % 100 <= 10 || PWeapon == NULL) return;

			Action->additionalEffect = SUBEFFECT_TP_DRAIN;
			Action->addEffectMessage = 165;
			Action->addEffectParam = (float)(Action->param * 0.1f);
			PAttacker->addTP(Action->addEffectParam);


			charutils::UpdateHealth(PAttacker);

			return;
		case 4:  // MP Drain																						
			//10 % chance to drain, will restore MP 10% of damage done																					
			if (rand() % 100 <= chance || PWeapon == NULL) return;

			Action->additionalEffect = SUBEFFECT_MP_DRAIN;
			Action->addEffectMessage = 162;
			Action->addEffectParam = (float)(Action->param * 0.1f);
			PAttacker->addMP(Action->addEffectParam);
			if (PAttacker->objtype == TYPE_PC)
			{
				charutils::UpdateHealth(PAttacker);
			}
			return;
		case 5:  // HP or MP Drain																						
			//15 % chance to drain, will heal 7.5% of dmg done or restore mp 7.5% of dmg done																					
			if (rand() % 100 <= 15 || PWeapon == NULL) return;
			if (rand() % 2 == 1)
				Action->additionalEffect = SUBEFFECT_HP_DRAIN;
			Action->addEffectMessage = 161;
			Action->addEffectParam = (float)(Action->param * 0.075f);
			PAttacker->addHP(Action->addEffectParam);


			if (PAttacker->objtype == TYPE_PC){
				charutils::UpdateHealth(PAttacker);
			}
			else
				Action->additionalEffect = SUBEFFECT_MP_DRAIN;
			Action->addEffectMessage = 162;
			Action->addEffectParam = (float)(Action->param * 0.075f);
			PAttacker->addMP(Action->addEffectParam);

			if (PAttacker->objtype == TYPE_PC){
				charutils::UpdateHealth(PAttacker);
			}
			return;
		case 6:  // Stun																						
			if (rand() % 100 <= 5)
			{
				if (rand() % 100 >= PDefender->getMod(MOD_STUNRES))
				{
					if (PDefender->hasImmunity(8) == false)
					{
						if (PDefender->StatusEffectContainer->AddStatusEffect(
							new CStatusEffect(EFFECT_STUN, EFFECT_STUN, 1, 0, 4)))
						{
							Action->additionalEffect = SUBEFFECT_STUN;
							Action->addEffectParam = EFFECT_STUN;
							Action->addEffectMessage = 160;
						}
					}
				}
			}
			return;
		case 7:  // Paralyze																						
			if (rand() % 100 <= 5)
			{
				if (rand() % 100 >= PDefender->getMod(MOD_PARALYZERES))
				{
					if (PDefender->hasImmunity(32) == false)
					{
						if (PDefender->StatusEffectContainer->AddStatusEffect(
							new CStatusEffect(EFFECT_PARALYSIS, EFFECT_PARALYSIS, 20, 0, 10)))
						{
							Action->additionalEffect = SUBEFFECT_PARALYSIS;
							Action->addEffectParam = EFFECT_PARALYSIS;
							Action->addEffectMessage = 160;
						}
					}
				}
			}
			return;
		case 8:  // Petrify																						
			if (rand() % 100 <= 5)
			{
				if (rand() % 100 >= PDefender->getMod(MOD_PETRIFYRES))
				{
					// if(PDefender->hasImmunity(?) == false) // Petrify immunity unknown																			
					if (PDefender->StatusEffectContainer->AddStatusEffect(
						new CStatusEffect(EFFECT_PETRIFICATION, EFFECT_PETRIFICATION, 1, 0, 30)))
					{
						Action->additionalEffect= SUBEFFECT_PETRIFY;
						Action->addEffectParam = EFFECT_PETRIFICATION;
						Action->addEffectMessage = 160;
					}
				}
			}
			return;
		//case 9:  // Terror																						
			//if (rand() % 100 <= 5)
			//{
				// if(PDefender->hasImmunity(?) == false) // Terror immunity unknown																				
				//if (rand() % 100 >= PDefender->getMod(MOD_TERRORRES))
				//{
					//if (PDefender->StatusEffectContainer->AddStatusEffect(
						//new CStatusEffect(EFFECT_TERROR, EFFECT_TERROR, 1, 0, 10)))
					//{
						//Action->subparam = EFFECT_TERROR;
						//Action->submessageID = 160;
						//Action->flag = 3;
					//}
				//}

			//}
			//return;
		//case 10: // Addle																						
			//if (rand() % 100 <= 5)
			//{
				// if(PDefender->hasImmunity(?) == false) // Addle immunity unknown																				
				//if (PDefender->StatusEffectContainer->AddStatusEffect(
				//	new CStatusEffect(EFFECT_ADDLE, EFFECT_ADDLE, 1, 0, 10)))
				//{
				//	Action->subparam = EFFECT_ADDLE;
					//Action->submessageID = 160;
					//Action->flag = 1;
				//}
			//}
			//return;
		//case 11:  // Choke																						
			//if (rand() % 100 <= 5)
			//{
				// if(PDefender->hasImmunity(?) == false) // Choke immunity unknown																				
			//	if (PDefender->StatusEffectContainer->AddStatusEffect(
				//	new CStatusEffect(EFFECT_CHOKE, EFFECT_CHOKE, 5, 3, 10)))
				//{
				//	Action->additionalEffect = SUBEFFECT_ADDLE;
				//	Action->addEffectMessage = 160;
				//}
			//}
			//return;
		case 12:  // Weight																						
			if (rand() % 100 <= 5)
			{
				if (PDefender->hasImmunity(2) == false)
				{
					if (rand() % 100 >= PDefender->getMod(MOD_GRAVITYRES))
					{
						if (PDefender->StatusEffectContainer->AddStatusEffect(
							new CStatusEffect(EFFECT_WEIGHT, EFFECT_WEIGHT, 50, 0, 30)))
						{
							Action->additionalEffect = SUBEFFECT_WEIGHT;
							Action->addEffectParam = EFFECT_WEIGHT;
							Action->addEffectMessage = 160;
						}
					}
				}
			}
			return;
		//case 13:  // Amnesia																						
			//if (rand() % 100 <= 5)
			//{
				// if(PDefender->hasImmunity(?) == false) // Amnesia immunity unknown																				
				//if (rand() % 100 >= PDefender->getMod(MOD_AMNESIARES))
				//{
					//if (PDefender->StatusEffectContainer->AddStatusEffect(
					//	new CStatusEffect(EFFECT_AMNESIA, EFFECT_AMNESIA, 1, 0, 10)))
					//{
					//	Action->subparam = EFFECT_AMNESIA;
					//	Action->submessageID = 160;
					//	Action->flag = 1;
					//}
				//}

	//		}
		//	return;
		case 14:  // Bind																						
			if (rand() % 100 <= 5)
			{
				if (PDefender->hasImmunity(4) == false)
				{
					if (rand() % 100 >= PDefender->getMod(MOD_BINDRES))
					{
						if (PDefender->StatusEffectContainer->AddStatusEffect(
							new CStatusEffect(EFFECT_BIND, EFFECT_BIND, 1, 0, 10)))
						{
							Action->additionalEffect = SUBEFFECT_BIND;
							Action->addEffectParam = EFFECT_BIND;
							Action->addEffectMessage = 160;
						}
					}
				}
			}
			return;
		case 15:  // Slow																						
			if (rand() % 100 <= 5)
			{
				if (PDefender->hasImmunity(128) == false)
				{
					if (rand() % 100 >= PDefender->getMod(MOD_SLOWRES))
					{
						if (PDefender->StatusEffectContainer->AddStatusEffect(
							new CStatusEffect(EFFECT_HASTE, EFFECT_HASTE, -51, 0, 30)))
						{
							Action->addEffectParam = EFFECT_HASTE;
							Action->addEffectMessage = 160;
						}
					}
				}
			}
			return;
		case 16:  // Flash																						
			if (rand() % 100 <= 5)
			{
				if (PDefender->hasImmunity(64) == false)
				{
					if (PDefender->StatusEffectContainer->AddStatusEffect(
						new CStatusEffect(EFFECT_FLASH, EFFECT_FLASH, 100, 0, 30)))
					{
						Action->additionalEffect = SUBEFFECT_BLIND;
						Action->addEffectParam = EFFECT_FLASH;
						Action->addEffectMessage = 160;
					}
				}
			}
			return;
		case 17:  // TP Removal																						
			if (rand() % 100 <= 2)
			{
				Action->addEffectMessage = 363;
				int tpRemove = 5;
				tpRemove = dsp_cap(tpRemove, 5, 50);
				tpRemove += rand() % 46;	//Random 5-50																			
				PDefender->addTP(-tpRemove);

			}
			return;
		case 18:  // STR Boost																						
			if (rand() % 100 <= 10)
			{
				int strBoost = 5;
				strBoost = dsp_cap(strBoost, 5, 7);
				strBoost += rand() % 3; //5~7 randomised																				
				if (PAttacker->StatusEffectContainer->AddStatusEffect(
					new CStatusEffect(EFFECT_STR_BOOST, EFFECT_STR_BOOST, strBoost, 0, 30)))
				{
					Action->addEffectParam = EFFECT_STR_BOOST;
					Action->addEffectMessage = 166;
				}
			}
			return;
		case 19:  // Haste																						
			if (rand() % 100 <= 7)
			{
				if (PAttacker->StatusEffectContainer->AddStatusEffect(
					new CStatusEffect(EFFECT_HASTE, EFFECT_HASTE, 307, 0, 30)))
				{
					Action->addEffectParam = EFFECT_HASTE;
					Action->addEffectMessage = 166;
				}
			}
			return;
		case 20:  // Blink																						
			if (rand() % 100 <= 15)
			{
				if (PAttacker->StatusEffectContainer->AddStatusEffect(
					new CStatusEffect(EFFECT_BLINK, EFFECT_BLINK, 1, 0, 5)))
				{
					Action->addEffectParam = EFFECT_BLINK;
					Action->addEffectMessage = 166;
				}
			}
			return;
		case 21:  // Poison																						
			if (rand() % 100 <= 5)
			{
				if (rand() % 100 >= PDefender->getMod(MOD_POISONRES))
				{
					if (PDefender->hasImmunity(256) == false)
					{
						if (PDefender->StatusEffectContainer->AddStatusEffect(
							new CStatusEffect(EFFECT_POISON, EFFECT_POISON, 1, 3, 30)))
						{
							Action->additionalEffect = SUBEFFECT_POISON;
							Action->addEffectParam = EFFECT_POISON;
							Action->addEffectMessage = 160;
						}
					}
				}
			}
			return;
		case 22:  // Blind																						
			if (rand() % 100 <= 5)
			{
				if (rand() % 100 >= PDefender->getMod(MOD_BLINDRES))
				{
					if (PDefender->hasImmunity(64) == false)
					{
						if (PDefender->StatusEffectContainer->AddStatusEffect(
							new CStatusEffect(EFFECT_BLINDNESS, EFFECT_BLINDNESS, 10, 0, 10)))
						{
							Action->additionalEffect = SUBEFFECT_BLIND;
							Action->addEffectParam = EFFECT_BLINDNESS;
							Action->addEffectMessage = 160;
						}
					}
				}
			}
			return;
		case 23:  // Silence																						
			if (rand() % 100 <= 5)
			{
				if (rand() % 100 >= PDefender->getMod(MOD_SILENCERES))
				{
					if (PDefender->hasImmunity(16) == false)
					{
						if (PDefender->StatusEffectContainer->AddStatusEffect(
							new CStatusEffect(EFFECT_SILENCE, EFFECT_SILENCE, 1, 0, 10)))
						{
							Action->additionalEffect = SUBEFFECT_SILENCE;
							Action->addEffectParam = EFFECT_SILENCE;
							Action->addEffectMessage = 160;
						}
					}
				}
			}
			return;
		case 24:  // Curse																						
			if (rand() % 100 <= 5)
			{
				if (rand() % 100 >= PDefender->getMod(MOD_CURSERES))
				{
					// if(PDefender->hasImmunity(?) == false) // Curse immunity unknown																			

					if (PDefender->StatusEffectContainer->AddStatusEffect(
						new CStatusEffect(EFFECT_CURSE, EFFECT_CURSE, 1, 0, 10)))
					{
						Action->additionalEffect = SUBEFFECT_CURSE;
						Action->addEffectParam = EFFECT_CURSE;
						Action->addEffectMessage = 160;
					}

				}
			}
			return;
		//case 25:  // Death																						
			//if (rand() % 100 <= 2)
			//{
			//	if (rand() % 100 >= PDefender->getMod(MOD_DEATHRES))
				//{
					// if(PDefender->hasImmunity(?) == false) // Death immunity unknown																			

				//	Action->subparam = SUBEFFECT_NONE;
				//	Action->submessageID = 160;
				//	Action->flag = 3;
				//	PDefender->addHP(-999999);
				//}
			//}
			//return;
		case 26:  // Sleep																						
			if (rand() % 100 <= 5)
			{
				if (rand() % 100 >= PDefender->getMod(MOD_SLEEPRES))
				{
					if (!PDefender->isDead() && PDefender->hasImmunity(1) == false)
					{
						int duration = 25 - (PDefender->GetMLevel() - PAttacker->GetMLevel()) * 5 - ((float)PDefender->getMod(MOD_SLEEPRES) / 5);

						if (duration <= 1)
						{
							duration = 1;
						}
						else
						{
							//randomize sleep duration																	
							duration -= rand() % (duration / 2);
						}

						duration = dsp_cap(duration, 1, 25);
						if (PDefender->StatusEffectContainer->AddStatusEffect(
							new CStatusEffect(EFFECT_SLEEP, EFFECT_SLEEP, 1, 0, duration)))
						{
							Action->additionalEffect = SUBEFFECT_SLEEP;
							Action->addEffectParam = EFFECT_SLEEP;
							Action->addEffectMessage = 160;
						}
					}
				}
			}
			return;
		case 27:  // Evasion Down																						
			if (rand() % 100 <= 5)
			{
				if (PDefender->StatusEffectContainer->AddStatusEffect(
					new CStatusEffect(EFFECT_EVASION_DOWN, EFFECT_EVASION_DOWN, 10, 0, 10)))
				{
					Action->additionalEffect = SUBEFFECT_IMPAIRS_EVASION;
					Action->addEffectParam = EFFECT_EVASION_DOWN;
					Action->addEffectMessage = 160;
				}
			}
			return;
		case 28:  // Defense Down																						
			if (rand() % 100 <= 5)
				{
					Action->additionalEffect = SUBEFFECT_DEFENSE_DOWN;
					Action->addEffectParam = EFFECT_DEFENSE_DOWN;
					Action->addEffectMessage = 160;
				}
			
			return;
		case 29:  // Weaken Attack																						
			if (rand() % 100 <= 5)
				{
					Action->addEffectParam = EFFECT_ATTACK_DOWN;
					Action->addEffectMessage = 160;
				}
			return;
		case 30:  // Darkness Dmg																						
			if (rand() % 100 <= chance)
			{
				if (rand() % 100 >= PDefender->getMod(MOD_DARKRES))
				{
					Action->additionalEffect = SUBEFFECT_DARKNESS_DAMAGE;
					Action->addEffectMessage = 163;
					int damage = (PAttacker->INT() - PDefender->INT()) / 2;
					damage += (PAttacker->GetMLevel() - PDefender->GetMLevel());
					damage = dsp_cap(damage, 0, 50);
					damage += PAttacker->GetMLevel() / 2;
					damage += rand() % 20; //At 75 -> 37~56 low or 87~106 high																			

					damage += (float)damage * ((float)PDefender->getMod(MOD_DARKRES) / -100);

					Action->addEffectParam = damage;
					PDefender->addHP(-damage);
				}
			}
			return;
		case 31:  // Earth Dmg																						
			if (rand() % 100 <= 5)
			{
				if (rand() % 100 >= PDefender->getMod(MOD_EARTHRES))
				{
					Action->additionalEffect = SUBEFFECT_EARTH_DAMAGE;
					Action->addEffectMessage = 163;
					//calculate damage																			
					uint8 damage = (PAttacker->VIT() - PDefender->VIT()) / 2;
					damage = dsp_cap(damage, 0, 50);
					damage += 10; //10~60																			
					damage += rand() % 8; //10~67 randomised																			
					//set damage TODO: handle resist/staff/day																			

					damage += (float)damage * ((float)PDefender->getMod(MOD_EARTHRES) / -100);
					Action->addEffectParam = damage;
					PDefender->addHP(-damage);
				}
			}
			return;
		case 32:  // Fire Dmg																						
			if (rand() % 100 <= 5)
			{
				if (rand() % 100 >= PDefender->getMod(MOD_FIRERES))
				{
					Action->additionalEffect = SUBEFFECT_FIRE_DAMAGE;
					Action->addEffectMessage = 163;
					//calculate damage																			
					uint8 damage = (PAttacker->INT() - PDefender->INT()) / 2;
					damage = dsp_cap(damage, 0, 50);

					damage += 10; //10~60																			
					damage += rand() % 8; //10~67 randomised																			
					//set damage TODO: handle resist/staff/day																			
					damage += (float)damage * ((float)PDefender->getMod(MOD_FIRERES) / -100);

					Action->addEffectParam = damage;
					PDefender->addHP(-damage);
				}
			}
			return;
		case 33:  // Light Dmg																						
			if (rand() % 100 <= chance)
			{
				if (rand() % 100 >= PDefender->getMod(MOD_LIGHTRES))
				{
					Action->additionalEffect = SUBEFFECT_LIGHT_DAMAGE;
					Action->addEffectMessage = 163;
					//calculate damage																			
					uint8 damage = (PAttacker->MND() - PDefender->MND()) / 2;
					damage = dsp_cap(damage, 0, 50);
					damage += 10; //10~60																			
					damage += rand() % 8; //10~67 randomised																			
					//set damage TODO: handle resist/staff/day																			
					damage += (float)damage * ((float)PDefender->getMod(MOD_LIGHTRES) / -100);
					Action->addEffectParam = damage;
					PDefender->addHP(-damage);
				}
			}
			return;
		case 34:  // Water Dmg																						
			if (rand() % 100 <= chance)
			{
				if (rand() % 100 >= PDefender->getMod(MOD_WATERRES))
				{
					Action->additionalEffect = SUBEFFECT_WATER_DAMAGE;
					Action->addEffectMessage = 163;
					//calculate damage																			
					uint8 damage = (PAttacker->MND() - PDefender->MND()) / 2;
					damage = dsp_cap(damage, 0, 50);
					if (ammoUse == 1)
					{
						if (PAmmo->getID() == 18232) // Hydro Pump																		
						{
							damage += 10; //10~60																	
							damage += rand() % 6; //10~65 randomised																	
						}
						else if (PAmmo->getID() == 18233) // Kilo Pump																		
						{
							damage += 20; //20~70																	
							damage += rand() % 6; //20~75 randomised																	
						}
						else if (PAmmo->getID() == 18234) // Mega Pump																		
						{
							damage += 30; //30~80																	
							damage += rand() % 6; //30~85 randomised																	
						}
						// ShowDebug(CL_CYAN"Water Pump Removal \n");																		
						if ((PAmmo->getQuantity() - 1) < 1) // ammo will run out after this attack, make sure we remove it from equip																		
						{
							uint8 slot = PAttacker->equip[SLOT_AMMO];
							charutils::UnequipItem(PAttacker, SLOT_AMMO);
							charutils::UpdateItem(PAttacker, LOC_INVENTORY, slot, -1);
						}
						else
						{
							charutils::UpdateItem(PAttacker, LOC_INVENTORY, PAttacker->equip[SLOT_AMMO], -1);
						}
						PAttacker->pushPacket(new CInventoryFinishPacket());
					}
					else
					{
						damage += 10; //10~60																			
						damage += rand() % 8; //10~67 randomised																			
					}
					//set damage TODO: handle resist/staff/day																			
					damage += (float)damage * ((float)PDefender->getMod(MOD_WATERRES) / -100);
					Action->addEffectParam = damage;
					PDefender->addHP(-damage);
				}
			}
			return;
		case 35:  // Ice Dmg																						
			if (rand() % 100 <= 5)
			{
				if (rand() % 100 >= PDefender->getMod(MOD_ICERES))
				{
					Action->additionalEffect = SUBEFFECT_ICE_DAMAGE;
					Action->addEffectMessage = 163;
					//calculate damage																			
					uint8 damage = (PAttacker->INT() - PDefender->INT()) / 2;
					damage = dsp_cap(damage, 0, 50);
					damage += 10; //10~60																			
					damage += rand() % 8; //10~67 randomised																			
					//set damage TODO: handle resist/staff/day																			
					damage += (float)damage * ((float)PDefender->getMod(MOD_ICERES) / -100);
					Action->addEffectParam = damage;
					PDefender->addHP(-damage);
				}
			}
			return;
		case 36:  // Lightning Dmg																						
			if (rand() % 100 <= chance)
			{
				if (rand() % 100 >= PDefender->getMod(MOD_THUNDERRES))
				{
					Action->additionalEffect = SUBEFFECT_LIGHTNING_DAMAGE;
					Action->addEffectMessage = 163;
					//calculate damage																			
					uint8 damage = (PAttacker->DEX() - PDefender->DEX()) / 2;
					damage = dsp_cap(damage, 0, 50);
					if (ammoUse == 1)
					{
						if (PAmmo->getID() == 18228) // Battery																		
						{
							damage += 10; //10~60																	
							damage += rand() % 6; //10~65 randomised																	
						}
						else if (PAmmo->getID() == 18229) // Kilo Battery																		
						{
							damage += 20; //20~70																	
							damage += rand() % 6; //20~75 randomised																	
						}
						else if (PAmmo->getID() == 18230) // Mega Battery																		
						{
							damage += 30; //30~80																	
							damage += rand() % 6; //30~85 randomised																	
						}
						// ShowDebug(CL_CYAN"Battery Removal \n");																		
						if ((PAmmo->getQuantity() - 1) < 1) // ammo will run out after this attack, make sure we remove it from equip																		
						{
							uint8 slot = PAttacker->equip[SLOT_AMMO];
							charutils::UnequipItem(PAttacker, SLOT_AMMO);
							charutils::UpdateItem(PAttacker, LOC_INVENTORY, slot, -1);
						}
						else
						{
							charutils::UpdateItem(PAttacker, LOC_INVENTORY, PAttacker->equip[SLOT_AMMO], -1);
						}
						PAttacker->pushPacket(new CInventoryFinishPacket());
					}
					else
					{
						damage += 10; //10~60																		
						damage += rand() % 8; //10~67 randomised																		
					}
					//set damage TODO: handle resist/staff/day																			
					damage += (float)damage * ((float)PDefender->getMod(MOD_THUNDERRES) / -100);
					Action->addEffectParam = damage;
					PDefender->addHP(-damage);
				}
			}
			return;
		case 37:  // Wind Dmg																						
			if (rand() % 100 <= chance)
			{
				if (rand() % 100 >= PDefender->getMod(MOD_WINDRES))
				{
					Action->additionalEffect = SUBEFFECT_WIND_DAMAGE;
					Action->addEffectMessage = 163;
					//calculate damage																			
					uint8 damage = (PAttacker->AGI() - PDefender->AGI()) / 2;
					damage = dsp_cap(damage, 0, 50);
					if (ammoUse == 1)
					{
						if (PAmmo->getID() == 18236) // Wind Fan																		
						{
							damage += 10; //10~60																	
							damage += rand() % 6; //10~65 randomised																	
						}
						else if (PAmmo->getID() == 18237) // Kilo Fan																		
						{
							damage += 20; //20~70																	
							damage += rand() % 6; //20~75 randomised																	
						}
						else if (PAmmo->getID() == 18238) // Mega Fan																		
						{
							damage += 30; //30~80																	
							damage += rand() % 6; //30~85 randomised																	
						}
						// ShowDebug(CL_CYAN"Wind Fan Removal \n");																		
						if ((PAmmo->getQuantity() - 1) < 1) // ammo will run out after this attack, make sure we remove it from equip																		
						{
							uint8 slot = PAttacker->equip[SLOT_AMMO];
							charutils::UnequipItem(PAttacker, SLOT_AMMO);
							charutils::UpdateItem(PAttacker, LOC_INVENTORY, slot, -1);
						}
						else
						{
							charutils::UpdateItem(PAttacker, LOC_INVENTORY, PAttacker->equip[SLOT_AMMO], -1);
						}
						PAttacker->pushPacket(new CInventoryFinishPacket());
					}
					else
					{
						damage += 10; //10~60																		
						damage += rand() % 8; //10~67 randomised																		
					}
					damage += (float)damage * ((float)PDefender->getMod(MOD_WINDRES) / -100);
					//set damage TODO: handle resi st/staff/day																			
					Action->addEffectParam = damage;
					PDefender->addHP(-damage);
				}
			}
			return;
		case 38:  // Flee																						
			chance = 0;
			if (PDefender->GetMLevel() > PAttacker->GetMLevel())
			{
				chance += 5 * (PDefender->GetMLevel() - PAttacker->GetMLevel());
				chance = dsp_cap(chance, 0, 75);
			}
			if (rand() % 100 <= chance)
			{
				if (PAttacker->StatusEffectContainer->AddStatusEffect(
					new CStatusEffect(EFFECT_FLEE, EFFECT_FLEE, 100, 0, 30)))
				{
					Action->addEffectParam = EFFECT_FLEE;
					Action->addEffectMessage = 160;
				}
			}
			return;
		//case 39:  // Dispel																						
		//	if (rand() % 100 > 10) return;
			//dispelled = PDefender->StatusEffectContainer->DispelStatusEffect();
			// if(dispelled > 0){																							
			//     Action->subparam = dispelled;																							
			// }																							
			//return;
		case 40:  // Regen																						
			if (rand() % 100 <= 7)
			{
				if (PAttacker->StatusEffectContainer->AddStatusEffect(
					new CStatusEffect(EFFECT_REGEN, EFFECT_REGEN, 30, 3, 180)))
				{
					Action->addEffectParam = EFFECT_REGEN;
					Action->addEffectMessage = 166;
				}
			}
			return;
		case 41:  // Ice Spikes																						
			if (rand() % 100 <= 7)
			{
				if (PAttacker->StatusEffectContainer->AddStatusEffect(
					new CStatusEffect(EFFECT_ICE_SPIKES, EFFECT_ICE_SPIKES, 20, 0, 90)))
				{
					Action->addEffectParam = EFFECT_ICE_SPIKES;
					Action->addEffectMessage = 166;
				}
			}
			return;
		case 42:  // Damage to MP - Tizona																						
			//12-20 % chance to Recover MP																					
			if (rand() % 100 <= tizonaChance)
			{
				Action->addEffectMessage = 152;
				int mpRecover = ceil((float)(finaldamage * (tizonaPercent / 100.0f)));
				// ShowDebug(CL_CYAN"Tizona MP Recovery %d \n", mpRecover);																				
				Action->addEffectParam = mpRecover;
				PAttacker->addMP(Action->addEffectParam);

				if (PAttacker->objtype == TYPE_PC){
					charutils::UpdateHealth(PAttacker);
				}
			}
			return;
		case 43:  // Slashing Dmg -- Excalibur																						
		{
			Action->additionalEffect = SUBEFFECT_LIGHT_DAMAGE;
			Action->addEffectMessage = 163;
			uint8 damage = (PAttacker->health.maxhp / 4.0f);
			Action->addEffectParam = damage;
			PDefender->addHP(-damage);
		}
			return;
		case 44:  // Dmg varies with MP -- Tyrfing																						
			if (rand() % 100 <= 10)
			{
				Action->addEffectMessage = 163;
				uint8 damage = ceil((float)PAttacker->health.mp / 5.24f);
				Action->addEffectParam = damage;
				PDefender->addHP(-damage);
			}
			return;
		case 45:  // Dmg varies with TP -- Erebus's Lance																						
			if (rand() % 100 <= 5)
			{
				Action->additionalEffect = SUBEFFECT_LIGHT_DAMAGE;
				Action->addEffectMessage = 163;
				uint8 damage = 40;
				damage += ceil((float)PAttacker->health.tp / 2.0f);

				Action->addEffectParam = damage;
				PDefender->addHP(-damage);
			}
			return;
		case 46:  // HP Recover																						
			//15 % chance to Recover HP, 4-16 MP																					
			if (rand() % 100 <= 15)
			{
				Action->addEffectMessage = 167;
				int hpRecover = 4;
				hpRecover = dsp_cap(hpRecover, 4, 16);
				hpRecover += rand() % 13; //4~16 randomised																				
				Action->addEffectParam = hpRecover;
				PAttacker->addHP(Action->addEffectParam);

				if (PAttacker->objtype == TYPE_PC){
					charutils::UpdateHealth(PAttacker);
				}
			}
			return;
		default:
			return;
		}
	}
}		
	/************************************************************************
	* *
	* Handles Ranged weapon's additional effects (e.g. Bolts) *
	* *
	************************************************************************/
	// TODO: remove function, move additional effects into items script files (deleting from switch as they get done)
	void HandleRangedAdditionalEffect(CCharEntity* PAttacker, CBattleEntity* PDefender, apAction_t* Action){
		CItemWeapon* PAmmo = (CItemWeapon*)PAttacker->getStorage(LOC_INVENTORY)->GetItem(PAttacker->equip[SLOT_AMMO]);
		//add effects dont have 100% proc, presume level dependant. 95% chance but -5% for each level diff.
		//capped at 5% proc when mob is 18 (!!!) levels higher than you.
		uint8 chance = 95;
		if (PDefender->GetMLevel() > PAttacker->GetMLevel()){
			chance -= 5 * (PDefender->GetMLevel() - PAttacker->GetMLevel());
			chance = dsp_cap(chance, 5, 95);
		}
		if (WELL512::irand() % 100 >= chance || PAmmo == NULL){ return; }
		switch (PAmmo->getID()){
		case 18700:{ //Wind Arrow
			//damage doesn't exceed ~67 unless wearing wind staff/iceday/weather
			//there isn't a formula, but INT affects damage, so this is guesstimated. It seems to be level
			//invarient since its used on harder monsters for damage occasionally. Assuming the modifier
			//is simply AGI with a degree of randomisation
			Action->additionalEffect = SUBEFFECT_WIND_DAMAGE;
			Action->addEffectMessage = 163;
			//calculate damage
			uint8 damage = (PAttacker->AGI() - PDefender->AGI()) / 2;
			damage = dsp_cap(damage, 0, 50);
			damage += 10; //10~60
			damage += WELL512::irand() % 8; //10~67 randomised
			damage += (float)damage * ((float)PDefender->getMod(MOD_WINDRES) / -100);
			damage = HandleStoneskin(PDefender, damage);
			//set damage TODO: handle resi st/staff/day
			Action->addEffectParam = damage;
			PDefender->addHP(-damage);
		}
			break;
		case 18699:{ //Earth Arrow
			//damage doesn't exceed ~67 unless wearing Earth staff/earthsday/weather
			//there isn't a formula, but VIT affects damage, so this is guesstimated. It seems to be level
			//invarient since its used on harder monsters for damage occasionally. Assuming the modifier
			//is simply VIT with a degree of randomisation
			Action->additionalEffect = SUBEFFECT_EARTH_DAMAGE;
			Action->addEffectMessage = 163;
			//calculate damage
			uint8 damage = (PAttacker->VIT() - PDefender->VIT()) / 2;
			damage = dsp_cap(damage, 0, 50);
			damage += 10; //10~60
			damage += WELL512::irand() % 8; //10~67 randomised
			//set damage TODO: handle resist/staff/day
			damage += (float)damage * ((float)PDefender->getMod(MOD_EARTHRES) / -100);
			damage = HandleStoneskin(PDefender, damage);
			Action->addEffectParam = damage;
			PDefender->addHP(-damage);
		}
			break;
		case 18698:{ //Water Arrow
			//damage doesn't exceed ~67 unless wearing light staff/iceday/weather
			//there isn't a formula, but INT affects damage, so this is guesstimated. It seems to be level
			//invarient since its used on harder monsters for damage occasionally. Assuming the modifier
			//is simply MND with a degree of randomisation
			Action->additionalEffect = SUBEFFECT_WATER_DAMAGE;
			Action->addEffectMessage = 163;
			//calculate damage
			uint8 damage = (PAttacker->MND() - PDefender->MND()) / 2;
			damage = dsp_cap(damage, 0, 50);
			damage += 10; //10~60
			damage += WELL512::irand() % 8; //10~67 randomised
			//set damage TODO: handle resist/staff/day
			damage += (float)damage * ((float)PDefender->getMod(MOD_WATERRES) / -100);
			damage = HandleStoneskin(PDefender, damage);
			Action->addEffectParam = damage;
			PDefender->addHP(-damage);
		}
			break;
		case 18153:{ //Holy Bolt
			//damage doesn't exceed ~67 unless wearing light staff/lightsday/weather
			//there isn't a formula, but MND affects damage, so this is guesstimated. It seems to be level
			//invarient since its used on harder monsters for damage occasionally. Assuming the modifier
			//is simply MND with a degree of randomisation
			Action->additionalEffect = SUBEFFECT_LIGHT_DAMAGE;
			Action->addEffectMessage = 163;
			//calculate damage
			uint8 damage = (PAttacker->MND() - PDefender->MND()) / 2;
			damage = dsp_cap(damage, 0, 50);
			damage += 10; //10~60
			damage += WELL512::irand() % 8; //10~67 randomised
			//set damage TODO: handle resist/staff/day
			damage += (float)damage * ((float)PDefender->getMod(MOD_LIGHTRES) / -100);
			damage = HandleStoneskin(PDefender, damage);
			Action->addEffectParam = damage;
			PDefender->addHP(-damage);
		}
			break;
		case 17324:{ //Lightning Arrow
			//damage doesn't exceed ~67
			//there isn't a formula. It seems to be level
			//invarient since its used on harder monsters for damage occasionally. Assuming the modifier
			//is simply DEX with a degree of randomisation
			Action->additionalEffect = SUBEFFECT_LIGHTNING_DAMAGE;
			Action->addEffectMessage = 163;
			//calculate damage
			uint8 damage = (PAttacker->DEX() - PDefender->DEX()) / 2;
			damage = dsp_cap(damage, 0, 50);
			damage += 10; //10~60
			damage += WELL512::irand() % 8; //10~67 randomised
			//set damage TODO: handle resist/staff/day
			damage += (float)damage * ((float)PDefender->getMod(MOD_THUNDERRES) / -100);
			damage = HandleStoneskin(PDefender, damage);
			Action->addEffectParam = damage;
			PDefender->addHP(-damage);
		}
			break;
		case 17323:{ //Ice Arrow
			//damage doesn't exceed ~67 unless wearing ice staff/iceday/weather
			//there isn't a formula, but INT affects damage, so this is guesstimated. It seems to be level
			//invarient since its used on harder monsters for damage occasionally. Assuming the modifier
			//is simply INT with a degree of randomisation
			Action->additionalEffect = SUBEFFECT_ICE_DAMAGE;
			Action->addEffectMessage = 163;
			//calculate damage
			uint8 damage = (PAttacker->INT() - PDefender->INT()) / 2;
			damage = dsp_cap(damage, 0, 50);
			damage += 10; //10~60
			damage += WELL512::irand() % 8; //10~67 randomised
			//set damage TODO: handle resist/staff/day
			damage += (float)damage * ((float)PDefender->getMod(MOD_ICERES) / -100);
			damage = HandleStoneskin(PDefender, damage);
			Action->addEffectParam = damage;
			PDefender->addHP(-damage);
		}
			break;
		case 17327: // Grand knights Arrow
		case 17322:{ //Fire Arrow
			//damage doesn't exceed ~67 unless wearing ice staff/iceday/weather
			//there isn't a formula, but INT affects damage, so this is guesstimated. It seems to be level
			//invarient since its used on harder monsters for damage occasionally. Assuming the modifier
			//is simply INT with a degree of randomisation
			Action->additionalEffect = SUBEFFECT_FIRE_DAMAGE;
			Action->addEffectMessage = 163;
			//calculate damage
			uint8 damage = (PAttacker->INT() - PDefender->INT()) / 2;
			damage = dsp_cap(damage, 0, 50);
			damage += 10; //10~60
			damage += WELL512::irand() % 8; //10~67 randomised
			//set damage TODO: handle resist/staff/day
			damage += (float)damage * ((float)PDefender->getMod(MOD_FIRERES) / -100);
			if (PAmmo->getID() == 17327){
				damage *= 2;
			}
			damage = HandleStoneskin(PDefender, damage);
			Action->addEffectParam = damage;
			PDefender->addHP(-damage);
		}
			break;
		}
	}
	uint8 GetRangedHitRate(CBattleEntity* PAttacker, CBattleEntity* PDefender, bool isBarrage)
	{
		int acc = 0;
		int hitrate = 75;
		if (PAttacker->objtype == TYPE_PC)
		{
			CCharEntity* PChar = (CCharEntity*)PAttacker;
			CItemWeapon* PItem = (CItemWeapon*)PChar->getEquip(SLOT_RANGED);
			if (PItem == NULL || !PItem->isType(ITEM_WEAPON))
			{
				// try throwing weapon
				PItem = (CItemWeapon*)PChar->getEquip(SLOT_AMMO);
			}
			if (PItem != NULL && PItem->isType(ITEM_WEAPON))
			{
				int skill = PChar->GetSkill(PItem->getSkillType());
				acc = skill;
				if (skill > 200)
				{
					acc = 200 + (skill - 200)*0.9;
				}
				acc += PChar->getMod(MOD_RACC);
				acc += battleutils::GetRangedAccuracyBonuses(PAttacker);
				acc += PChar->AGI() / 2;
				acc = ((100 + PChar->getMod(MOD_RACCP)) * acc) / 100 +
					dsp_min(((100 + PChar->getMod(MOD_FOOD_RACCP)) * acc) / 100, PChar->getMod(MOD_FOOD_RACC_CAP));
			}
		}
		//Check For Ambush Merit - Ranged
		if (PAttacker->objtype == TYPE_PC && (charutils::hasTrait((CCharEntity*)PAttacker, TRAIT_AMBUSH)) && ((abs(PDefender->loc.p.rotation - PAttacker->loc.p.rotation) < 23))) {
			acc += ((CCharEntity*)PAttacker)->PMeritPoints->GetMeritValue(MERIT_AMBUSH, (CCharEntity*)PAttacker);
		}
		int eva = PDefender->EVA();
		hitrate = hitrate + (acc - eva) / 2 + (PAttacker->GetMLevel() - PDefender->GetMLevel()) * 2;
		uint8 finalhitrate = dsp_cap(hitrate, 20, 95);
		return finalhitrate;
	}
	//todo: need to penalise attacker's RangedAttack depending on distance from mob. (% decrease)
	float GetRangedPDIF(CBattleEntity* PAttacker, CBattleEntity* PDefender)
	{
		//get ranged attack value
		uint16 rAttack = 1;
		if (PAttacker->objtype == TYPE_PC)
		{
			CCharEntity* PChar = (CCharEntity*)PAttacker;
			CItemWeapon* PItem = (CItemWeapon*)PChar->getEquip(SLOT_RANGED);
			if (PItem != NULL && PItem->isType(ITEM_WEAPON))
			{
				rAttack = PChar->RATT(PItem->getSkillType());
			}
			else
			{
				PItem = (CItemWeapon*)PChar->getEquip(SLOT_AMMO);
				if (PItem == NULL || !PItem->isType(ITEM_WEAPON) || (PItem->getSkillType() != SKILL_THR)){
					ShowDebug("battleutils::GetRangedPDIF Cannot find a valid ranged weapon to calculate PDIF for. \n");
				}
				else
				{
					rAttack = PChar->RATT(PItem->getSkillType());
				}
			}
		}
		else
		{
			//assume mobs capped
			rAttack = battleutils::GetMaxSkill(SKILL_ARC, JOB_RNG, PAttacker->GetMLevel());
		}
		//get ratio (not capped for RAs)
		float ratio = (float)rAttack / (float)PDefender->DEF();
		dsp_cap(ratio, 0, 3);
		//level correct (0.025 not 0.05 like for melee)
		if (PDefender->GetMLevel() > PAttacker->GetMLevel()){
			ratio -= 0.025f * (PDefender->GetMLevel() - PAttacker->GetMLevel());
		}
		//calculate min/max PDIF
		float minPdif = 0;
		float maxPdif = 0;
		if (ratio < 0.9)
		{
			minPdif = ratio;
			maxPdif = (10.0f / 9.0f) * ratio;
		}
		else if (ratio <= 1.1)
		{
			minPdif = 1;
			maxPdif = 1;
		}
		else
		{
			minPdif = (-3.0f / 19.0f) + ((20.0f / 19.0f) * ratio);
			maxPdif = ratio;
		}
		dsp_cap(minPdif, 0, 3);
		dsp_cap(maxPdif, 0, 3);
		//return random number between the two
		return ((maxPdif - minPdif) * WELL512::drand()) + minPdif;
	}
	int16 CalculateBaseTP(int delay){
		int16 x = 1;
		if (delay <= 180){
			x = 50 + (((float)delay - 180)*1.5f) / 18;
		}
		else if (delay <= 450){
			x = 50 + (((float)delay - 180)*6.5f) / 27;
		}
		else if (delay <= 480){
			x = 115 + (((float)delay - 450)*1.5f) / 3;
		}
		else if (delay <= 530){
			x = 130 + (((float)delay - 480)*1.5f) / 5;
		}
		else{
			x = 145 + (((float)delay - 530)*3.5f) / 47;
		}
		return x;
	}
	bool TryInterruptSpell(CBattleEntity* PAttacker, CBattleEntity* PDefender){
		// cannot interrupt when manafont is active
		if (PDefender->StatusEffectContainer->HasStatusEffect(EFFECT_MANAFONT))
		{
			return false;
		}
		//Reasonable assumption for the time being.
		int base = 40;
		int diff = PAttacker->GetMLevel() - PDefender->GetMLevel();
		if (PDefender->objtype == TYPE_MOB) {
			base = 5;
		}
		float check = base + diff;
		uint8 meritReduction = 0;
		if (PDefender->objtype == TYPE_PC) { //Check player's skill.
			//For mobs, we can assume their skill is capped at their level, so this term is 1 anyway.
			CCharEntity* PChar = (CCharEntity*)PDefender;
			float skill = PChar->GetSkill(PChar->PBattleAI->GetCurrentSpell()->getSkillType());
			if (skill <= 0) {
				skill = 1;
			}
			float cap = GetMaxSkill((SKILLTYPE)PChar->PBattleAI->GetCurrentSpell()->getSkillType(), PChar->GetMJob(), PChar->GetMLevel());
			//if cap is 0 then player is using a spell from their subjob
			if (cap == 0)
			{
				cap = GetMaxSkill((SKILLTYPE)PChar->PBattleAI->GetCurrentSpell()->getSkillType(), PChar->GetSJob(),
					PChar->GetSLevel()); // << this might be GetMLevel, however this leaves no chance of avoiding interuption
			}
			if (skill > cap)
				skill = cap;
			float ratio = (float)cap / skill;
			check *= ratio;
			//prevent from spilling over 100 - resulting in players never being interupted
			if (check > 100) check = 100;
			//apply any merit reduction
			meritReduction = ((CCharEntity*)PDefender)->PMeritPoints->GetMeritValue(MERIT_SPELL_INTERUPTION_RATE, (CCharEntity*)PDefender);
		}
		float aquaveil = ((float)((100.0f - (meritReduction + (float)PDefender->getMod(MOD_SPELLINTERRUPT))) / 100.0f));
		check *= aquaveil;
		uint8 chance = WELL512::irand() % 100;
		// caps, always give a 1% chance of interrupt
		if (check < 1) {
			check = 0;
		}
		if (chance < check)
		{
			//Interrupt the spell cast.
			return true;
		}
		return false;
	}
	/***********************************************************************
	Calculates the block rate of the defender
	Incorporates testing and data from:
	http://www.ffxiah.com/forum/topic/21671/paladin-faq-info-and-trade-studies/34/#2581818
	https://docs.google.com/spreadsheet/ccc?key=0AkX3maplDraRdFdCZHI2OU93aVgtWlZhN3ozZEtnakE#gid=0
	http://www.ffxionline.com/forums/paladin/55139-shield-data-size-2-vs-size-3-a.html
	Formula is:
	ShieldBaseRate + (DefenderShieldSkill - AttackerCombatSkill) * SkillModifier
	Skill Modifier appears to be 0.215 based on the data available.
	Base block rates are (small to large shield type) 55% -> 50% -> 45% -> 30%
	Aegis is a special case, having the base block rate of a size 2 type.
	************************************************************************/
	uint8 GetBlockRate(CBattleEntity* PAttacker, CBattleEntity* PDefender)
	{
		int8 shieldSize = 3;
		int8 base = 0;
		uint16 attackskill = PAttacker->GetSkill((SKILLTYPE)(PAttacker->m_Weapons[SLOT_MAIN]->getSkillType()));
		uint16 blockskill = PDefender->GetSkill(SKILL_SHL);
		if (PDefender->objtype == TYPE_PC)
		{
			CCharEntity* PChar = (CCharEntity*)PDefender;
			CItemArmor* PItem = (CItemArmor*)PChar->getEquip(SLOT_SUB);
			if (PItem)
				shieldSize = PItem->getShieldSize();
			else
				return 0;
		}
		else if (PDefender->objtype == TYPE_MOB && PDefender->GetMJob() == JOB_PLD)
		{
			CMobEntity* PMob = (CMobEntity*)PDefender;
			if (PMob->m_EcoSystem != SYSTEM_UNDEAD && PMob->m_EcoSystem != SYSTEM_BEASTMEN)
				return 0;
		}
		else
			return 0;
		switch (shieldSize)
		{
		case 1: // buckler
			base = 55;
			break;
		case 2: // round
		case 5: // aegis
			base = 50;
			break;
		case 3: // kite
			base = 45;
			break;
		case 4: // tower
			base = 30;
			break;
		case 6: // ochain
			base = 110;
			break;
		default:
			return 0;
		}
		float skillmodifier = (blockskill - attackskill) * 0.215f;
		return dsp_cap(base + (int8)skillmodifier, 5, (shieldSize == 6 ? 100 : 65));
	}
	uint8 GetParryRate(CBattleEntity* PAttacker, CBattleEntity* PDefender)
	{
		CItemWeapon* PWeapon = GetEntityWeapon(PDefender, SLOT_MAIN);
		if ((PWeapon != NULL && PWeapon->getID() != 0 && PWeapon->getID() != 65535 &&
			PWeapon->getSkillType() != SKILL_H2H) && battleutils::IsEngauged(PDefender))
		{
			JOBTYPE job = PDefender->GetMJob();
			if (job == JOB_NIN || job == JOB_SAM ||
				job == JOB_THF || job == JOB_BST || job == JOB_DRG ||
				job == JOB_PLD || job == JOB_WAR || job == JOB_BRD ||
				job == JOB_DRK || job == JOB_RDM || job == JOB_COR ||
				job == JOB_DNC || job == JOB_PUP || job == JOB_RUN ||
				job == JOB_BLU || job == JOB_MNK || job == JOB_GEO ||
				job == JOB_SCH)
			{
				// http://wiki.ffxiclopedia.org/wiki/Talk:Parrying_Skill
				// {(Parry Skill x .125) + ([Player Agi - Enemy Dex] x .125)} x Diff
				float skill = PDefender->GetSkill(SKILL_PAR) + PDefender->getMod(MOD_PARRY);
				float diff = 1.0f + (((float)PDefender->GetMLevel() - PAttacker->GetMLevel()) / 15.0f);
				if (PWeapon->isTwoHanded())
				{
					// two handed weapons get a bonus
					diff += 0.1f;
				}
				if (diff < 0.4f) diff = 0.4f;
				if (diff > 1.4f) diff = 1.4f;
				float dex = PAttacker->DEX();
				float agi = PDefender->AGI();
				uint8 parryRate = dsp_cap((skill * 0.1f + (agi - dex) * 0.125f + 10.0f) * diff, 5, 25);
				// Issekigan grants parry rate bonus. From best available data, if you already capped out at 25% parry it grants another 25% bonus for ~50% parry rate
				if (PDefender->objtype == TYPE_PC && PDefender->StatusEffectContainer->HasStatusEffect(EFFECT_ISSEKIGAN)){
					int16 issekiganBonus = PDefender->StatusEffectContainer->GetStatusEffect(EFFECT_ISSEKIGAN)->GetPower();
					//ShowDebug(CL_CYAN"GetParryRate: Issekigan Active, Parry Rate %d -> %d...\n" CL_RESET, parryRate, (parryRate+issekiganBonus));
					parryRate = parryRate + issekiganBonus;
				}
				return parryRate;
			}
		}
		return 0;
	}
	uint8 GetGuardRate(CBattleEntity* PAttacker, CBattleEntity* PDefender)
	{
		CItemWeapon* PWeapon = GetEntityWeapon(PDefender, SLOT_MAIN);
		// Defender must have no weapon equipped, or a hand to hand weapon equipped to guard
		bool validWeapon = (PWeapon == NULL || PWeapon->getSkillType() == SKILL_H2H);
		if (PDefender->objtype == TYPE_MOB || PDefender->objtype == TYPE_PET){
			validWeapon = PDefender->GetMJob() == JOB_MNK || PDefender->GetMJob() == JOB_PUP;
		}
		if (validWeapon && battleutils::IsEngauged(PDefender))
		{
			// assuming this is like parry
			float skill = PDefender->GetSkill(SKILL_GRD) + PDefender->getMod(MOD_GUARD);
			float diff = 1.0f + (((float)PDefender->GetMLevel() - PAttacker->GetMLevel()) / 15.0f);
			if (diff < 0.4f) diff = 0.4f;
			if (diff > 1.4f) diff = 1.4f;
			float dex = PAttacker->DEX();
			float agi = PDefender->AGI();
			return dsp_cap((skill * 0.1f + (agi - dex) * 0.125f + 10.0f) * diff, 5, 25);
		}
		return 0;
	}
	/************************************************************************
	* *
	* Calculates damage based on damage and resistance to damage type *
	* *
	************************************************************************/
	uint32 TakePhysicalDamage(CBattleEntity* PAttacker, CBattleEntity* PDefender, int32 damage, bool isBlocked, uint8 slot, uint16 tpMultiplier, CBattleEntity* taChar, bool giveTPtoVictim)
	{
		bool isRanged = (slot == SLOT_AMMO || slot == SLOT_RANGED);
		if (PAttacker->StatusEffectContainer->HasStatusEffect(EFFECT_FORMLESS_STRIKES))
		{
			uint8 formlessMod = 70;
			if (PAttacker->objtype == TYPE_PC)
				formlessMod += ((CCharEntity*)PAttacker)->PMeritPoints->GetMeritValue(MERIT_FORMLESS_STRIKES, (CCharEntity*)PAttacker);
			damage = damage * formlessMod / 100;
			// TODO: chance to 'resist'
			damage = MagicDmgTaken(PDefender, damage);
		}
		else
		{
			if (isRanged)
				damage = RangedDmgTaken(PDefender, damage);
			else
				damage = PhysicalDmgTaken(PDefender, damage);
			switch (PAttacker->m_Weapons[slot]->getDmgType())
			{
			case DAMAGE_CROSSBOW:
			case DAMAGE_GUN:
			case DAMAGE_PIERCING: damage = (damage * (PDefender->getMod(MOD_PIERCERES))) / 1000; break;
			case DAMAGE_SLASHING: damage = (damage * (PDefender->getMod(MOD_SLASHRES))) / 1000; break;
			case DAMAGE_IMPACT:	damage = (damage * (PDefender->getMod(MOD_IMPACTRES))) / 1000; break;
			case DAMAGE_HTH:	damage = (damage * (PDefender->getMod(MOD_HTHRES))) / 1000; break;
			}
			if (isBlocked)
			{
				uint8 absorb = 100;
				if (PDefender->objtype == TYPE_PC)
				{
					if (PDefender->m_Weapons[SLOT_SUB]->IsShield())
					{
						absorb = 100 - PDefender->m_Weapons[SLOT_SUB]->getShieldAbsorption();
						if ((dsp_max(damage - (PDefender->getMod(MOD_PHALANX) + PDefender->getMod(MOD_STONESKIN)), 0) >0)
							&& (charutils::hasTrait((CCharEntity*)PDefender, TRAIT_SHIELD_MASTERY)))
						{
							// If the player blocked with a shield and has shield mastery, add shield mastery TP bonus
							// unblocked damage (before block but as if affected by stoneskin/phalanx) must be greater than zero
							PDefender->addTP(PDefender->getMod(MOD_SHIELD_MASTERY_TP));
						}
					}
				}
				else
				{
					absorb = 50;
				}
				damage = (damage * absorb) / 100;
			}
		}
		if (damage > 0)
		{
			damage = dsp_max(damage - PDefender->getMod(MOD_PHALANX), 0);
			damage = HandleStoneskin(PDefender, damage);
			HandleAfflatusMiseryDamage(PDefender, damage);
		}
		damage = dsp_cap(damage, -99999, 99999);
		attackutils::TryAbsorbMPfromPhysicalAttack(PDefender, damage);
		int32 corrected = PDefender->addHP(-damage);
		if (damage < 0)
			damage = corrected;
		if (PAttacker->PMaster != NULL)
		{
			PDefender->m_OwnerID.id = PAttacker->PMaster->id;
			PDefender->m_OwnerID.targid = PAttacker->PMaster->targid;
		}
		else
		{
			if (PAttacker->objtype == TYPE_MOB && PAttacker->PMaster == NULL)
			{
				//uncharmed mob still attacking another mob - dont allow 2 mobs to go purple
			}
			else
			{
				PDefender->m_OwnerID.id = PAttacker->id;
				PDefender->m_OwnerID.targid = PAttacker->targid;
			}
		}
		if (damage > 0)
		{
			PDefender->StatusEffectContainer->DelStatusEffectsByFlag(EFFECTFLAG_DAMAGE);
			//40% chance to break bind when dmg received
			if (PDefender->StatusEffectContainer->HasStatusEffect(EFFECT_BIND) && WELL512::irand() % 100 < 40)
				PDefender->StatusEffectContainer->DelStatusEffect(EFFECT_BIND);
			switch (PDefender->objtype)
			{
			case TYPE_MOB:
				if (taChar == NULL)
					((CMobEntity*)PDefender)->PEnmityContainer->UpdateEnmityFromDamage(PAttacker, damage);
				else
					((CMobEntity*)PDefender)->PEnmityContainer->UpdateEnmityFromDamage(taChar, damage);
				//if the mob is charmed by player
				if (PDefender->PMaster != NULL && PDefender->PMaster->objtype == TYPE_PC)
					((CPetEntity*)PDefender)->loc.zone->PushPacket(PDefender, CHAR_INRANGE, new CEntityUpdatePacket(PDefender, ENTITY_UPDATE, UPDATE_COMBAT));
				break;
			case TYPE_PET:
				((CPetEntity*)PDefender)->loc.zone->PushPacket(PDefender, CHAR_INRANGE, new CEntityUpdatePacket(PDefender, ENTITY_UPDATE, UPDATE_COMBAT));
				break;
			}
			//battleutils::MakeEntityStandUp(PDefender); Removed: addHP() is already making victim stand if dmg > 0
			// try to interrupt spell if not a ranged attack and not blocked by Shield Mastery
			if (PDefender->PBattleAI->m_PMagicState != NULL)
			{
				if ((!isRanged)
					&& !((isBlocked) && (PDefender->objtype == TYPE_PC) && (charutils::hasTrait((CCharEntity*)PDefender, TRAIT_SHIELD_MASTERY))))
				{
					PDefender->PBattleAI->m_PMagicState->TryHitInterrupt(PAttacker);
				}
			}
			else
			{
				ShowError("battleutils::TakePhysicalDamage Entity (%d) has no magic state\n", PDefender->id);
			}
			int16 baseTp = 0;
			if ((slot == SLOT_RANGED || slot == SLOT_AMMO) && PAttacker->objtype == TYPE_PC)
			{
				CCharEntity* PChar = (CCharEntity*)PAttacker;
				CItemWeapon* PAmmo = (CItemWeapon*)PChar->getEquip(SLOT_AMMO);
				int16 delay = PAttacker->GetRangedWeaponDelay(true);
				baseTp = CalculateBaseTP((delay * 110) / 1000);
			}
			else
			{
				int16 delay = PAttacker->GetWeaponDelay(true);
				if (PAttacker->m_Weapons[SLOT_SUB]->getDmgType() > 0 &&
					PAttacker->m_Weapons[SLOT_SUB]->getDmgType() < 4 &&
					PAttacker->m_Weapons[slot]->getDmgType() != DAMAGE_HTH)
				{
					delay = delay / 2;
				}
				float ratio = 1.0f;
				if (PAttacker->m_Weapons[slot]->getDmgType() == DAMAGE_HTH)
				{
					ratio = 2.0f;
				}
				baseTp = CalculateBaseTP((delay * 60) / 1000) / ratio;
			}
			// add to to attacker
			PAttacker->addTP(tpMultiplier * (baseTp * (1.0f + 0.01f * (float)((PAttacker->getMod(MOD_STORETP) + getStoreTPbonusFromMerit(PAttacker))))));
			if (giveTPtoVictim == true)
			{
				//account for attacker's subtle blow which reduces the baseTP gain for the defender
				float sBlowMult = ((100.0f - dsp_cap((float)PAttacker->getMod(MOD_SUBTLE_BLOW), 0.0f, 50.0f)) / 100.0f);
				//mobs hit get basetp+30 whereas pcs hit get basetp/3
				if (PDefender->objtype == TYPE_PC)
				{
					//yup store tp counts on hits taken too!
					PDefender->addTP((baseTp / 3) * sBlowMult * (1.0f + 0.01f * (float)((PDefender->getMod(MOD_STORETP) + getStoreTPbonusFromMerit(PAttacker)))));
				}
				else
				{
					//subtle blow also reduces the "+30" on mob tp gain
					PDefender->addTP((baseTp + 30) * sBlowMult * (1.0f + 0.01f * (float)PDefender->getMod(MOD_STORETP)));
				}
			}
			if (PAttacker->objtype == TYPE_PC)
				charutils::UpdateHealth((CCharEntity*)PAttacker);
		}
		else {
			if (PDefender->objtype == TYPE_MOB){
				((CMobEntity*)PDefender)->PEnmityContainer->UpdateEnmityFromDamage(PAttacker, 0);
			}
		}
		if (PDefender->objtype == TYPE_PC)
			charutils::UpdateHealth((CCharEntity*)PDefender);
		if (PAttacker->objtype == TYPE_PC)
		{
			if (!isRanged)
			{
				// only for physical attacks
				PAttacker->StatusEffectContainer->DelStatusEffectsByFlag(EFFECTFLAG_ATTACK);
			}
		}
		if (damage < 0)
			return -damage;
		else
			return damage;
	}
	/************************************************************************
	* *
	* Calculate Probability attack will hit (20% min cap - 95% max cap) *
	* attackNumber: 0=main, 1=sub, 2=kick *
	* *
	************************************************************************/
	uint8 GetHitRateEx(CBattleEntity* PAttacker, CBattleEntity* PDefender, uint8 attackNumber, uint8 offsetAccuracy) //subWeaponAttack is for calculating acc of dual wielded sub weapon
	{
		int32 hitrate = 75;
		if (PAttacker->objtype == TYPE_PC && ((PAttacker->StatusEffectContainer->HasStatusEffect(EFFECT_SNEAK_ATTACK) && (abs(PDefender->loc.p.rotation - PAttacker->loc.p.rotation) < 23 || PAttacker->StatusEffectContainer->HasStatusEffect(EFFECT_HIDE))) ||
			(charutils::hasTrait((CCharEntity*)PAttacker, TRAIT_ASSASSIN) && PAttacker->StatusEffectContainer->HasStatusEffect(EFFECT_TRICK_ATTACK) && battleutils::getAvailableTrickAttackChar(PAttacker, PDefender))))
		{
			hitrate = 100; //attack with SA active or TA/Assassin cannot miss
		}
		else
		{
			//Check For Ambush Merit - Melee
			if (PAttacker->objtype == TYPE_PC && (charutils::hasTrait((CCharEntity*)PAttacker, TRAIT_AMBUSH)) && ((abs(PDefender->loc.p.rotation - PAttacker->loc.p.rotation) < 23))) {
				offsetAccuracy += ((CCharEntity*)PAttacker)->PMeritPoints->GetMeritValue(MERIT_AMBUSH, (CCharEntity*)PAttacker);
			}
			hitrate = hitrate + (PAttacker->ACC(attackNumber, offsetAccuracy) - PDefender->EVA()) / 2 + (PAttacker->GetMLevel() - PDefender->GetMLevel()) * 2;
			hitrate = dsp_cap(hitrate, 20, 95);
		}
		return (uint8)hitrate;
	}
	uint8 GetHitRate(CBattleEntity* PAttacker, CBattleEntity* PDefender)
	{
		return GetHitRateEx(PAttacker, PDefender, 0, 0); //assume attack 0(main)
	}
	uint8 GetHitRate(CBattleEntity* PAttacker, CBattleEntity* PDefender, uint8 attackNumber)
	{
		return GetHitRateEx(PAttacker, PDefender, attackNumber, 0);
	}
	uint8 GetHitRate(CBattleEntity* PAttacker, CBattleEntity* PDefender, uint8 attackNumber, uint8 offsetAccuracy)
	{
		return GetHitRateEx(PAttacker, PDefender, attackNumber, offsetAccuracy);
	}
	/************************************************************************
	* *
	* Crit Rate *
	* *
	************************************************************************/
	uint8 GetCritHitRate(CBattleEntity* PAttacker, CBattleEntity* PDefender, bool ignoreSneakTrickAttack)
	{
		int32 crithitrate = 5;
		if (PAttacker->StatusEffectContainer->HasStatusEffect(EFFECT_MIGHTY_STRIKES, 0) ||
			PAttacker->StatusEffectContainer->HasStatusEffect(EFFECT_MIGHTY_STRIKES)){
			return 100;
		}
		else if (PAttacker->objtype == TYPE_PC && (!ignoreSneakTrickAttack) && PAttacker->StatusEffectContainer->HasStatusEffect(EFFECT_SNEAK_ATTACK))
		{
			if (abs(PDefender->loc.p.rotation - PAttacker->loc.p.rotation) < 23 || PAttacker->StatusEffectContainer->HasStatusEffect(EFFECT_HIDE))
			{
				crithitrate = 100;
			}
		}
		else if (PAttacker->objtype == TYPE_PC && PAttacker->GetMJob() == JOB_THF && charutils::hasTrait((CCharEntity*)PAttacker, TRAIT_ASSASSIN) && (!ignoreSneakTrickAttack) &&
			PAttacker->StatusEffectContainer->HasStatusEffect(EFFECT_TRICK_ATTACK))
		{
			CBattleEntity* taChar = battleutils::getAvailableTrickAttackChar(PAttacker, PDefender);
			if (taChar != NULL) crithitrate = 100;
		}
		else
		{
			//apply merit mods
			if (PAttacker->objtype == TYPE_PC) crithitrate += ((CCharEntity*)PAttacker)->PMeritPoints->GetMeritValue(MERIT_CRIT_HIT_RATE, (CCharEntity*)PAttacker);
			if (PDefender->objtype == TYPE_PC) crithitrate -= ((CCharEntity*)PDefender)->PMeritPoints->GetMeritValue(MERIT_ENEMY_CRIT_RATE, (CCharEntity*)PDefender);
			int32 attackerdex = PAttacker->DEX();
			int32 defenderagi = PDefender->AGI();
			int32 dDEX = dsp_cap(attackerdex - defenderagi, 0, 50);
			crithitrate += (dDEX * 30) / 100 + PAttacker->getMod(MOD_CRITHITRATE) + PDefender->getMod(MOD_ENEMYCRITRATE);
			crithitrate = dsp_cap(crithitrate, 0, 100);
		}
		return (uint8)crithitrate;
	}
	/************************************************************************
	* *
	* Formula for calculating damage ratio *
	* *
	************************************************************************/
	float GetDamageRatio(CBattleEntity* PAttacker, CBattleEntity* PDefender, bool isCritical, uint16 bonusAttPercent)
	{
		// used to apply a % of attack bonus
		float attPercentBonus = 0;
		if (bonusAttPercent >= 1)
			attPercentBonus = (float)(PAttacker->ATT() * bonusAttPercent / 100);
		//wholly possible for DEF to be near 0 with the amount of debuffs/effects now.
		float ratio = (float)(PAttacker->ATT() + attPercentBonus) / (float)((PDefender->DEF() == 0) ? 1 : PDefender->DEF());
		float cRatioMax = 0;
		float cRatioMin = 0;
		float ratioCap = 2.0f;
		if (PAttacker->objtype == TYPE_PC)
		{
			ratioCap = 2.25f;
		}
		ratio = dsp_cap(ratio, 0, ratioCap);
		float cRatio = ratio;
		if (PAttacker->objtype == TYPE_PC)
		{
			if (PAttacker->GetMLevel() < PDefender->GetMLevel())
			{
				cRatio -= 0.050f * (PDefender->GetMLevel() - PAttacker->GetMLevel());
			}
		}
		else
		{
			if (PAttacker->GetMLevel() > PDefender->GetMLevel())
			{
				cRatio += 0.050f * (PAttacker->GetMLevel() - PDefender->GetMLevel());
			}
		}
		if (isCritical){
			cRatio += 1;
		}
		cRatio = dsp_cap(cRatio, 0, 4);
		if ((0 <= cRatio) && (cRatio < 0.5)) {
			cRatioMax = cRatio + 1;
		}
		else if ((0.5 <= cRatio) && (cRatio <= 0.7)) {
			cRatioMax = 1;
		}
		else if ((0.7 < cRatio) && (cRatio <= 1.2)) {
			cRatioMax = cRatio + 0.3;
		}
		else if ((1.2 < cRatio) && (cRatio <= 1.5)) {
			cRatioMax = (cRatio * 0.25f) + cRatio;
		}
		else if ((1.5 < cRatio) && (cRatio <= 2.625)) {
			cRatioMax = cRatio + 0.375;
		}
		else if ((2.625 < cRatio) && (cRatio <= 3.25)) {
			cRatioMax = 3;
		}
		else {
			cRatioMax = cRatio;
		}
		if ((0 <= cRatio) && (cRatio < 0.38)) {
			cRatioMin = 0;
		}
		else if ((0.38 <= cRatio) && (cRatio <= 1.25)) {
			cRatioMin = cRatio * (float)(1176 / 1024) - (float)(448 / 1024);
		}
		else if ((1.25 < cRatio) && (cRatio <= 1.51)) {
			cRatioMin = 1;
		}
		else if ((1.51 < cRatio) && (cRatio <= 2.44)) {
			cRatioMin = cRatio * (float)(1176 / 1024) - (float)(775 / 1024);
		}
		else {
			cRatioMin = cRatio - 0.375;
		}
		float pDIF = ((cRatioMax - cRatioMin) * WELL512::drand()) + cRatioMin;
		if (isCritical)
		{
			int16 criticaldamage = PAttacker->getMod(MOD_CRIT_DMG_INCREASE);
			criticaldamage = dsp_cap(criticaldamage, 0, 100);
			pDIF *= ((100 + criticaldamage) / 100.0f);
		}
		//x1.00 ~ x1.05 final multiplier, giving max value 3*1.05 -> 3.15
		return pDIF * (1 + ((0.05f) * WELL512::drand()));
	}
	/************************************************************************
	* Formula for Strength *
	************************************************************************/
	int32 GetFSTR(CBattleEntity* PAttacker, CBattleEntity* PDefender, uint8 SlotID)
	{
		int32 rank = 0;
		int32 fstr = 0;
		float dif = PAttacker->STR() - PDefender->VIT();
		if (dif >= 12) {
			fstr = (dif + 4) / 2;
		}
		else if (dif >= 6){
			fstr = (dif + 6) / 2;
		}
		else if (dif >= 1){
			fstr = (dif + 7) / 2;
		}
		else if (dif >= -2){
			fstr = (dif + 8) / 2;
		}
		else if (dif >= -7){
			fstr = (dif + 9) / 2;
		}
		else if (dif >= -15){
			fstr = (dif + 10) / 2;
		}
		else if (dif >= -21){
			fstr = (dif + 12) / 2;
		}
		else {
			fstr = (dif + 13) / 2;
		}
		if (SlotID == SLOT_RANGED)
		{
			rank = PAttacker->GetRangedWeaponRank();
			//different caps than melee weapons
			if (fstr <= (-rank * 2))
				return (-rank * 2);
			if ((fstr > (-rank * 2)) && (fstr <= (2 * (rank + 8))))
				return fstr;
			else
				return 2 * (rank + 8);
		}
		else
		{
			fstr /= 2;
			if (SlotID == SLOT_MAIN)
			{
				rank = PAttacker->GetMainWeaponRank();
			}
			else if (SlotID == SLOT_SUB)
			{
				rank = PAttacker->GetSubWeaponRank();
			}
			// everything else
			if (fstr <= (-rank))
				return (-rank);
			if ((fstr > (-rank)) && (fstr <= rank + 8))
				return fstr;
			else
				return rank + 8;
		}
	}
	/************************************************************************
	* *
	* Multihit calculator *
	* *
	************************************************************************/
	uint8 getHitCount(uint8 hits)
	{
		uint8 distribution = WELL512::irand() % 100;
		uint8 num = 1;
		switch (hits)
		{
		case 0: break;
		case 1: break;
		case 2: // cdf = 55,100
			if (distribution < 55){ break; }
			else{ num += 1; break; }
			break;
		case 3: // cdf = 30,80,100
			if (distribution < 30){ break; }
			else if (distribution < 80){ num += 1; break; }
			else{ num += 2; break; }
			break;
		case 4: // cdf = 20,50,80,100
			if (distribution < 20){ break; }
			else if (distribution < 50){ num += 1; break; }
			else if (distribution < 80){ num += 2; break; }
			else{ num += 3; break; }
			break;
		case 5: // cdf = 10,30,60,90,100
			if (distribution < 10){ break; }
			else if (distribution < 30){ num += 1; break; }
			else if (distribution < 60){ num += 2; break; }
			else if (distribution < 90){ num += 3; break; }
			else{ num += 4; break; }
			break;
		case 6: // cdf = 10,30,50,70,90,100
			if (distribution < 10){ break; }
			else if (distribution < 30){ num += 1; break; }
			else if (distribution < 50){ num += 2; break; }
			else if (distribution < 70){ num += 3; break; }
			else if (distribution < 90){ num += 4; break; }
			else{ num += 5; break; }
			break;
		case 7: // cdf = 5,20,45,70,85,95,100
			if (distribution < 5){ break; }
			else if (distribution < 20){ num += 1; break; }
			else if (distribution < 45){ num += 2; break; }
			else if (distribution < 70){ num += 3; break; }
			else if (distribution < 85){ num += 4; break; }
			else if (distribution < 95){ num += 5; break; }
			else{ num += 6; break; }
			break;
		case 8: // cdf = 5,20,45,70,85,95,98,100
			if (distribution < 5){ break; }
			else if (distribution < 20){ num += 1; break; }
			else if (distribution < 45){ num += 2; break; }
			else if (distribution < 70){ num += 3; break; }
			else if (distribution < 85){ num += 4; break; }
			else if (distribution < 95){ num += 5; break; }
			else if (distribution < 98){ num += 6; break; }
			else{ num += 7; break; }
			break;
		}
		return dsp_min(num, 8); // не более восьми ударов за одну атаку
	}
	/************************************************************************
	* *
	* Returns a mob / pets multihits *
	* *
	************************************************************************/
	uint8 CheckMobMultiHits(CBattleEntity* PEntity)
	{
		if (PEntity->objtype == TYPE_MOB || PEntity->objtype == TYPE_PET)
		{
			uint8 num = 1;
			//Monk
			if (PEntity->GetMJob() == JOB_MNK)
			{
				num = 2;
			}
			//check for unique mobs
			switch (PEntity->id)
			{
			case 17498522:// Charybdis 2-6
				return (1 + getHitCount(5));
			default:
				break;
			}
			int8 tripleAttack = PEntity->getMod(MOD_TRIPLE_ATTACK);
			int8 doubleAttack = PEntity->getMod(MOD_DOUBLE_ATTACK);
			doubleAttack = dsp_cap(doubleAttack, 0, 100);
			tripleAttack = dsp_cap(tripleAttack, 0, 100);
			if (WELL512::irand() % 100 < tripleAttack)
			{
				num += 2;
			}
			else if (WELL512::irand() % 100 < doubleAttack)
			{
				num += 1;
			}
			return num;
		}
		return 0;
	}
	/************************************************************************
	* *
	* Returns the number of hits for multihit weapons if applicable *
	* (Keeping this for backwards compatibility with the old system) *
	************************************************************************/
	uint8 CheckMultiHits(CBattleEntity* PEntity, CItemWeapon* PWeapon)
	{
		//checking players weapon hit count
		uint8 num = PWeapon->getHitCount();
		int8 tripleAttack = PEntity->getMod(MOD_TRIPLE_ATTACK);
		int8 doubleAttack = PEntity->getMod(MOD_DOUBLE_ATTACK);
		//check for merit upgrades
		if (PEntity->objtype == TYPE_PC)
		{
			CCharEntity* PChar = (CCharEntity*)PEntity;
			//merit chance only applies if player has the job trait
			if (charutils::hasTrait(PChar, TRAIT_TRIPLE_ATTACK)) {
				tripleAttack += PChar->PMeritPoints->GetMeritValue(MERIT_TRIPLE_ATTACK_RATE, (CCharEntity*)PEntity);
			}
			if (charutils::hasTrait(PChar, TRAIT_DOUBLE_ATTACK)) {
				doubleAttack += PChar->PMeritPoints->GetMeritValue(MERIT_DOUBLE_ATTACK_RATE, (CCharEntity*)PEntity);
			}
		}
		doubleAttack = dsp_cap(doubleAttack, 0, 100);
		tripleAttack = dsp_cap(tripleAttack, 0, 100);
		if (WELL512::irand() % 100 < tripleAttack)
		{
			num += 2;
		}
		else if (WELL512::irand() % 100 < doubleAttack)
		{
			num += 1;
		}
		// hasso occasionally triggers Zanshin after landing a normal attack, only active while Samurai is set as Main
		if (PEntity->GetMJob() == JOB_SAM)
		{
			if (PEntity->StatusEffectContainer->HasStatusEffect(EFFECT_HASSO))
			{
				uint8 zanshin = PEntity->getMod(MOD_ZANSHIN);
				if (PEntity->objtype == TYPE_PC)
					zanshin += ((CCharEntity*)PEntity)->PMeritPoints->GetMeritValue(MERIT_ZASHIN_ATTACK_RATE, (CCharEntity*)PEntity);
				if (WELL512::irand() % 100 < (zanshin / 4))
					num++;
			}
		}
		return dsp_min(num, 8);
	}
	/************************************************************************
	* *
	* Chance paralysis will cause you to be paralyzed *
	* *
	************************************************************************/
	bool IsParalyzed(CBattleEntity* PAttacker)
	{
		return (WELL512::irand() % 100 < dsp_cap(PAttacker->getMod(MOD_PARALYZE) - PAttacker->getMod(MOD_PARALYZERES), 0, 100));
	}
	/*****************************************************************************
	Returns true if the Third Eye anticipates the attacks. Must specify various
	parameters including if the effect should 100% be removed (e.g. in the case of AoE)
	by setting forceRemove to true. Must also specify the ignore boolean, which is true
	to ignore the effects of Third Eye (but NOT try to remove).
	******************************************************************************/
	bool IsAnticipated(CBattleEntity* PDefender, bool forceRemove, bool ignore)
	{
		if (ignore){
			return false;
		}
		if (PDefender->GetMJob() != JOB_SAM && PDefender->GetSJob() != JOB_SAM){
			//faster check than via hasStatusEffect
			return false;
		}
		if (!PDefender->StatusEffectContainer->HasStatusEffect(EFFECT_THIRD_EYE)){
			return false;
		}
		CStatusEffect* effect = PDefender->StatusEffectContainer->GetStatusEffect(EFFECT_THIRD_EYE, 0);
		if (effect == NULL) { //shouldn't occur but checking anyway
			return false;
		}
		if (forceRemove){
			PDefender->StatusEffectContainer->DelStatusEffect(EFFECT_THIRD_EYE);
			return false;
		}
		//power stores how many times this effect has anticipated
		uint8 pastAnticipations = effect->GetPower();
		if (pastAnticipations>7){
			//max 7 anticipates!
			PDefender->StatusEffectContainer->DelStatusEffect(EFFECT_THIRD_EYE);
			return false;
		}
		bool hasSeigan = PDefender->StatusEffectContainer->HasStatusEffect(EFFECT_SEIGAN, 0);
		if (!hasSeigan && pastAnticipations == 0){
			PDefender->StatusEffectContainer->DelStatusEffect(EFFECT_THIRD_EYE);
			return true;
		}
		else if (!hasSeigan){
			PDefender->StatusEffectContainer->DelStatusEffect(EFFECT_THIRD_EYE);
			return false;
		}
		else{ //do have seigan, decay anticipations correctly (guesstimated)
			//5-6 anticipates is a 'lucky' streak, going to assume 15% decay per proc, with a 100% base w/ Seigan
			if (WELL512::irand() % 100 < (100 - (pastAnticipations * 15))){
				//increment power and don't remove
				effect->SetPower(effect->GetPower() + 1);
				return true;
			}
			PDefender->StatusEffectContainer->DelStatusEffect(EFFECT_THIRD_EYE);
			return false;
		}
		return false;
	}
	/************************************************************************
	* *
	* *
	* *
	************************************************************************/
	bool IsAbsorbByShadow(CBattleEntity* PDefender)
	{
		//utsus always overwrites blink, so if utsus>0 then we know theres no blink.
		uint16 Shadow = PDefender->getMod(MOD_UTSUSEMI);
		uint16 modShadow = MOD_UTSUSEMI;
		if (Shadow == 0){
			Shadow = PDefender->getMod(MOD_BLINK);
			modShadow = MOD_BLINK;
			//random chance, assume 80% proc
			if (WELL512::irand() % 100 < 20){
				return false;
			}
		}
		if (Shadow > 0)
		{
			PDefender->setModifier(modShadow, --Shadow);
			if (Shadow == 0)
			{
				switch (modShadow){
				case MOD_UTSUSEMI:
					PDefender->StatusEffectContainer->DelStatusEffect(EFFECT_COPY_IMAGE);
					break;
				case MOD_BLINK:
					PDefender->StatusEffectContainer->DelStatusEffect(EFFECT_BLINK);
					break;
				}
			}
			else if (Shadow < 4 && MOD_UTSUSEMI == modShadow)
			{
				if (PDefender->objtype == TYPE_PC)
				{
					CStatusEffect* PStatusEffect = PDefender->StatusEffectContainer->GetStatusEffect(EFFECT_COPY_IMAGE, 0);
					if (PStatusEffect != NULL)
					{
						uint16 icon = EFFECT_COPY_IMAGE_3;
						switch (PDefender->getMod(MOD_UTSUSEMI))
						{
						case 1: icon = EFFECT_COPY_IMAGE_1; break;
						case 2: icon = EFFECT_COPY_IMAGE_2; break;
						}
						PStatusEffect->SetIcon(icon);
						PDefender->StatusEffectContainer->UpdateStatusIcons();
					}
				}
			}
			return true;
		}
		return false;
	}
	/************************************************************************
	* *
	* Intimidation from Killer Effects (chance to intimidate) *
	* *
	************************************************************************/
	bool IsIntimidated(CBattleEntity* PAttacker, CBattleEntity* PDefender)
	{
		// cannot intimidate yourself!
		if (PAttacker == PDefender) return false;
		int16 KillerEffect = 0;
		switch (PAttacker->m_EcoSystem)
		{
		case SYSTEM_AMORPH:	KillerEffect = PDefender->getMod(MOD_AMORPH_KILLER); break;
		case SYSTEM_AQUAN:	KillerEffect = PDefender->getMod(MOD_AQUAN_KILLER); break;
		case SYSTEM_ARCANA:	KillerEffect = PDefender->getMod(MOD_ARCANA_KILLER); break;
		case SYSTEM_BEAST:	KillerEffect = PDefender->getMod(MOD_BEAST_KILLER); break;
		case SYSTEM_BIRD:	KillerEffect = PDefender->getMod(MOD_BIRD_KILLER); break;
		case SYSTEM_DEMON:	KillerEffect = PDefender->getMod(MOD_DEMON_KILLER); break;
		case SYSTEM_DRAGON:	KillerEffect = PDefender->getMod(MOD_DRAGON_KILLER); break;
		case SYSTEM_EMPTY:	KillerEffect = PDefender->getMod(MOD_EMPTY_KILLER); break;
		case SYSTEM_HUMANOID:	KillerEffect = PDefender->getMod(MOD_HUMANOID_KILLER); break;
		case SYSTEM_LIZARD:	KillerEffect = PDefender->getMod(MOD_LIZARD_KILLER); break;
		case SYSTEM_LUMINION: KillerEffect = PDefender->getMod(MOD_LUMINION_KILLER); break;
		case SYSTEM_LUMORIAN: KillerEffect = PDefender->getMod(MOD_LUMORIAN_KILLER); break;
		case SYSTEM_PLANTOID:	KillerEffect = PDefender->getMod(MOD_PLANTOID_KILLER); break;
		case SYSTEM_UNDEAD:	KillerEffect = PDefender->getMod(MOD_UNDEAD_KILLER); break;
		case SYSTEM_VERMIN:	KillerEffect = PDefender->getMod(MOD_VERMIN_KILLER); break;
		}
		return (WELL512::irand() % 100 < KillerEffect);
	}
	/****************************************************************
	* Determine if an enfeeble spell will land - untested *
	****************************************************************/
	bool EnfeebleHit(CBattleEntity* PCaster, CBattleEntity* PDefender, EFFECT Effect)
	{
		int16 dlvl = (PCaster->GetMLevel() - PDefender->GetMLevel());
		int16 maxCap = 90;
		int16 minCap = 10;
		int16 chance = 40 + (dlvl * 5);
		chance = (chance > maxCap ? maxCap : chance);
		chance = (chance < minCap ? minCap : chance);
		if (Effect > 1 && Effect < 15)
		{
			chance = chance + (PDefender->getMod((MODIFIER)(Effect + 238)) / 10);
		}
		if (WELL512::irand() % 100 < chance)
		{
			return true;
		}
		return false;
	}
	/************************************************************************
	* *
	* Gets SkillChain Effect *
	* *
	************************************************************************/
#define PAIR(x, y) ((x << 8) + y)
	uint8 GetSkillchainSubeffect(SKILLCHAIN_ELEMENT skillchain)
	{
		DSP_DEBUG_BREAK_IF(skillchain < SC_NONE || skillchain > SC_DARKNESS_II);
		static const uint8 effects[] = {
			SUBEFFECT_NONE, // SC_NONE
			SUBEFFECT_TRANSFIXION, // SC_TRANSFIXION
			SUBEFFECT_COMPRESSION, // SC_COMPRESSION
			SUBEFFECT_LIQUEFACATION, // SC_LIQUEFACTION
			SUBEFFECT_SCISSION, // SC_SCISSION
			SUBEFFECT_REVERBERATION, // SC_REVERBERATION
			SUBEFFECT_DETONATION, // SC_DETONATION
			SUBEFFECT_INDURATION, // SC_INDURATION
			SUBEFFECT_IMPACTION, // SC_IMPACTION
			SUBEFFECT_GRAVITATION, // SC_GRAVITATION
			SUBEFFECT_DISTORTION, // SC_DISTORTION
			SUBEFFECT_FUSION, // SC_FUSION
			SUBEFFECT_FRAGMENTATION, // SC_FRAGMENTATION
			SUBEFFECT_LIGHT, // SC_LIGHT
			SUBEFFECT_DARKNESS, // SC_DARKNESS
			SUBEFFECT_LIGHT, // SC_LIGHT_II
			SUBEFFECT_DARKNESS, // SC_DARKNESS_II
		};
		return effects[skillchain];
	}
	uint8 GetSkillchainTier(SKILLCHAIN_ELEMENT skillchain)
	{
		DSP_DEBUG_BREAK_IF(skillchain < SC_NONE || skillchain > SC_DARKNESS_II);
		static const uint8 tiers[] = {
			0, // SC_NONE
			1, // SC_TRANSFIXION
			1, // SC_COMPRESSION
			1, // SC_LIQUEFACTION
			1, // SC_SCISSION
			1, // SC_REVERBERATION
			1, // SC_DETONATION
			1, // SC_INDURATION
			1, // SC_IMPACTION
			2, // SC_GRAVITATION
			2, // SC_DISTORTION
			2, // SC_FUSION
			2, // SC_FRAGMENTATION
			3, // SC_LIGHT
			3, // SC_DARKNESS
			4, // SC_LIGHT_II
			4, // SC_DARKNESS_II
		};
		return tiers[skillchain];
	}
	SKILLCHAIN_ELEMENT FormSkillchain(std::list<SKILLCHAIN_ELEMENT> resonance, std::list<SKILLCHAIN_ELEMENT> skill)
	{
		SKILLCHAIN_ELEMENT result = SC_NONE;
		for (std::list<SKILLCHAIN_ELEMENT>::iterator i = skill.begin(); i != skill.end(); i++)
		{
			for (std::list<SKILLCHAIN_ELEMENT>::iterator j = resonance.begin(); j != resonance.end(); j++)
			{
				// TODO: This could probably be implemented as a composite key lookup map. For now I like the way this looks.
				switch (PAIR((*j), (*i)))
				{
					// Level 3 Pairs
				case PAIR(SC_LIGHT, SC_LIGHT): return SC_LIGHT_II; break; // -> Lv4
				case PAIR(SC_DARKNESS, SC_DARKNESS): return SC_DARKNESS_II; break; // -> Lv4
					// Level 2 Pairs
				case PAIR(SC_GRAVITATION, SC_DISTORTION): return SC_DARKNESS; break; // -> Lv3
				case PAIR(SC_GRAVITATION, SC_FRAGMENTATION): return SC_FRAGMENTATION; break;
				case PAIR(SC_DISTORTION, SC_GRAVITATION): return SC_DARKNESS; break; // -> Lv3
				case PAIR(SC_DISTORTION, SC_FUSION): return SC_FUSION; break;
				case PAIR(SC_FUSION, SC_GRAVITATION): return SC_GRAVITATION; break;
				case PAIR(SC_FUSION, SC_FRAGMENTATION): return SC_LIGHT; break; // -> Lv3
				case PAIR(SC_FRAGMENTATION, SC_DISTORTION): return SC_DISTORTION; break;
				case PAIR(SC_FRAGMENTATION, SC_FUSION): return SC_LIGHT; break; // -> Lv3
					// Level 1 Pairs
				case PAIR(SC_TRANSFIXION, SC_COMPRESSION): return SC_COMPRESSION; break;
				case PAIR(SC_TRANSFIXION, SC_SCISSION): return SC_DISTORTION; break; // -> Lv2
				case PAIR(SC_TRANSFIXION, SC_REVERBERATION): return SC_REVERBERATION; break;
				case PAIR(SC_COMPRESSION, SC_TRANSFIXION): return SC_TRANSFIXION; break;
				case PAIR(SC_COMPRESSION, SC_DETONATION): return SC_DETONATION; break;
				case PAIR(SC_LIQUEFACTION, SC_SCISSION): return SC_SCISSION; break;
				case PAIR(SC_LIQUEFACTION, SC_IMPACTION): return SC_FUSION; break; // -> Lv2
				case PAIR(SC_SCISSION, SC_LIQUEFACTION): return SC_LIQUEFACTION; break;
				case PAIR(SC_SCISSION, SC_REVERBERATION): return SC_REVERBERATION; break;
				case PAIR(SC_SCISSION, SC_DETONATION): return SC_DETONATION; break;
				case PAIR(SC_REVERBERATION, SC_INDURATION): return SC_INDURATION; break;
				case PAIR(SC_REVERBERATION, SC_IMPACTION): return SC_IMPACTION; break;
				case PAIR(SC_DETONATION, SC_COMPRESSION): return SC_GRAVITATION; break; // -> Lv2
				case PAIR(SC_DETONATION, SC_SCISSION): return SC_SCISSION; break;
				case PAIR(SC_INDURATION, SC_COMPRESSION): return SC_COMPRESSION; break;
				case PAIR(SC_INDURATION, SC_REVERBERATION): return SC_FRAGMENTATION; break; // -> Lv2
				case PAIR(SC_INDURATION, SC_IMPACTION): return SC_IMPACTION; break;
				case PAIR(SC_IMPACTION, SC_LIQUEFACTION): return SC_LIQUEFACTION; break;
				case PAIR(SC_IMPACTION, SC_DETONATION): return SC_DETONATION; break;
				default: break;
				}
			}
		}
		return SC_NONE;
	}
	SUBEFFECT GetSkillChainEffect(CBattleEntity* PDefender, CWeaponSkill* PWeaponSkill)
	{
		CStatusEffect* PSCEffect = PDefender->StatusEffectContainer->GetStatusEffect(EFFECT_SKILLCHAIN, 0);
		CStatusEffect* PCBEffect = PDefender->StatusEffectContainer->GetStatusEffect(EFFECT_CHAINBOUND, 0);
		SKILLCHAIN_ELEMENT skillchain = SC_NONE;
		if (PSCEffect == NULL && PCBEffect == NULL)
		{
			// No effect exists, apply an effect using the weaponskill ID as the power with a tier of 0.
			PDefender->StatusEffectContainer->AddStatusEffect(new CStatusEffect(EFFECT_SKILLCHAIN, 0, PWeaponSkill->getID(), 0, 6, 0, 0, 0));
			return SUBEFFECT_NONE;
		}
		else
		{
			std::list<SKILLCHAIN_ELEMENT> skillProperties;
			skillProperties.push_back((SKILLCHAIN_ELEMENT)PWeaponSkill->getPrimarySkillchain());
			skillProperties.push_back((SKILLCHAIN_ELEMENT)PWeaponSkill->getSecondarySkillchain());
			skillProperties.push_back((SKILLCHAIN_ELEMENT)PWeaponSkill->getTertiarySkillchain());
			std::list<SKILLCHAIN_ELEMENT> resonanceProperties;
			// Chainbound active on target
			if (PCBEffect)
			{
				//Konzen-Ittai
				if (PCBEffect->GetPower() > 1)
				{
					resonanceProperties.push_back(SC_LIGHT);
					resonanceProperties.push_back(SC_DARKNESS);
					resonanceProperties.push_back(SC_GRAVITATION);
					resonanceProperties.push_back(SC_FRAGMENTATION);
					resonanceProperties.push_back(SC_DISTORTION);
					resonanceProperties.push_back(SC_FUSION);
				}
				resonanceProperties.push_back(SC_LIQUEFACTION);
				resonanceProperties.push_back(SC_INDURATION);
				resonanceProperties.push_back(SC_REVERBERATION);
				resonanceProperties.push_back(SC_IMPACTION);
				resonanceProperties.push_back(SC_COMPRESSION);
				skillchain = FormSkillchain(resonanceProperties, skillProperties);
				PDefender->StatusEffectContainer->AddStatusEffect(new CStatusEffect(EFFECT_SKILLCHAIN, 0, PWeaponSkill->getID(), 0, 6, 0, 0, 0));
				PDefender->StatusEffectContainer->DelStatusEffect(EFFECT_CHAINBOUND);
				PSCEffect = PDefender->StatusEffectContainer->GetStatusEffect(EFFECT_SKILLCHAIN, 0);
			}
			// Previous effect exists
			else if (PSCEffect->GetTier() == 0)
			{
				DSP_DEBUG_BREAK_IF(!PSCEffect->GetPower() && !PSCEffect->GetSubPower());
				// Previous effect is an opening effect, meaning the power is
				// actually the ID of the opening weaponskill. We need all 3
				// of the possible skillchain properties on the initial link.
				if (PSCEffect->GetPower())
				{
					resonanceProperties.push_back((SKILLCHAIN_ELEMENT)g_PWeaponSkillList[PSCEffect->GetPower()]->getPrimarySkillchain());
					resonanceProperties.push_back((SKILLCHAIN_ELEMENT)g_PWeaponSkillList[PSCEffect->GetPower()]->getSecondarySkillchain());
					resonanceProperties.push_back((SKILLCHAIN_ELEMENT)g_PWeaponSkillList[PSCEffect->GetPower()]->getTertiarySkillchain());
				}
				else
				{
					CBlueSpell* oldSpell = (CBlueSpell*)spell::GetSpell(PSCEffect->GetSubPower());
					resonanceProperties.push_back((SKILLCHAIN_ELEMENT)oldSpell->getPrimarySkillchain());
					resonanceProperties.push_back((SKILLCHAIN_ELEMENT)oldSpell->getSecondarySkillchain());
				}
				skillchain = FormSkillchain(resonanceProperties, skillProperties);
			}
			else
			{
				// Previous effect is not an opening effect, meaning the power is
				// The skill chain ID resonating.
				resonanceProperties.push_back((SKILLCHAIN_ELEMENT)PSCEffect->GetPower());
				skillchain = FormSkillchain(resonanceProperties, skillProperties);
			}
			if (skillchain != SC_NONE)
			{
				PSCEffect->SetStartTime(gettick());
				PSCEffect->SetTier(GetSkillchainTier((SKILLCHAIN_ELEMENT)skillchain));
				PSCEffect->SetPower(skillchain);
				PSCEffect->SetSubPower(dsp_min(PSCEffect->GetSubPower() + 1, 5)); // Linked, limited to 5
				return (SUBEFFECT)GetSkillchainSubeffect((SKILLCHAIN_ELEMENT)skillchain);
			}
			PSCEffect->SetStartTime(gettick());
			PSCEffect->SetTier(0);
			PSCEffect->SetPower(PWeaponSkill->getID());
			PSCEffect->SetSubPower(0);
			return SUBEFFECT_NONE;
		}
	}
	SUBEFFECT GetSkillChainEffect(CBattleEntity* PDefender, CBlueSpell* PSpell)
	{
		CStatusEffect* PSCEffect = PDefender->StatusEffectContainer->GetStatusEffect(EFFECT_SKILLCHAIN, 0);
		CStatusEffect* PCBEffect = PDefender->StatusEffectContainer->GetStatusEffect(EFFECT_CHAINBOUND, 0);
		SKILLCHAIN_ELEMENT skillchain = SC_NONE;
		if (PSCEffect == NULL && PCBEffect == NULL)
		{
			// No effect exists, apply an effect using the weaponskill ID as the power with a tier of 0.
			PDefender->StatusEffectContainer->AddStatusEffect(new CStatusEffect(EFFECT_SKILLCHAIN, 0, 0, 0, 6, 0, PSpell->getID(), 0));
			return SUBEFFECT_NONE;
		}
		else
		{
			std::list<SKILLCHAIN_ELEMENT> skillProperties;
			skillProperties.push_back((SKILLCHAIN_ELEMENT)PSpell->getPrimarySkillchain());
			skillProperties.push_back((SKILLCHAIN_ELEMENT)PSpell->getSecondarySkillchain());
			std::list<SKILLCHAIN_ELEMENT> resonanceProperties;
			// Chainbound active on target
			if (PCBEffect)
			{
				//Konzen-Ittai
				if (PCBEffect->GetPower() > 1)
				{
					resonanceProperties.push_back(SC_LIGHT);
					resonanceProperties.push_back(SC_DARKNESS);
					resonanceProperties.push_back(SC_GRAVITATION);
					resonanceProperties.push_back(SC_FRAGMENTATION);
					resonanceProperties.push_back(SC_DISTORTION);
					resonanceProperties.push_back(SC_FUSION);
				}
				resonanceProperties.push_back(SC_LIQUEFACTION);
				resonanceProperties.push_back(SC_INDURATION);
				resonanceProperties.push_back(SC_REVERBERATION);
				resonanceProperties.push_back(SC_IMPACTION);
				resonanceProperties.push_back(SC_COMPRESSION);
				skillchain = FormSkillchain(resonanceProperties, skillProperties);
				PDefender->StatusEffectContainer->AddStatusEffect(new CStatusEffect(EFFECT_SKILLCHAIN, 0, 0, 0, 6, 0, PSpell->getID(), 0));
				PDefender->StatusEffectContainer->DelStatusEffect(EFFECT_CHAINBOUND);
				PSCEffect = PDefender->StatusEffectContainer->GetStatusEffect(EFFECT_SKILLCHAIN, 0);
			}
			// Previous effect exists
			else if (PSCEffect->GetTier() == 0)
			{
				DSP_DEBUG_BREAK_IF(!PSCEffect->GetPower() && !PSCEffect->GetSubPower());
				// Previous effect is an opening effect, meaning the power is
				// actually the ID of the opening weaponskill. We need all 3
				// of the possible skillchain properties on the initial link.
				if (PSCEffect->GetPower())
				{
					resonanceProperties.push_back((SKILLCHAIN_ELEMENT)g_PWeaponSkillList[PSCEffect->GetPower()]->getPrimarySkillchain());
					resonanceProperties.push_back((SKILLCHAIN_ELEMENT)g_PWeaponSkillList[PSCEffect->GetPower()]->getSecondarySkillchain());
					resonanceProperties.push_back((SKILLCHAIN_ELEMENT)g_PWeaponSkillList[PSCEffect->GetPower()]->getTertiarySkillchain());
				}
				else
				{
					CBlueSpell* oldSpell = (CBlueSpell*)spell::GetSpell(PSCEffect->GetSubPower());
					resonanceProperties.push_back((SKILLCHAIN_ELEMENT)oldSpell->getPrimarySkillchain());
					resonanceProperties.push_back((SKILLCHAIN_ELEMENT)oldSpell->getSecondarySkillchain());
				}
				skillchain = FormSkillchain(resonanceProperties, skillProperties);
			}
			else
			{
				// Previous effect is not an opening effect, meaning the power is
				// The skill chain ID resonating.
				resonanceProperties.push_back((SKILLCHAIN_ELEMENT)PSCEffect->GetPower());
				skillchain = FormSkillchain(resonanceProperties, skillProperties);
			}
			if (skillchain != SC_NONE)
			{
				PSCEffect->SetStartTime(gettick());
				PSCEffect->SetTier(GetSkillchainTier((SKILLCHAIN_ELEMENT)skillchain));
				PSCEffect->SetPower(skillchain);
				PSCEffect->SetSubPower(dsp_min(PSCEffect->GetSubPower() + 1, 5)); // Linked, limited to 5
				return (SUBEFFECT)GetSkillchainSubeffect((SKILLCHAIN_ELEMENT)skillchain);
			}
			PSCEffect->SetStartTime(gettick());
			PSCEffect->SetTier(0);
			PSCEffect->SetSubPower(PSpell->getID());
			PSCEffect->SetPower(0);
			return SUBEFFECT_NONE;
		}
	}
	uint16 GetSkillchainMinimumResistance(SKILLCHAIN_ELEMENT element, CBattleEntity* PDefender)
	{
		static const uint16 resistances[][4] =
		{
			{ MOD_NONE, MOD_NONE, MOD_NONE, MOD_NONE }, // SC_NONE
			{ MOD_LIGHTDEF, MOD_NONE, MOD_NONE, MOD_NONE }, // SC_TRANSFIXION
			{ MOD_DARKDEF, MOD_NONE, MOD_NONE, MOD_NONE }, // SC_COMPRESSION
			{ MOD_FIREDEF, MOD_NONE, MOD_NONE, MOD_NONE }, // SC_LIQUEFACTION
			{ MOD_EARTHDEF, MOD_NONE, MOD_NONE, MOD_NONE }, // SC_SCISSION
			{ MOD_WATERDEF, MOD_NONE, MOD_NONE, MOD_NONE }, // SC_REVERBERATION
			{ MOD_WINDDEF, MOD_NONE, MOD_NONE, MOD_NONE }, // SC_DETONATION
			{ MOD_ICEDEF, MOD_NONE, MOD_NONE, MOD_NONE }, // SC_INDURATION
			{ MOD_THUNDERDEF, MOD_NONE, MOD_NONE, MOD_NONE }, // SC_IMPACTION
			{ MOD_FIREDEF, MOD_LIGHTDEF, MOD_NONE, MOD_NONE }, // SC_FUSION
			{ MOD_WINDDEF, MOD_THUNDERDEF, MOD_NONE, MOD_NONE }, // SC_FRAGMENTATION
			{ MOD_EARTHDEF, MOD_DARKDEF, MOD_NONE, MOD_NONE }, // SC_GRAVITATION
			{ MOD_ICEDEF, MOD_WATERDEF, MOD_NONE, MOD_NONE }, // SC_DISTORTION
			{ MOD_FIREDEF, MOD_WINDDEF, MOD_THUNDERDEF, MOD_LIGHTDEF }, // SC_LIGHT
			{ MOD_ICEDEF, MOD_EARTHDEF, MOD_WATERDEF, MOD_DARKDEF }, // SC_DARKNESS_II
			{ MOD_FIREDEF, MOD_WINDDEF, MOD_THUNDERDEF, MOD_LIGHTDEF }, // SC_LIGHT
			{ MOD_ICEDEF, MOD_EARTHDEF, MOD_WATERDEF, MOD_DARKDEF }, // SC_DARKNESS_II
		};
		switch (element)
		{
			// Level 1 skill chains
		case SC_LIQUEFACTION:
		case SC_IMPACTION:
		case SC_DETONATION:
		case SC_SCISSION:
		case SC_REVERBERATION:
		case SC_INDURATION:
		case SC_COMPRESSION:
		case SC_TRANSFIXION:
			return PDefender->getMod(resistances[element][0]);
			break;
			// Level 2 skill chains
		case SC_FUSION:
		case SC_FRAGMENTATION:
		case SC_GRAVITATION:
		case SC_DISTORTION:
			return dsp_min(PDefender->getMod(resistances[element][0]), PDefender->getMod(resistances[element][1]));
			break;
			// Level 3 & 4 skill chains
		case SC_LIGHT:
		case SC_LIGHT_II:
		case SC_DARKNESS:
		case SC_DARKNESS_II:
			return dsp_min(dsp_min(PDefender->getMod(resistances[element][0]),
				PDefender->getMod(resistances[element][1])),
				dsp_min(PDefender->getMod(resistances[element][2]),
				PDefender->getMod(resistances[element][3])));
			break;
		default:
			DSP_DEBUG_BREAK_IF(true);
			return 0;
			break;
		}
	}
	uint16 TakeSkillchainDamage(CBattleEntity* PAttacker, CBattleEntity* PDefender, uint16 lastSkillDamage)
	{
		DSP_DEBUG_BREAK_IF(PAttacker == NULL);
		DSP_DEBUG_BREAK_IF(PDefender == NULL);
		CStatusEffect* PEffect = PDefender->StatusEffectContainer->GetStatusEffect(EFFECT_SKILLCHAIN, 0);
		// Determine the skill chain level and elemental resistance.
		SKILLCHAIN_ELEMENT skillchain = (SKILLCHAIN_ELEMENT)PEffect->GetPower();
		uint16 chainLevel = PEffect->GetTier();
		uint16 chainCount = PEffect->GetSubPower();
		int16 resistance = GetSkillchainMinimumResistance(skillchain, PDefender);
		DSP_DEBUG_BREAK_IF(chainLevel <= 0 || chainLevel > 4 || chainCount <= 0 || chainCount > 5);
		// Skill chain damage = (Closing Damage)
		// × (Skill chain Level/Number from Table)
		// × (1 + Skill chain Bonus ÷ 100)
		// × (1 + Skill chain Damage + %/100)
		// TODO: × (1 + Day/Weather bonuses)
		// TODO: × (1 + Staff Affinity)
		uint32 damage = floor((double)lastSkillDamage
			* g_SkillChainDamageModifiers[chainLevel][chainCount] / 1000
			* (100 + PAttacker->getMod(MOD_SKILLCHAINBONUS)) / 100
			* (100 + PAttacker->getMod(MOD_SKILLCHAINDMG)) / 100);
		damage = damage * (1000 - resistance) / 1000;
		// Handle Severe Damage Reduction Effects
		damage = HandleSevereDamage(PDefender, damage);
		PDefender->addHP(-damage);
		if (PAttacker->PMaster != NULL)
		{
			PDefender->m_OwnerID.id = PAttacker->PMaster->id;
			PDefender->m_OwnerID.targid = PAttacker->PMaster->targid;
		}
		else
		{
			PDefender->m_OwnerID.id = PAttacker->id;
			PDefender->m_OwnerID.targid = PAttacker->targid;
		}
		switch (PDefender->objtype)
		{
		case TYPE_PC:
		{
			PDefender->StatusEffectContainer->DelStatusEffectsByFlag(EFFECTFLAG_DAMAGE);
			if (PDefender->animation == ANIMATION_SIT)
			{
				PDefender->animation = ANIMATION_NONE;
				((CCharEntity*)PDefender)->pushPacket(new CCharUpdatePacket((CCharEntity*)PDefender));
			}
			charutils::UpdateHealth((CCharEntity*)PDefender);
		}
			break;
		case TYPE_MOB:
		{
			((CMobEntity*)PDefender)->PEnmityContainer->UpdateEnmityFromDamage(PAttacker, damage);
		}
			break;
		}
		return damage;
	}
	CItemArmor* GetEntityArmor(CBattleEntity* PEntity, SLOTTYPE Slot)
	{
		DSP_DEBUG_BREAK_IF(Slot < SLOT_HEAD || Slot > SLOT_LINK);
		if (PEntity->objtype == TYPE_PC)
		{
			return (((CCharEntity*)PEntity)->getEquip(Slot));
		}
		else if (PEntity->objtype == TYPE_NPC)
		{
			return NULL;
		}
		return NULL;
	}
	CItemWeapon* GetEntityWeapon(CBattleEntity* PEntity, SLOTTYPE Slot)
	{
		DSP_DEBUG_BREAK_IF(Slot < SLOT_MAIN || Slot > SLOT_AMMO);
		if (PEntity->objtype == TYPE_PC)
		{
			return (CItemWeapon*)(((CCharEntity*)PEntity)->getEquip(Slot));
		}
		else if (PEntity->objtype == TYPE_NPC)
		{
			return (CItemWeapon*)(((CMobEntity*)PEntity)->m_Weapons[Slot]);
		}
		return NULL;
	}
	void MakeEntityStandUp(CBattleEntity* PEntity)
	{
		DSP_DEBUG_BREAK_IF(PEntity == NULL);
		if (PEntity->objtype == TYPE_PC)
		{
			CCharEntity* PPlayer = ((CCharEntity*)PEntity);
			if (PPlayer->animation == ANIMATION_HEALING)
			{
				PPlayer->StatusEffectContainer->DelStatusEffect(EFFECT_HEALING);
			}
		}
	}
	bool IsEngauged(CBattleEntity* PEntity)
	{
		DSP_DEBUG_BREAK_IF(PEntity == NULL);
		return (PEntity->animation == ANIMATION_ATTACK &&
			PEntity->PBattleAI != NULL &&
			PEntity->PBattleAI->GetBattleTarget() != NULL &&
			PEntity->status != STATUS_DISAPPEAR);
	}
	/************************************************************************
	* *
	* Для всех сущностей, за исключением персонажей, по умолчанию true *
	* *
	************************************************************************/
	bool HasNinjaTool(CBattleEntity* PEntity, CSpell* PSpell, bool ConsumeTool)
	{
		DSP_DEBUG_BREAK_IF(PEntity == NULL || PSpell == NULL);
		if (PEntity->objtype == TYPE_PC)
		{
			CCharEntity* PChar = ((CCharEntity*)PEntity);
			uint8 SlotID = 0;
			uint16 toolID = PSpell->getMPCost();
			if (ERROR_SLOTID == (SlotID = PChar->getStorage(LOC_INVENTORY)->SearchItem(toolID)))
			{
				if (PChar->GetMJob() == JOB_NIN)
				{
					switch (toolID)
					{
					case ITEM_UCHITAKE:
					case ITEM_TSURARA:
					case ITEM_KAWAHORI_OGI:
					case ITEM_MAKIBISHI:
					case ITEM_HIRAISHIN:
					case ITEM_MIZU_DEPPO:
						toolID = ITEM_INOSHISHINOFUDA;
						break;
					case ITEM_RYUNO:
					case ITEM_MOKUJIN:
					case ITEM_SANJAKU_TENUGUI:
					case ITEM_KABENRO:
					case ITEM_SHINOBI_TABI:
					case ITEM_SHIHEI:
						toolID = ITEM_SHIKANOFUDA;
						break;
					case ITEM_SOSHI:
					case ITEM_KODOKU:
					case ITEM_KAGINAWA:
					case ITEM_JUSATSU:
					case ITEM_SAIRUI_RAN:
					case ITEM_JINKO:
						toolID = ITEM_CHONOFUDA;
						break;
					default: return false;
					}
					if (ERROR_SLOTID == (SlotID = PChar->getStorage(LOC_INVENTORY)->SearchItem(toolID)))
					{
						return false;
					}
				}
				else
				{
					return false;
				}
			}
			// Should only make it to this point if a ninja tool was found.
			// Check For Futae Effect
			bool hasFutae = PChar->StatusEffectContainer->HasStatusEffect(EFFECT_FUTAE);
			// Futae only applies to Elemental Wheel Tools
			bool useFutae = (toolID == ITEM_UCHITAKE || toolID == ITEM_TSURARA || toolID == ITEM_KAWAHORI_OGI || toolID == ITEM_MAKIBISHI || toolID == ITEM_HIRAISHIN || toolID == ITEM_MIZU_DEPPO);
			// If you have Futae active, Ninja Tool Expertise does not apply.
			if (ConsumeTool && hasFutae && useFutae)
			{
				// Futae Takes 2 of Your Tools
				charutils::UpdateItem(PChar, LOC_INVENTORY, SlotID, -2);
				PChar->pushPacket(new CInventoryFinishPacket());
			}
			else
			{
				uint16 meritBonus = 0;
				if (charutils::hasTrait(PChar, TRAIT_NINJA_TOOL_EXPERT))
					meritBonus = PChar->PMeritPoints->GetMeritValue(MERIT_NINJA_TOOL_EXPERTISE, (CCharEntity*)PChar);
				uint16 chance = (PChar->getMod(MOD_NINJA_TOOL) + meritBonus);
				if (ConsumeTool && WELL512::irand() % 100 > chance)
				{
					charutils::UpdateItem(PChar, LOC_INVENTORY, SlotID, -1);
					PChar->pushPacket(new CInventoryFinishPacket());
				}
			}
		}
		return true;
	}
	/*
	* Find if any party members are in position for trick attack. Do this by making a narrow triangle:
	* one endpoint at the mob, and the other two endpoints being equidistant from the TA user, perpendicular to
	* the line between the mob and the TA user. Find the slope of the line between the TA user and the mob, and
	* decide whether to use x or z as the dependent variable (to avoid big numbers and divide by 0 errors on
	* vertical slopes). Using this slope, we can find the angle of the perpendicular line to the x or z line
	* (depending on what the dependent var is), and using that angle, the disassembled x and z components to that
	* line. Divide those by 2 for a half yalm length line for each side of the base of the triangle, and we get
	* the min and max values for x/z around the TA user. Now it's simply a matter of first: making sure the
	* TA target is closer than the TA user, and that the TA targets x and z coordinates fall within the triangle
	* we made. Using the min/max points and the mobs coordinate, we can construct min and max slopes, check that
	* the x or z coordinates are between the mob and player, and finally calculate where the z coordinate should
	* be based on the users x coordinate (or the other way around in a z dependent scenario) and check if the
	* actual z coordinate is between those two values.
	*/
	CBattleEntity* getAvailableTrickAttackChar(CBattleEntity* taUser, CBattleEntity* PMob)
	{
		if (!taUser->StatusEffectContainer->HasStatusEffect(EFFECT_TRICK_ATTACK))
		{
			return NULL;
		}
		float taUserX = taUser->loc.p.x;
		float taUserZ = taUser->loc.p.z;
		float mobX = PMob->loc.p.x;
		float mobZ = PMob->loc.p.z;
		float xdif = taUserX - mobX;
		float zdif = taUserZ - mobZ;
		float slope = 0;
		float maxSlope = 0;
		float minSlope = 0;
		bool zDependent = true; //using a slope where z is dependent var
		if (abs(xdif) <= abs(zdif))
		{
			slope = xdif / zdif;
			float angle = atan((double)1) * 2 - atan(slope);
			float zoffset = cos(angle) / 2;
			float xoffset = sin(angle) / 2;
			float maxXpoint = taUserX + xoffset;
			float maxZpoint = taUserZ - zoffset;
			float minXpoint = taUserX - xoffset;
			float minZpoint = taUserZ + zoffset;
			maxSlope = ((maxXpoint - mobX) / (maxZpoint - mobZ));
			minSlope = ((minXpoint - mobX) / (minZpoint - mobZ));
			zDependent = false;
		}
		else {
			slope = zdif / xdif;
			float angle = atan((double)1) * 2 - atan(slope);
			float xoffset = cos(angle) / 2;
			float zoffset = sin(angle) / 2;
			float maxXpoint = taUserX + xoffset;
			float maxZpoint = taUserZ + zoffset;
			float minXpoint = taUserX - xoffset;
			float minZpoint = taUserZ - zoffset;
			maxSlope = (maxZpoint - mobZ) / (maxXpoint - mobX);
			minSlope = (minZpoint - mobZ) / (minXpoint - mobX);
		}
		if (taUser->PParty != NULL)
		{
			if (taUser->PParty->m_PAlliance != NULL)
			{
				for (uint8 a = 0; a < taUser->PParty->m_PAlliance->partyList.size(); ++a)
				{
					for (uint8 i = 0; i < taUser->PParty->m_PAlliance->partyList.at(a)->members.size(); ++i)
					{
						CBattleEntity* member = taUser->PParty->m_PAlliance->partyList.at(a)->members.at(i);
						if (taUser->id != member->id && distance(member->loc.p, PMob->loc.p) <= distance(taUser->loc.p, PMob->loc.p))
						{
							if (zDependent)
							{
								//is member between taUser and PMob on x line?
								if ((member->loc.p.x <= taUserX && member->loc.p.x >= mobX) ||
									(member->loc.p.x >= taUserX && member->loc.p.x <= mobX))
								{
									if ((member->loc.p.z - mobZ <= (member->loc.p.x - mobX)*maxSlope) &&
										(member->loc.p.z - mobZ >= (member->loc.p.x - mobX)*minSlope))
									{
										//finally found a TA partner
										return member;
									}
								}
							}
							else {
								//is member between taUser and PMob on z line?
								if ((member->loc.p.z <= taUserZ && member->loc.p.z >= mobZ) ||
									(member->loc.p.z >= taUserZ && member->loc.p.z <= mobZ))
								{
									if ((member->loc.p.x - mobX <= (member->loc.p.z - mobZ)*maxSlope) &&
										(member->loc.p.x - mobX >= (member->loc.p.z - mobZ)*minSlope))
									{
										//finally found a TA partner
										return member;
									}
								}
							}
						}
					}
				}
			}
			else{//no alliance
				for (uint8 i = 0; i < taUser->PParty->members.size(); ++i)
				{
					CBattleEntity* member = taUser->PParty->members.at(i);
					if (member->id != taUser->id && distance(member->loc.p, PMob->loc.p) <= distance(taUser->loc.p, PMob->loc.p))
					{
						if (zDependent)
						{
							//is member between taUser and PMob on x line?
							if ((member->loc.p.x <= taUserX && member->loc.p.x >= mobX) ||
								(member->loc.p.x >= taUserX && member->loc.p.x <= mobX))
							{
								if ((member->loc.p.z - mobZ <= (member->loc.p.x - mobX)*maxSlope) &&
									(member->loc.p.z - mobZ >= (member->loc.p.x - mobX)*minSlope))
								{
									//finally found a TA partner
									return member;
								}
							}
						}
						else {
							//is member between taUser and PMob on z line?
							if ((member->loc.p.z <= taUserZ && member->loc.p.z >= mobZ) ||
								(member->loc.p.z >= taUserZ && member->loc.p.z <= mobZ))
							{
								if ((member->loc.p.x - mobX <= (member->loc.p.z - mobZ)*maxSlope) &&
									(member->loc.p.x - mobX >= (member->loc.p.z - mobZ)*minSlope))
								{
									//finally found a TA partner
									return member;
								}
							}
						}
					}
				}
			}
		}
		//no Trick attack party member available
		return NULL;
	}
	/************************************************************************
	* *
	* Adds enmity to PSource for all the MOB targets who have *
	* PTarget on their enmity list. *
	* *
	************************************************************************/
	void GenerateCureEnmity(CBattleEntity* PSource, CBattleEntity* PTarget, uint16 amount)
	{
		DSP_DEBUG_BREAK_IF(PSource == NULL);
		DSP_DEBUG_BREAK_IF(PTarget == NULL);
		DSP_DEBUG_BREAK_IF(amount < 0);
		DSP_DEBUG_BREAK_IF(PSource->objtype != TYPE_PC);
		CCharEntity* PChar = (CCharEntity*)PSource;
		for (SpawnIDList_t::const_iterator it = PChar->SpawnMOBList.begin(); it != PChar->SpawnMOBList.end(); ++it)
		{
			CMobEntity* PCurrentMob = (CMobEntity*)it->second;
			if (PCurrentMob->PEnmityContainer->HasTargetID(PTarget->id))
			{
				PCurrentMob->PEnmityContainer->UpdateEnmityFromCure(PChar, PTarget->GetMLevel(), amount, (amount == 65535)); //true for "cure v"
			}
		}
	}
	/************************************************************************
	* *
	* Transfer Enmity (used with ACCOMPLICE & COLLABORATOR ability type) *
	* *
	************************************************************************/
	void TransferEnmity(CBattleEntity* CharHateReceiver, CBattleEntity* CharHateGiver, CMobEntity* PMob, uint8 percentToTransfer)
	{
		// Ensure the players have a battle target..
		if (PMob == NULL || ((CMobEntity*)PMob)->PEnmityContainer == NULL)
			return;
		//CBaseEntity* PMob = CharHateGiver->GetEntity(mobID, TYPE_MOB);
		((CMobEntity*)PMob)->PEnmityContainer->LowerEnmityByPercent(CharHateGiver, percentToTransfer, CharHateReceiver);
	}
	/************************************************************************
	* *
	* Effect from soul eater *
	* *
	************************************************************************/
	uint16 doSoulEaterEffect(CCharEntity* m_PChar, uint32 damage)
	{
		// Souleater has no effect <10HP.
		if (m_PChar->GetMJob() == JOB_DRK && m_PChar->health.hp >= 10 && m_PChar->StatusEffectContainer->HasStatusEffect(EFFECT_SOULEATER))
		{
			//lost 10% current hp, converted to damage (displayed as just a strong regular hit)
			float drainPercent = 0.1;
			CItem* PItemHead = ((CCharEntity*)m_PChar)->getEquip(SLOT_HEAD);
			CItem* PItemBody = ((CCharEntity*)m_PChar)->getEquip(SLOT_BODY);
			CItem* PItemLegs = ((CCharEntity*)m_PChar)->getEquip(SLOT_LEGS);
			if ((PItemHead && (PItemHead->getID() == 12516 || PItemHead->getID() == 15232)) || (PItemBody && PItemBody->getID() == 14409) || (PItemLegs && PItemLegs->getID() == 15370))
				drainPercent = 0.12;
			damage = damage + m_PChar->health.hp*drainPercent;
			m_PChar->addHP(-drainPercent*m_PChar->health.hp);
		}
		else if (m_PChar->GetSJob() == JOB_DRK &&m_PChar->health.hp >= 10 && m_PChar->StatusEffectContainer->HasStatusEffect(EFFECT_SOULEATER)){
			//lose 10% Current HP, only HALF (5%) converted to damage
			damage = damage + m_PChar->health.hp*0.05;
			m_PChar->addHP(-0.1*m_PChar->health.hp);
		}
		return damage;
	}
	/************************************************************************
	* *
	* Samurai get merit storeTP value *
	* *
	************************************************************************/
	uint8 getStoreTPbonusFromMerit(CBattleEntity* PEntity)
	{
		if (PEntity->objtype == TYPE_PC)
		{
			if (((CCharEntity*)PEntity)->GetMJob() == JOB_SAM)
			{
				return ((CCharEntity*)PEntity)->PMeritPoints->GetMeritValue(MERIT_STORE_TP_EFFECT, (CCharEntity*)PEntity);
			}
		}
		return 0;
	}
	/************************************************************************
	* *
	* Samurai overwhelm damage bonus *
	* *
	************************************************************************/
	uint16 getOverWhelmDamageBonus(CCharEntity* m_PChar, CBattleEntity* PDefender, uint16 damage)
	{
		if (m_PChar->GetMJob() == JOB_SAM || m_PChar->GetSJob() == JOB_SAM) // only allow if player 75 or more
		{
			if (m_PChar->GetMLevel() >= 75)
			{
				// must be facing mob
				if (isFaceing(PDefender->loc.p, m_PChar->loc.p, 90))
				{
					uint8 meritCount = m_PChar->PMeritPoints->GetMeritValue(MERIT_OVERWHELM, m_PChar);
					float tmpDamage = damage;
					switch (meritCount)
					{
					case 1:	tmpDamage += tmpDamage * 0.05f; break;
					case 2:	tmpDamage += tmpDamage * 0.10f; break;
					case 3:	tmpDamage += tmpDamage * 0.15f; break;
					case 4:	tmpDamage += tmpDamage * 0.17f; break;
					case 5:	tmpDamage += tmpDamage * 0.19f; break;
					default: break;
					}
					damage = (uint16)floor(tmpDamage);
				}
			}
		}
		return damage;
	}
	/************************************************************************
	* *
	* get barrage shot count *
	* *
	************************************************************************/
	uint8 getBarrageShotCount(CCharEntity* PChar)
	{
		/*
		Ranger level 30, four shots.
		Ranger level 50, five shots.
		Ranger level 75, six shots.
		Ranger level 90, seven shots.
		Ranger level 99, eight shots.
		*/
		// only archery + marksmanship can use barrage
		CItemWeapon* PItem = (CItemWeapon*)PChar->getEquip(SLOT_RANGED);
		if (PItem && PItem->getSkillType() != 25 && PItem->getSkillType() != 26)
		{
			return 0;
		}
		uint8 lvl = PChar->jobs.job[JOB_RNG];	// Get Ranger level of char
		uint8 shotCount = 0;	// the total number of extra hits
		if (PChar->GetSJob() == JOB_RNG)	// if rng is sub then use the sub level
			lvl = PChar->GetSLevel();
		// Hunters bracers+1 will add an extra shot
		CItemArmor* PItemHands = PChar->getEquip(SLOT_HANDS);
		if (PItemHands && PItemHands->getID() == 14900)
			shotCount++;
		if (lvl < 30)	return 0;
		else if (lvl < 50)	shotCount += 3;
		else if (lvl < 75)	shotCount += 4;
		else if (lvl < 90)	shotCount += 5;
		else if (lvl < 99)	shotCount += 6;
		else if (lvl >= 99) shotCount += 7;
		// make sure we have enough ammo for all these shots
		CItemWeapon* PAmmo = (CItemWeapon*)PChar->getEquip(SLOT_AMMO);
		if (PAmmo && PAmmo->getQuantity() < shotCount)
		{
			shotCount = PAmmo->getQuantity() - 1;
		}
		return shotCount;
	}
	/************************************************************************
	* *
	* Jump DRG Job ability *
	* *
	************************************************************************/
	uint16 jumpAbility(CBattleEntity* PAttacker, CBattleEntity* PVictim, uint8 tier)
	{
		// super jump - remove 99% of enmity
		if (tier == 3 && PVictim->objtype == TYPE_MOB)
		{
			((CMobEntity*)PVictim)->PEnmityContainer->LowerEnmityByPercent(PAttacker, 99, NULL);
			return 0;
		}
		// target has perfect dodge - do not go any further
		if (PVictim->StatusEffectContainer->HasStatusEffect(EFFECT_PERFECT_DODGE, 0))
			return 0;
		// multihit's just multiply jump damage
		uint16 subType = PAttacker->m_Weapons[SLOT_SUB]->getDmgType();
		uint8 numattacksLeftHand = 0;
		//sub weapon is equipped
		if ((subType > 0 && subType < 4))
			numattacksLeftHand = battleutils::CheckMultiHits(PAttacker, PAttacker->m_Weapons[SLOT_SUB]);
		//h2h equipped
		if (PAttacker->m_Weapons[SLOT_MAIN]->getDmgType() == DAMAGE_HTH)
			numattacksLeftHand = battleutils::CheckMultiHits(PAttacker, PAttacker->m_Weapons[SLOT_MAIN]);
		// normal multi hit from left hand
		uint8 numattacksRightHand = battleutils::CheckMultiHits(PAttacker, PAttacker->m_Weapons[SLOT_MAIN]);
		CItemWeapon* PWeapon = PAttacker->m_Weapons[SLOT_MAIN];
		uint8 fstrslot = SLOT_MAIN;
		uint8 hitrate = battleutils::GetHitRate(PAttacker, PVictim);
		uint8 realHits = 0;	// to store the real number of hit for tp multipler
		uint16 totalDamage = 0;
		uint16 damageForRound = 0;
		bool hitTarget = false;
		// Loop number of hits
		for (uint8 i = 0; i < (numattacksLeftHand + numattacksRightHand); ++i)
		{
			if (i != 0)
			{
				if (PVictim->isDead())
					break;
				if (PAttacker->m_Weapons[SLOT_MAIN]->getDmgType() != DAMAGE_HTH && i >= numattacksRightHand)
				{
					PWeapon = PAttacker->m_Weapons[SLOT_SUB];
					fstrslot = SLOT_SUB;
				}
			}
			if (WELL512::irand() % 100 < hitrate)
			{
				// attack hit, try to be absorbed by shadow
				if (!battleutils::IsAbsorbByShadow(PVictim))
				{
					// successful hit, add damage
					uint8 AttMultiplerPercent = 0;
					// get jump attack bonus from gear
					if (PAttacker->objtype == TYPE_PC)
						AttMultiplerPercent = PAttacker->getMod(MOD_JUMP_ATT_BONUS);
					float DamageRatio = battleutils::GetDamageRatio(PAttacker, PVictim, false, AttMultiplerPercent);
					damageForRound = (uint16)((PAttacker->GetMainWeaponDmg() + battleutils::GetFSTR(PAttacker, PVictim, SLOT_MAIN)) * DamageRatio);
					// bonus applies to jump only, not high jump
					if (tier == 1)
					{
						float jumpBonus = ((PAttacker->VIT() / (float)256) + 1);
						damageForRound = damageForRound * jumpBonus;
					}
					hitTarget = true;
					realHits++;
					// incase player has gungnir^^ (or any other damage increases weapons)
					damageForRound = attackutils::CheckForDamageMultiplier((CCharEntity*)PAttacker, PWeapon, damageForRound, ATTACK_NORMAL);
					totalDamage += damageForRound;
				}
			}
		}
		//check for soul eater
		if (PAttacker->objtype == TYPE_PC)
			totalDamage = battleutils::doSoulEaterEffect((CCharEntity*)PAttacker, totalDamage);
		// bonus jump tp is added even if damage is 0, will not add if jump misses
		if (PAttacker->objtype == TYPE_PC && hitTarget)
		{
			int mod = PAttacker->getMod(MOD_JUMP_TP_BONUS);
			PAttacker->addTP(mod);
		}
		// if damage is 0 then jump missed
		if (totalDamage == 0)
			return 0;
		// high jump removes %50 emnity + more from any gear mods
		if (tier == 2 && PVictim->objtype == TYPE_MOB)
		{
			uint16 enmityReduction = PAttacker->getMod(MOD_HIGH_JUMP_ENMITY_REDUCTION) + 50;
			// cap it
			if (enmityReduction > 100)
			{
				enmityReduction = 100;
			}
			((CMobEntity*)PVictim)->PEnmityContainer->LowerEnmityByPercent(PAttacker, enmityReduction, NULL);
		}
		// Under Spirit Surge, High Jump lowers the target's TP proportionately to the amount of damage dealt (TP is reduced by damage * 20)
		if (tier == 2 && PAttacker->StatusEffectContainer->HasStatusEffect(EFFECT_SPIRIT_SURGE))
			PVictim->addTP(-(totalDamage * 20));
		// try skill up (CharEntity only)
		if (PAttacker->objtype == TYPE_PC)
			charutils::TrySkillUP((CCharEntity*)PAttacker, (SKILLTYPE)PWeapon->getSkillType(), PVictim->GetMLevel());
		// jump + high jump doesn't give any tp to victim
		battleutils::TakePhysicalDamage(PAttacker, PVictim, totalDamage, false, fstrslot, realHits, NULL, false);
		return totalDamage;
	}
	/************************************************************************
	* *
	* Entity charms another *
	* *
	************************************************************************/
	void tryToCharm(CBattleEntity* PCharmer, CBattleEntity* PVictim)
	{
		//Gear with Charm + does not affect the success rate of Charm, but increases the duration of the Charm.
		//Each +1 to Charm increases the duration of charm by 5%; +20 Charm doubles the duration of charm.
		//Too Weak 30 Minutes
		//Easy Prey 20 Minutes
		//Decent Challenge 10 Minutes
		//Even Match 3.0 Minutes
		//Tough 1.5 Minutes
		//VT 1 minute guess
		//IT 30 seconds guess
		uint32 CharmTime = 0;
		uint32 base = 0;
		// player charming mob
		if (PVictim->objtype == TYPE_MOB && PCharmer->objtype == TYPE_PC)
		{
			//Bind uncharmable mobs for 5 seconds
			if (PVictim->m_EcoSystem == SYSTEM_BEASTMEN || PVictim->m_EcoSystem == SYSTEM_ARCANA ||
				PVictim->m_EcoSystem == SYSTEM_UNCLASSIFIED || PVictim->m_EcoSystem == SYSTEM_ARCHAICMACHINE ||
				PVictim->m_EcoSystem == SYSTEM_AVATAR || PVictim->m_EcoSystem == SYSTEM_DEMON ||
				PVictim->m_EcoSystem == SYSTEM_ELEMENTAL || PVictim->m_EcoSystem == SYSTEM_EMPTY ||
				PVictim->m_EcoSystem == SYSTEM_LUMORIAN || PVictim->m_EcoSystem == SYSTEM_LUMINION ||
				PVictim->m_EcoSystem == SYSTEM_UNDEAD || PVictim->PMaster != NULL){
				PVictim->StatusEffectContainer->AddStatusEffect(
					new CStatusEffect(EFFECT_BIND, EFFECT_BIND, 1, 0, 5));
				return;
			}
			// cannot charm NM
			if (((CMobEntity*)PVictim)->m_Type & MOBTYPE_NOTORIOUS){
				return;
			}
			// mob is charmable
			uint16 baseExp = charutils::GetRealExp(PCharmer->GetMLevel(), PVictim->GetMLevel());
			if (baseExp >= 400) {//IT
				CharmTime = 22500;
				base = 90;
			}
			else if (baseExp >= 240) {//VT
				CharmTime = 45000;
				base = 75;
			}
			else if (baseExp >= 120) {//T
				CharmTime = 90000;
				base = 60;
			}
			else if (baseExp == 100) {//EM
				CharmTime = 180000;
				base = 40;
			}
			else if (baseExp >= 75) {//DC
				CharmTime = 600000;
				base = 30;
			}
			else if (baseExp >= 15) {//EP
				CharmTime = 1200000;
				base = 20;
			}
			else if (baseExp == 0) {//TW
				CharmTime = 1800000;
				base = 10;
			}
			//apply charm time extension from gear
			uint8 charmModValue = (PCharmer->getMod(MOD_CHARM_TIME));
			// adds 5% increase
			float extraCharmTime = (float)(CharmTime*(charmModValue * 0.5f) / 10);
			CharmTime += extraCharmTime;
			//randomize charm time if > EM
			if (baseExp > 100)
			{
				CharmTime += (float)(CharmTime*(((WELL512::irand() % 50) - 25) * 0.1f) / 10);
			}
			if (TryCharm(PCharmer, PVictim, base) == false)
			{
				//player failed to charm mob - agro mob
				battleutils::ClaimMob(PVictim, PCharmer);
				return;
			}
		}
		else if (PVictim->objtype == TYPE_PC)
		{
			//TODO: calculate time mob charms player for and work out a reliable base
			base = 50;
			//mob failed to charm player
			if (TryCharm(PCharmer, PVictim, base) == false)
			{
				return;
			}
		}
		//Charm is a success - take control of charmed Entity
		PVictim->isCharmed = true;
		PVictim->PMaster = PCharmer;
		PCharmer->PPet = PVictim;
		if (PVictim->objtype == TYPE_MOB)
		{
			//make the mob disengage
			if (PCharmer->PPet->PBattleAI != NULL && PCharmer->PPet->PBattleAI->GetCurrentAction() == ACTION_ENGAGE){
				PCharmer->PPet->PBattleAI->SetCurrentAction(ACTION_DISENGAGE);
			}
			//clear the victims emnity list
			((CMobEntity*)PVictim)->PEnmityContainer->Clear();
			//cancel the mobs mobBattle ai
			delete PCharmer->PPet->PBattleAI;
			//set the mobs ai to petAi
			PCharmer->PPet->PBattleAI = new CAIPetDummy((CPetEntity*)PVictim);
			PCharmer->PPet->PBattleAI->SetLastActionTime(gettick());
			PCharmer->PPet->charmTime = gettick() + CharmTime;
			// this will make him transition back to roaming if sleeping
			PCharmer->PPet->animation = ANIMATION_NONE;
			// only move to roaming action if not asleep
			if (!PCharmer->PPet->StatusEffectContainer->HasPreventActionEffect())
			{
				PCharmer->PPet->PBattleAI->SetCurrentAction(ACTION_ROAMING);
			}
			charutils::BuildingCharPetAbilityTable((CCharEntity*)PCharmer, (CPetEntity*)PVictim, PVictim->id);
			((CCharEntity*)PCharmer)->pushPacket(new CCharUpdatePacket((CCharEntity*)PCharmer));
			((CCharEntity*)PCharmer)->pushPacket(new CPetSyncPacket((CCharEntity*)PCharmer));
			PVictim->loc.zone->PushPacket(PVictim, CHAR_INRANGE, new CEntityUpdatePacket(PVictim, ENTITY_UPDATE, UPDATE_COMBAT));
			PVictim->allegiance = ALLEGIANCE_PLAYER;
			((CMobEntity*)PVictim)->m_OwnerID.clean();
		}
		else if (PVictim->objtype == TYPE_PC)
		{
			//TODO: mob take control of player
		}
	}
	/************************************************************************
	* *
	* calculate if charm is successful *
	* *
	************************************************************************/
	bool TryCharm(CBattleEntity* PCharmer, CBattleEntity* PVictim, uint32 base)
	{
		//---------------------------------------------------------
		// chance of charm is based on:
		// -CHR - both entities
		// -Victims M level
		// -charmers BST level (not main level)
		//
		// -75 with a BST SJ lvl10 will struggle on EP
		// -75 with a BST SJ lvl75 will not - thats player has bst leveled to 75 and is using it as SJ
		//---------------------------------------------------------
		uint8 charmerBSTlevel = 0;
		if (PCharmer->objtype == TYPE_PC)
			charmerBSTlevel = ((CCharEntity*)PCharmer)->jobs.job[JOB_BST];
		else if (PCharmer->objtype == TYPE_MOB)
			charmerBSTlevel = PCharmer->GetMLevel();
		float check = base;
		float levelRatio = (float)(PVictim->GetMLevel()) / charmerBSTlevel;
		check *= levelRatio;
		float chrRatio = (float)PVictim->CHR() / PCharmer->CHR();
		check *= chrRatio;
		float charmChanceMods = PCharmer->getMod(MOD_CHARM_CHANCE);
		check *= ((float)((100.0f - charmChanceMods) / 100.0f));
		//cap chance at 95%
		if (check < 5) {
			check = 5;
		}
		if (check < WELL512::irand() % 100) {
			return true;
		}
		return false;
	}
	/************************************************************************
	* *
	* get corsair roll effect *
	* *
	************************************************************************/
	EFFECT getCorsairRollEffect(uint16 id)
	{
		switch (id)
		{
		case ABILITY_FIGHTERS_ROLL: return EFFECT_FIGHTERS_ROLL;
		case ABILITY_MONKS_ROLL:	return EFFECT_MONKS_ROLL;
		case ABILITY_HEALERS_ROLL:	return EFFECT_HEALERS_ROLL;
		case ABILITY_WIZARDS_ROLL:	return EFFECT_WIZARDS_ROLL;
		case ABILITY_WARLOCKS_ROLL:	return EFFECT_WARLOCKS_ROLL;
		case ABILITY_ROGUES_ROLL:	return EFFECT_ROGUES_ROLL;
		case ABILITY_GALLANTS_ROLL:	return EFFECT_GALLANTS_ROLL;
		case ABILITY_CHAOS_ROLL:	return EFFECT_CHAOS_ROLL;
		case ABILITY_BEAST_ROLL:	return EFFECT_BEAST_ROLL;
		case ABILITY_CHORAL_ROLL:	return EFFECT_CHORAL_ROLL;
		case ABILITY_HUNTERS_ROLL:	return EFFECT_HUNTERS_ROLL;
		case ABILITY_SAMURAI_ROLL:	return EFFECT_SAMURAI_ROLL;
		case ABILITY_NINJA_ROLL:	return EFFECT_NINJA_ROLL;
		case ABILITY_DRACHEN_ROLL:	return EFFECT_DRACHEN_ROLL;
		case ABILITY_EVOKERS_ROLL:	return EFFECT_EVOKERS_ROLL;
		case ABILITY_MAGUSS_ROLL:	return EFFECT_MAGUSS_ROLL;
		case ABILITY_CORSAIRS_ROLL:	return EFFECT_CORSAIRS_ROLL;
		case ABILITY_PUPPET_ROLL:	return EFFECT_PUPPET_ROLL;
		case ABILITY_DANCERS_ROLL:	return EFFECT_DANCERS_ROLL;
		case ABILITY_SCHOLARS_ROLL:	return EFFECT_SCHOLARS_ROLL;
		}
		//Unhandled Scenario
		DSP_DEBUG_BREAK_IF(true);
		return EFFECT_BUST;
	}
	void ClaimMob(CBattleEntity* PDefender, CBattleEntity* PAttacker)
	{
		if (PDefender->objtype == TYPE_MOB)
		{
			CMobEntity* mob = (CMobEntity*)PDefender;
			if (PAttacker->objtype != TYPE_PC){
				if (PAttacker->PMaster != NULL)
				{
					// claim by master
					PAttacker = PAttacker->PMaster;
				}
				else
				{
					ShowError("battleutils::ClaimMob Mob being claimed by non-player\n");
				}
			}
			mob->PEnmityContainer->AddBaseEnmity(PAttacker);
			mob->m_OwnerID.id = PAttacker->id;
			mob->m_OwnerID.targid = PAttacker->targid;
		}
	}
	int32 BreathDmgTaken(CBattleEntity* PDefender, int32 damage)
	{
		float resist = 1.0f + (PDefender->getMod(MOD_UDMGBREATH) / 100.0f);
		damage *= resist;
		resist = 1.0f + (PDefender->getMod(MOD_DMGBREATH) / 100.0f) + (PDefender->getMod(MOD_DMG) / 100.0f);
		if (resist < 0.5f)
			resist = 0.5f;
		damage = damage * resist;
		// Handle Severe Damage Reduction Effects
		damage = HandleSevereDamage(PDefender, damage);
		return damage;
	}
	int32 MagicDmgTaken(CBattleEntity* PDefender, int32 damage)
	{
		float resist = (256 + PDefender->getMod(MOD_UDMGMAGIC)) / 256.0f;
		damage *= resist;
		resist = ((256 + PDefender->getMod(MOD_DMGMAGIC)) / 256.0f) + (PDefender->getMod(MOD_DMG) / 100.0f);
		if (resist < 0.5f)
		{
			resist = 0.5f;
		}
		damage = damage * resist;
		// Handle Severe Damage Reduction Effects
		damage = HandleSevereDamage(PDefender, damage);
		return damage;
	}
	int32 PhysicalDmgTaken(CBattleEntity* PDefender, int32 damage)
	{
		float resist = 1.0f + (PDefender->getMod(MOD_UDMGPHYS) / 100.0f);
		damage *= resist;
		resist = 1.0f + (PDefender->getMod(MOD_DMGPHYS) / 100.0f) + (PDefender->getMod(MOD_DMG) / 100.0f);
		if (resist < 0.5f)
		{
			resist = 0.5f;
		}
		damage = damage * resist;
		// Handle Severe Damage Reduction Effects
		damage = HandleSevereDamage(PDefender, damage);
		// Handle Fan Dance reduction effects
		damage = HandleFanDance(PDefender, damage);
		return damage;
	}
	int32 RangedDmgTaken(CBattleEntity* PDefender, int32 damage)
	{
		float resist = 1.0f + (PDefender->getMod(MOD_UDMGRANGE) / 100.0f);
		damage *= resist;
		resist = 1.0f + (PDefender->getMod(MOD_DMGRANGE) / 100.0f) + (PDefender->getMod(MOD_DMG) / 100.0f);
		if (resist < 0.5f)
		{
			resist = 0.5f;
		}
		damage = damage * resist;
		// Handle Severe Damage Reduction Effects
		damage = HandleSevereDamage(PDefender, damage);
		// Handle Fan Dance reduction effects
		damage = HandleFanDance(PDefender, damage);
		return damage;
	}
	void HandleIssekiganEnmityBonus(CBattleEntity* PDefender, CMobEntity* PAttacker){
		if (PDefender->StatusEffectContainer->HasStatusEffect(EFFECT_ISSEKIGAN)){
			// Issekigan is Known to Grant 300 CE per parry, but unknown how it effects VE (per bgwiki). So VE is left alone for now.
			PAttacker->PEnmityContainer->UpdateEnmity(PDefender, 300, 0, false);
		}
	}
	void HandleAfflatusMiseryAccuracyBonus(CBattleEntity* PAttacker){
		if (PAttacker->StatusEffectContainer->HasStatusEffect(EFFECT_AFFLATUS_MISERY) &&
			PAttacker->StatusEffectContainer->HasStatusEffect(EFFECT_AUSPICE)){
			// We keep track of the running total of Accuracy Bonus as part of the Sub Power of the Effect
			// This is used to re-adjust MOD_ACC when the effect wears off
			uint16 accBonus = PAttacker->StatusEffectContainer->GetStatusEffect(EFFECT_AFFLATUS_MISERY)->GetSubPower();
			// Per BGWiki, this bonus is thought to cap at +30
			if (accBonus < 30) {
				accBonus = accBonus + 10;
				PAttacker->StatusEffectContainer->GetStatusEffect(EFFECT_AFFLATUS_MISERY)->SetSubPower(accBonus);
				// Update the Accuracy Modifer as well, so that this is reflected
				// throughout the battle system
				PAttacker->addModifier(MOD_ACC, 10);
			}
		}
	}
	void HandleAfflatusMiseryDamage(CBattleEntity* PDefender, int32 damage)
	{
		if (PDefender->StatusEffectContainer->HasStatusEffect(EFFECT_AFFLATUS_MISERY)){
			PDefender->setModifier(MOD_AFFLATUS_MISERY, damage);
		}
	}
	void HandleTacticalParry(CBattleEntity* PEntity){
		for (uint8 j = 0; j < PEntity->TraitList.size(); ++j)
		{
			CTrait* PExistingTrait = PEntity->TraitList.at(j);
			if (PExistingTrait->getID() == TRAIT_TACTICAL_PARRY){
				int16 tpBonus = PEntity->getMod(MOD_TACTICAL_PARRY);
				//ShowDebug(CL_CYAN"HandleTacticalParry: Tactical Parry Tp Bonus = %d\n" CL_RESET, tpBonus);
				PEntity->addTP(tpBonus);
			}
		}
	}
	float HandleTranquilHeart(CBattleEntity* PEntity){
		//ShowDebug(CL_CYAN"HandleTranquilHeart: Checking For Tranquil Heart...\n" CL_RESET);
		float reductionPercent = 0;
		for (uint8 j = 0; j < PEntity->TraitList.size(); ++j)
		{
			CTrait* PExistingTrait = PEntity->TraitList.at(j);
			// Checks for Tranquil Heart Trait
			if (PExistingTrait->getID() == 114){ // Trait 114 = Tranquil Heart
				int16 healingSkill = PEntity->GetSkill(SKILL_HEA);
				reductionPercent = ((healingSkill / 10) * .5);
				// Reduction Percent Caps at 25%
				if (reductionPercent > 25){
					reductionPercent = 25;
				}
				//ShowDebug(CL_CYAN"HandleTranquilHeart: Tranquil Heart is Active! Reduction Percent = %f\n" CL_RESET, reductionPercent);
				reductionPercent = reductionPercent / 100;
			}
		}
		return reductionPercent;
	}
	int32 HandleStoneskin(CBattleEntity* PDefender, int32 damage)
	{
		int16 skin = PDefender->getMod(MOD_STONESKIN);
		if (damage > 0 && skin > 0)
		{
			if (skin > damage)
			{
				PDefender->delModifier(MOD_STONESKIN, damage);
				return 0;
			}
			PDefender->StatusEffectContainer->DelStatusEffect(EFFECT_STONESKIN);
			return damage - skin;
		}
		return damage;
	}
	int32 HandleSevereDamage(CBattleEntity* PDefender, int32 damage){
		damage = HandleSevereDamageEffect(PDefender, EFFECT_MIGAWARI, damage, true);
		// In the future, handle other Severe Damage Effects like Scherzo & Earthen Armor here
		return damage;
	}
	int32 HandleFanDance(CBattleEntity* PDefender, int32 damage)
	{
		// Handle Fan Dance
		if (PDefender->StatusEffectContainer->HasStatusEffect(EFFECT_FAN_DANCE))
		{
			int power = PDefender->StatusEffectContainer->GetStatusEffect(EFFECT_FAN_DANCE)->GetPower();
			float resist = 1.0f - (power / 100.0f);
			damage *= resist;
			if (power > 20)
			{
				// reduce fan dance effectiveness by 10% each hit, to a min of 20%
				PDefender->StatusEffectContainer->GetStatusEffect(EFFECT_FAN_DANCE)->SetPower(power - 10);
			}
		}
		return damage;
	}
	int32 HandleSevereDamageEffect(CBattleEntity* PDefender, EFFECT effect, int32 damage, bool removeEffect){
		if (PDefender->StatusEffectContainer->HasStatusEffect(effect)){
			int32 maxHp = PDefender->GetMaxHP();
			// The Threshold for Damage is Stored in the Effect Power
			float threshold = (PDefender->StatusEffectContainer->GetStatusEffect(effect)->GetPower() / 100.00);
			// We calcluate the Damage Threshold off of Max HP & the Threshold Percentage
			float damageThreshold = maxHp * threshold;
			//ShowDebug(CL_CYAN"HandleSevereDamageEffect: Severe Damage Occurred! Damage = %d, Threshold = %f, Damage Threshold = %f\n" CL_RESET, damage, threshold, damageThreshold);
			// Severe Damage is when the Attack's Damage Exceeds a Certain Threshold
			if (damage > damageThreshold){
				uint16 severeReduction = PDefender->StatusEffectContainer->GetStatusEffect(effect)->GetSubPower();
				severeReduction = dsp_cap((100 - severeReduction), 0, 100) / 100;
				damage = damage * severeReduction;
				if (removeEffect){
					PDefender->StatusEffectContainer->DelStatusEffect(effect);
				}
				//ShowDebug(CL_CYAN"HandleSevereDamageEffect: Reduciing Severe Damage!\n" CL_RESET);
			}
		}
		//ShowDebug(CL_CYAN"HandleSevereDamageEffect: NOT Reducing Severe Damage!\n" CL_RESET);
		return damage;
	}
	/************************************************************************
	* *
	* get mobs 2 hour skills (should be moved into mobskill.cpp) *
	* *
	************************************************************************/
	CMobSkill* GetTwoHourMobSkill(JOBTYPE job)
	{
		uint16 id = 0;
		switch (job)
		{
		case JOB_WAR: id = 432; break;
		case JOB_MNK: id = 434; break;
		case JOB_WHM: id = 433; break;
		case JOB_BLM: id = 435; break;
		case JOB_RDM: id = 436; break;
		case JOB_THF: id = 437; break;
		case JOB_PLD: id = 438; break;
		case JOB_DRK: id = 439; break;
		case JOB_BST: id = 484; break;
		case JOB_BRD: id = 440; break;
		case JOB_RNG: id = 479; break;
		case JOB_SAM: id = 474; break;
		case JOB_NIN: id = 475; break;
		case JOB_DRG: id = 476; break;
			//case JOB_SMN: id = ???; break;
			//case JOB_PUP: id = ???; break;
			//case JOB_BLU: id = ???; break;
			//case JOB_COR: id = ???; break;
			//case JOB_DNC: id = ???; break;
			//case JOB_SCH: id = ???; break;
		default: return NULL;
		}
		return GetMobSkill(id);
	}
	/************************************************************************
	* *
	* handle the /assist command *
	* *
	************************************************************************/
	void assistTarget(CCharEntity* PChar, uint16 TargID)
	{
		// get the player we want to assist
		CBattleEntity* PlayerToAssist = (CBattleEntity*)PChar->GetEntity(TargID, TYPE_MOB | TYPE_PC);
		if (PlayerToAssist != NULL)
		{
			if (PlayerToAssist->objtype == TYPE_PC && PlayerToAssist->m_TargID != 0)
			{
				// get that players target (mob,player,pet only)
				CBattleEntity* EntityToLockon = (CBattleEntity*)PChar->GetEntity(PlayerToAssist->m_TargID, TYPE_MOB | TYPE_PC | TYPE_PET);
				if (EntityToLockon != NULL)
				{
					// lock on to the new target!
					PChar->pushPacket(new CLockOnPacket(PChar, EntityToLockon));
				}
			}
			else if (PlayerToAssist->PBattleAI != NULL && PlayerToAssist->PBattleAI->GetBattleTarget() != NULL)
			{
				// lock on to the new target!
				PChar->pushPacket(new CLockOnPacket(PChar, PlayerToAssist->PBattleAI->GetBattleTarget()));
			}
		}
	}
	uint8 GetSpellAoEType(CBattleEntity* PCaster, CSpell* PSpell)
	{
		if (PSpell->getAOE() == SPELLAOE_RADIAL_ACCE)
			if (PCaster->StatusEffectContainer->HasStatusEffect(EFFECT_ACCESSION))
				return SPELLAOE_RADIAL;
			else
				return SPELLAOE_NONE;
		if (PSpell->getAOE() == SPELLAOE_RADIAL_MANI)
			if (PCaster->StatusEffectContainer->HasStatusEffect(EFFECT_MANIFESTATION))
				return SPELLAOE_RADIAL;
			else
				return SPELLAOE_NONE;
		if (PSpell->getAOE() == SPELLAOE_PIANISSIMO)
			if (PCaster->StatusEffectContainer->HasStatusEffect(EFFECT_PIANISSIMO))
			{
			PCaster->StatusEffectContainer->DelStatusEffect(EFFECT_PIANISSIMO);
			return SPELLAOE_NONE;
			}
			else
				return SPELLAOE_RADIAL;
		if (PSpell->getAOE() == SPELLAOE_DIFFUSION)
			if (PCaster->StatusEffectContainer->HasStatusEffect(EFFECT_DIFFUSION))
				return SPELLAOE_RADIAL;
			else
				return SPELLAOE_NONE;
		return PSpell->getAOE();
	}
	WEATHER GetWeather(CBattleEntity* PEntity, bool ignoreScholar)
	{
		WEATHER scholarSpell = WEATHER_NONE;
		if (PEntity->StatusEffectContainer->HasStatusEffect(EFFECT_FIRESTORM))
			scholarSpell = WEATHER_HOT_SPELL;
		if (PEntity->StatusEffectContainer->HasStatusEffect(EFFECT_RAINSTORM))
			scholarSpell = WEATHER_RAIN;
		if (PEntity->StatusEffectContainer->HasStatusEffect(EFFECT_SANDSTORM))
			scholarSpell = WEATHER_DUST_STORM;
		if (PEntity->StatusEffectContainer->HasStatusEffect(EFFECT_WINDSTORM))
			scholarSpell = WEATHER_WIND;
		if (PEntity->StatusEffectContainer->HasStatusEffect(EFFECT_HAILSTORM))
			scholarSpell = WEATHER_SNOW;
		if (PEntity->StatusEffectContainer->HasStatusEffect(EFFECT_THUNDERSTORM))
			scholarSpell = WEATHER_THUNDER;
		if (PEntity->StatusEffectContainer->HasStatusEffect(EFFECT_AURORASTORM))
			scholarSpell = WEATHER_AURORAS;
		if (PEntity->StatusEffectContainer->HasStatusEffect(EFFECT_VOIDSTORM))
			scholarSpell = WEATHER_GLOOM;
		WEATHER zoneWeather = zoneutils::GetZone(PEntity->getZone())->GetWeather();
		if (ignoreScholar || scholarSpell == WEATHER_NONE || zoneWeather == (scholarSpell + 1)) // Strong weather overwrites scholar spell weak weather
			return zoneWeather;
		else if (scholarSpell == zoneWeather)
			return (WEATHER)(zoneWeather + 1); // Storm spells stack with weather
		else
			return scholarSpell;
	}
	bool WeatherMatchesElement(WEATHER weather, uint8 element)
	{
		switch (element)
		{
		case ELEMENT_NONE:
			return false; // Can't match with no element...
			break;
		case ELEMENT_FIRE:
			switch (weather)
			{
			case WEATHER_HOT_SPELL:
			case WEATHER_HEAT_WAVE:
				return true;
				break;
			default:
				return false;
			}
			break;
		case ELEMENT_EARTH:
			switch (weather)
			{
			case WEATHER_DUST_STORM:
			case WEATHER_SAND_STORM:
				return true;
				break;
			default:
				return false;
			}
			break;
		case ELEMENT_WATER:
			switch (weather)
			{
			case WEATHER_RAIN:
			case WEATHER_SQUALL:
				return true;
				break;
			default:
				return false;
			}
			break;
		case ELEMENT_WIND:
			switch (weather)
			{
			case WEATHER_WIND:
			case WEATHER_GALES:
				return true;
				break;
			default:
				return false;
			}
			break;
		case ELEMENT_ICE:
			switch (weather)
			{
			case WEATHER_SNOW:
			case WEATHER_BLIZZARDS:
				return true;
				break;
			default:
				return false;
			}
			break;
		case ELEMENT_THUNDER:
			switch (weather)
			{
			case WEATHER_THUNDER:
			case WEATHER_THUNDERSTORMS:
				return true;
				break;
			default:
				return false;
			}
			break;
		case ELEMENT_LIGHT:
			switch (weather)
			{
			case WEATHER_AURORAS:
			case WEATHER_STELLAR_GLARE:
				return true;
				break;
			default:
				return false;
			}
			break;
		case ELEMENT_DARK:
			switch (weather)
			{
			case WEATHER_GLOOM:
			case WEATHER_DARKNESS:
				return true;
				break;
			default:
				return false;
			}
			break;
		default:
			return false;
		}
	}
	void DrawIn(CBattleEntity* PEntity, CMobEntity* PMob, float offset)
	{
		if (PMob->getMobMod(MOBMOD_DRAW_IN) > 0)
		{
			position_t* pos = &PMob->loc.p;
			position_t nearEntity = nearPosition(*pos, offset, M_PI);
			float drawInDistance = (PMob->getMobMod(MOBMOD_DRAW_IN) > 1 ? PMob->getMobMod(MOBMOD_DRAW_IN) : PMob->m_ModelSize * 2);
			// check if i should draw-in party/alliance
			if (PMob->getMobMod(MOBMOD_DRAW_IN) > 1 && PEntity->PParty != NULL)
			{
				// party draw-in
				if (PEntity->PParty->m_PAlliance == NULL)
				{
					for (uint8 i = 0; i < PEntity->PParty->members.size(); ++i)
					{
						CBattleEntity* PMember = (CBattleEntity*)PEntity->PParty->members[i];
						float pDistance = distance(PMob->loc.p, PMember->loc.p);
						if (PMob->loc.zone == PMember->loc.zone && pDistance > drawInDistance && PMember->status != STATUS_CUTSCENE_ONLY)
						{
							// don't draw in dead players for now!
							// see tractor
							if (PMember->isDead() || PMember->animation == ANIMATION_CHOCOBO)
							{
								// don't do anything
							}
							else
							{
								// draw in!
								PMember->loc.p.x = nearEntity.x;
								PMember->loc.p.y = nearEntity.y;
								PMember->loc.p.z = nearEntity.z;
								if (PMember->objtype == TYPE_PC)
								{
									CCharEntity* PChar = (CCharEntity*)PMember;
									PChar->pushPacket(new CPositionPacket(PChar));
								}
								else
								{
									PMember->loc.zone->PushPacket(PMember, CHAR_INRANGE, new CEntityUpdatePacket(PMember, ENTITY_UPDATE, UPDATE_POS));
								}
								luautils::OnMobDrawIn(PMob, PMember);
								PMob->loc.zone->PushPacket(PMob, CHAR_INRANGE, new CMessageBasicPacket(PMember, PMember, 0, 0, 232));
							}
						}
					}
				}
				// alliance draw-in
				else
				{
					// find all parties present in alliance
					for (uint8 i = 0; i < PEntity->PParty->m_PAlliance->partyList.size(); ++i)
					{
						CParty* PParty = PEntity->PParty->m_PAlliance->partyList[i];
						// find all members in that party
						for (uint8 m = 0; m < PParty->members.size(); ++m)
						{
							CBattleEntity* PMember = PParty->members[m];
							float pDistance = distance(PMob->loc.p, PMember->loc.p);
							// ensure target is in zone before drawing them in, cannot draw-in if target is watching a cutscene
							if (PMob->loc.zone == PMember->loc.zone && pDistance > drawInDistance && PMember->status != STATUS_CUTSCENE_ONLY)
							{
								// don't draw in dead players for now!
								// see tractor
								if (PMember->isDead() || PMember->animation == ANIMATION_CHOCOBO)
								{
									// don't do anything
								}
								else
								{
									// draw in!
									PMember->loc.p.x = nearEntity.x;
									PMember->loc.p.y = nearEntity.y;
									PMember->loc.p.z = nearEntity.z;
									if (PMember->objtype == TYPE_PC)
									{
										CCharEntity* PChar = (CCharEntity*)PMember;
										PChar->pushPacket(new CPositionPacket(PChar));
									}
									else
									{
										PMember->loc.zone->PushPacket(PMember, CHAR_INRANGE, new CEntityUpdatePacket(PMember, ENTITY_UPDATE, UPDATE_POS));
									}
									luautils::OnMobDrawIn(PMob, PMember);
									PMob->loc.zone->PushPacket(PMob, CHAR_INRANGE, new CMessageBasicPacket(PMember, PMember, 0, 0, 232));
								}
							}
						}
					}
				}
			}
			// no party present or draw-in is set to target only
			else
			{
				if (PEntity->status != STATUS_CUTSCENE_ONLY)
				{
					// don't draw in dead players for now!
					// see tractor
					if (PEntity->isDead() || PEntity->animation == ANIMATION_CHOCOBO)
					{
						// don't do anything
					}
					else
					{
						// draw in!
						PEntity->loc.p.x = nearEntity.x;
						PEntity->loc.p.y = nearEntity.y;
						PEntity->loc.p.z = nearEntity.z;
						if (PEntity->objtype == TYPE_PC)
						{
							CCharEntity* PChar = (CCharEntity*)PEntity;
							PChar->pushPacket(new CPositionPacket(PChar));
						}
						else
						{
							PEntity->loc.zone->PushPacket(PEntity, CHAR_INRANGE, new CEntityUpdatePacket(PEntity, ENTITY_UPDATE, UPDATE_POS));
						}
						luautils::OnMobDrawIn(PMob, PEntity);
						PMob->loc.zone->PushPacket(PMob, CHAR_INRANGE, new CMessageBasicPacket(PEntity, PEntity, 0, 0, 232));
					}
				}
			}
		}
	}
	/************************************************************************
	* *
	* Does the wild card effect to a specific character *
	* *
	************************************************************************/
	void DoWildCardToEntity(CCharEntity* PCaster, CCharEntity* PTarget, uint8 roll)
	{
		uint8 TotalRecasts = PTarget->PRecastContainer->GetRecastList(RECAST_ABILITY)->size();
		// Don't count the 2hr.
		if (PTarget->PRecastContainer->Has(RECAST_ABILITY, 0))
		{
			TotalRecasts -= 1;
		}
		if (TotalRecasts == 0)
		{
			return;
		}
		// Restore some abilities (Randomly select some abilities?)
		uint8 RecastsToDelete = WELL512::irand() % TotalRecasts;
		// Restore at least 1 ability.
		RecastsToDelete = RecastsToDelete == 0 ? 1 : RecastsToDelete;
		switch (roll)
		{
		case 1:
			// Restores some Job Abilities (does not restore One Hour Abilities)
			for (uint8 i = RecastsToDelete; i > 0; --i)
			{
				if (PTarget->PRecastContainer->GetRecastList(RECAST_ABILITY)->at(i - 1)->ID != 0)
				{
					PTarget->PRecastContainer->DeleteByIndex(RECAST_ABILITY, i - 1);
				}
			}
			break;
		case 2:
			// Restores all Job Abilities (does not restore One Hour Abilities)
			PTarget->PRecastContainer->ResetAbilities();
			break;
		case 3:
			// Restores some Job Abilities (does not restore One Hour Abilities), 100% TP Restore
			for (uint8 i = RecastsToDelete; i > 0; --i)
			{
				if (PTarget->PRecastContainer->GetRecastList(RECAST_ABILITY)->at(i - 1)->ID != 0)
				{
					PTarget->PRecastContainer->DeleteByIndex(RECAST_ABILITY, i - 1);
				}
			}
			PTarget->health.tp = 1000;
			break;
		case 4:
			// Restores all Job Abilities (does not restore One Hour Abilities), 300% TP Restore
			PTarget->PRecastContainer->ResetAbilities();
			PTarget->health.tp = 3000;
			break;
		case 5:
			// Restores some Job Abilities and One Hour Abilities (Not Wild Card though), 50% MP Restore
			for (uint8 i = RecastsToDelete; i > 0; --i)
			{
				if (PTarget->PRecastContainer->GetRecastList(RECAST_ABILITY)->at(i - 1)->ID != 0)
				{
					PTarget->PRecastContainer->DeleteByIndex(RECAST_ABILITY, i - 1);
				}
			}
			// Retore 2hr except for Wildcard.
			if (PTarget != PCaster)
			{
				PTarget->PRecastContainer->Del(RECAST_ABILITY, 0);
			}
			if (PTarget->health.maxmp > 0 && (PTarget->health.mp < (PTarget->health.maxmp / 2)))
			{
				PTarget->health.mp = PTarget->health.maxmp / 2;
			}
			break;
		case 6:
			// Restores all Job Abilities and One Hour Abilities (Not Wild Card though), 100% MP Restore
			if (PCaster == PTarget)
			{
				PTarget->PRecastContainer->ResetAbilities();
			}
			else
			{
				PTarget->PRecastContainer->Del(RECAST_ABILITY);
			}
			PTarget->addMP(PTarget->health.maxmp);
			break;
		}
	}
	/************************************************************************
	* *
	* Get the Snapshot shot time reduction *
	* *
	************************************************************************/
	void GetSnapshotReduction(CCharEntity* m_PChar)
	{
		// Set this to zero to start with
		uint32 SnapShotReductionPercent = 0;
		// Reduction from gear.
		SnapShotReductionPercent += m_PChar->getMod(MOD_SNAP_SHOT);
		// Reduction from merits.
		if (charutils::hasTrait(m_PChar, TRAIT_SNAPSHOT))
		{
			SnapShotReductionPercent += m_PChar->PMeritPoints->GetMeritValue(MERIT_SNAPSHOT, m_PChar);
		}
		// Reduction from velocity shot mod
		if (m_PChar->StatusEffectContainer->HasStatusEffect(EFFECT_VELOCITY_SHOT))
		{
			SnapShotReductionPercent += m_PChar->getMod(MOD_VELOCITY_SNAPSHOT_BONUS);
		}
		// Only apply if we have snapshot bonus to offer.
		if (SnapShotReductionPercent > 0)
		{
			m_PChar->m_rangedDelay -= (float)(m_PChar->m_rangedDelay * ((float)SnapShotReductionPercent / 100));
		}
	}
	/************************************************************************
	* *
	* Get any ranged attack bonuses here *
	* *
	************************************************************************/
	int32 GetRangedAttackBonuses(CBattleEntity* battleEntity)
	{
		if (battleEntity->objtype != TYPE_PC)
		{
			return 0;
		}
		int32 bonus = 0;
		// Reduction from velocity shot mod
		if (battleEntity->StatusEffectContainer->HasStatusEffect(EFFECT_VELOCITY_SHOT))
		{
			bonus += battleEntity->getMod(MOD_VELOCITY_RATT_BONUS);
		}
		return bonus;
	}
	/************************************************************************
	* *
	* Get any ranged accuracy bonuses here *
	* *
	************************************************************************/
	int32 GetRangedAccuracyBonuses(CBattleEntity* battleEntity)
	{
		if (battleEntity->objtype != TYPE_PC)
		{
			return 0;
		}
		int32 bonus = 0;
		// Bonus from barrage mod
		if (battleEntity->StatusEffectContainer->HasStatusEffect(EFFECT_BARRAGE))
		{
			bonus += battleEntity->getMod(MOD_BARRAGE_ACC);
		}
		return bonus;
	}
};