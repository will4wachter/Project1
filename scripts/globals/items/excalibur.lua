-----------------------------------------
-- ID: 18276
-- Item: Excalibur
-- Additional Effect: Damage varies with HP
-----------------------------------------
require("scripts/globals/status");
-----------------------------------
-- onAdditionalEffect Action
-----------------------------------
function onAdditionalEffect(player,target,damage)
	local chance = 95;
	if
		local diff = player:getStat(MAXHP)*.25;
		damage =  diff
		return SUBEFFECT_LIGHT_DAMAGE, 163;
	end
end;




