---------------------------------------------
--  Hexidiscs
--
--  Description: A sixfold attack damages targets in a fan-shaped area of effect.
--  Type: Physical
--  Utsusemi/Blink absorb: 6 shadows
--  Range: Unknown cone
--  Notes: Only used in "ball" form.
---------------------------------------------
require("/scripts/globals/settings");
require("/scripts/globals/status");
require("/scripts/globals/monstertpmoves");

---------------------------------------------
function OnMobSkillCheck(target,mob,skill)
	return 0;
end;

function OnMobWeaponSkill(target, mob, skill)
	local numhits = 6;
	local accmod = 1;
	local dmgmod = .7;
	local info = MobPhysicalMove(mob,target,skill,numhits,accmod,dmgmod,TP_NO_EFFECT);
	local dmg = MobFinalAdjustments(info.dmg,mob,skill,target,MOBSKILL_PHYSICAL,MOBPARAM_BLUNT,info.hitslanded);
	target:delHP(dmg);
	return dmg;
end;
