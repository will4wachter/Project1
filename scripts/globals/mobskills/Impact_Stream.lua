---------------------------------------------
--  Impact_Stream
--
--  Impact Stream - 10' AOE damage, Stun  and Defense Down , wipes Shadow Images
--  Type: Magical
--  Utsusemi/Blink absorb: Wipes shadows
--  Range: Less than or equal to 10.0
--  Notes: Absolute Virtue
---------------------------------------------

require("/scripts/globals/settings");
require("/scripts/globals/status");
require("/scripts/globals/monstertpmoves");

---------------------------------------------

function OnMobSkillCheck(target,mob,skill)
	return 0;
end;

function OnMobWeaponSkill(target, mob, skill)

	local typeEffect = EFFECT_STUN, EFFECT_DEFENSE_DOWN;

	MobStatusEffectMove(mob, target, EFFECT_STUN, 10, 0, 10);
	
	MobStatusEffectMove(mob, target, EFFECT_DEFENSE_DOWN, 75, 0, 60);

	local dmgmod = 400;
	local accmod = 1;
	local info = MobMagicalMove(mob,target,skill,mob:getWeaponDmg()*3,accmod,dmgmod,TP_NO_EFFECT);
	local dmg = MobFinalAdjustments(dmgmod,mob,skill,target,MOBSKILL_MAGICAL,MOBPARAM_EARTH,MOBPARAM_WIPE_SHADOWS);
	target:delHP(dmg);
	return dmg;
end;