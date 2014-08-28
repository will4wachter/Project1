---------------------------------------------
--  Medusa_Javelin
--
--  Description: Deals damage to a single target. Additional effect: Petrification
--  Type: Physical
--  Utsusemi/Blink absorb: 1 shadow
--  Range: Melee
--  Notes: Absolute Virtue
---------------------------------------------
require("/scripts/globals/settings");
require("/scripts/globals/status");
require("/scripts/globals/monstertpmoves");

---------------------------------------------

function OnMobWeaponSkill(target, mob, skill)

	MobStatusEffectMove(mob, target, EFFECT_PETRIFICATION, 15, 0, 120);
	local numhits = 1;
	local accmod = 1;
	local dmgmod = 2000;
	local info = MobMagicalMove(mob,target,skill,mob:getWeaponDmg()*4,ELE_EARTH,dmgmod,TP_NO_EFFECT);
	local dmg = MobFinalAdjustments(dmgmod,mob,skill,target,MOBSKILL_MAGICAL,MOBPARAM_EARTH,MOBPARAM_IGNORE_SHADOWS);
	target:delHP(dmg);
	return dmg;
end;

