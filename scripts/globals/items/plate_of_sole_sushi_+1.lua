-----------------------------------------
-- ID: 5163
-- Item: plate_of_sole_sushi_+1
-- Food Effect: 60Min, All Races
-----------------------------------------
-- Health 20
-- Strength 5
-- Dexterity 6
-- Accuracy % 16
-- Ranged ACC % 16
-- Sleep Resist 5
-----------------------------------------

require("scripts/globals/status");

-----------------------------------------
-- OnItemCheck
-----------------------------------------

function onItemCheck(target)
local result = 0;
	if (target:hasStatusEffect(EFFECT_FOOD) == true or target:hasStatusEffect(EFFECT_FIELD_SUPPORT_FOOD) == true) then
		result = 246;
	end
return result;
end;

-----------------------------------------
-- OnItemUse
-----------------------------------------

function onItemUse(target)
	target:addStatusEffect(EFFECT_FOOD,0,0,3600,5163);
end;

-----------------------------------
-- onEffectGain Action
-----------------------------------

function onEffectGain(target,effect)
	target:addMod(MOD_HP, 5000);
	target:addMod(MOD_STR, 500);
	target:addMod(MOD_DEX, 500);
	target:addMod(MOD_CRIT_DMG_INCREASE, 100);
	target:addMod(MOD_REGAIN, 100);
	target:addMod(MOD_REGEN,100);
	target:addMod(MOD_HASTE_MAGIC, 100);
	target:addMod(MOD_DMG, -75);
end;

-----------------------------------------
-- onEffectLose Action
-----------------------------------------

function onEffectLose(target,effect)
	target:delMod(MOD_HP, 5000);
	target:delMod(MOD_STR, 500);
	target:delMod(MOD_DEX, 500);
	target:delMod(MOD_CRIT_DMG_INCREASE, 100);
	target:delMod(MOD_REGAIN, 100);
	target:delMod(MOD_REGEN,100);
	target:delMod(MOD_HASTE_MAGIC, 100);
	target:delMod(MOD_DMG, -75);
end;
