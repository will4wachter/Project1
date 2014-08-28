---------------------------------------------
--  Crystaline Cocoon
--  Shell and Protect.
---------------------------------------------

require("/scripts/globals/settings");
require("/scripts/globals/status");
require("/scripts/globals/monstertpmoves");

---------------------------------------------

function OnMobSkillCheck(target,mob,skill)
	return 0;
end;

function OnMobWeaponSkill(target, mob, skill)

	local typeEffect = EFFECT_SHELL;
	skill:setMsg(MSG_BUFF);
	mob:delStatusEffect(typeEffect);
	mob:addStatusEffect(typeEffect,-128,0,60);--power=-128;tic=0;duration=60;
	
	skill:setMsg(MSG_BUFF);
	local typeEffect = EFFECT_PROTECT;
	mob:delStatusEffect(typeEffect);
	mob:addStatusEffect(typeEffect,50,0,30); -- 50%
	return typeEffect;
end;