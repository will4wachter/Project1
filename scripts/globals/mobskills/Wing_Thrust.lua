---------------------------------------------
--  Wing_Thrust
--
--  Description: Deals damage to a single target. Additional effect: Slow
--  Type: Physical
--  Utsusemi/Blink absorb: 4 shadows
--  Range: Melee
--  Notes:
---------------------------------------------
require("/scripts/globals/settings");
require("/scripts/globals/status");
require("/scripts/globals/monstertpmoves");

---------------------------------------------
function OnMobSkillCheck(target,mob,skill)
	return 0;
end;

function OnMobWeaponSkill(target, mob, skill)

	MobStatusEffectMove(mob, target, EFFECT_SLOW, 30, 0, 120);
	local numhits = 4;
	local accmod = 1;
	local dmgmod = 500;
	local info = MobPhysicalMove(mob,target,skill,numhits,accmod,dmgmod,TP_NO_EFFECT);
	local dmg = MobFinalAdjustments(dmgmod,mob,skill,target,MOBSKILL_PHYSICAL,MOBPARAM_NONE);
	target:delHP(dmg);
	return dmg;
end;