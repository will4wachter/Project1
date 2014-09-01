-----------------------------------------
-- ID: 18312
-- Item: Kikoku
-- Additional Effect: Paralysis
-----------------------------------------
require("scripts/globals/status");
require("scripts/globals/magic");

-----------------------------------
-- onAdditionalEffect Action
-----------------------------------
function onAdditionalEffect(player,target,damage)
    local chance = 84;
    if (math.random(0,99) >= chance ) then
        target:addStatusEffect(EFFECT_PARALYSIS, 10, 0, 60);
        return SUBEFFECT_PARALYSIS, 160, EFFECT_PARALYSIS;
    else
        return 0,0,0;
    end
end;
