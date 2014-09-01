-----------------------------------------
-- ID: 18324
-- Item: Mjillnir
-- Additional Effect: Recover MP
-----------------------------------------
require("scripts/globals/status");
require("scripts/globals/magic");

-----------------------------------
-- onAdditionalEffect Action
-----------------------------------
function onAdditionalEffect(player,target,damage)
    local chance = 80;
    if (math.random(0,99) >= chance ) then
 	local damage = (math.random(4,16));
        return SUBEFFECT_DRAIN, 163, player:addMP(damage);
    else
        return 0,0,0;
    end
end;
