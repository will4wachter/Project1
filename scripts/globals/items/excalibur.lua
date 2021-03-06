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
    local chance = 84;

    if (math.random(2,99) >= chance) then
        local damage = (player:getHP()*.25);
        local params = {};
        return SUBEFFECT_LIGHT_DAMAGE, 163, target:addHP(-damage);
    else
	return 0,0,0;		
    end
end;