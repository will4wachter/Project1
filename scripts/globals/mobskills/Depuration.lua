---------------------------------------------
--  Depuration
--  Erases all negative effects on the mob. Aerns will generally not attempt to use this ability if no erasable effects exist on them.
--  Type: Healing
---------------------------------------------

require("/scripts/globals/settings");
require("/scripts/globals/status");
require("/scripts/globals/monstertpmoves");

---------------------------------------------
function OnMobSkillCheck(target,mob,skill)
	return 0;
end;

function OnMobWeaponSkill(target, mob, skill)
    return 0;
end;

