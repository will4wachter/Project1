---------------------------------------------------
-- Auroral Wind
-- Deals Wind damage to enemies within a fan-shaped area originating from the caster. Additional effect: Silence.
---------------------------------------------------

require("/scripts/globals/settings");
require("/scripts/globals/status");
require("/scripts/globals/monstertpmoves");

---------------------------------------------------

function OnMobSkillCheck(target,mob,skill)
	return 0;
end;

function OnMobWeaponSkill(target, mob, skill)


	MobStatusEffectMove(mob, target, EFFECT_SILENCE, 30, 0, 60);

	local dmgmod = 800;
	local accmod = 1;
	local info = MobMagicalMove(mob,target,skill,mob:getWeaponDmg() * 3,accmod,dmgmod,TP_MAB_BONUS,1);
	local dmg = MobFinalAdjustments(dmgmod,mob,skill,target,MOBSKILL_MAGICAL,MOBPARAM_WIND,MOBPARAM_WIPE_SHADOWS);
	target:delHP(dmg);
	return dmg;
end;