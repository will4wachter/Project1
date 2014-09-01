-----------------------------------------
-- ID: 18306
-- Item: Apocalypse
-- Additional Effect: Blindness
-----------------------------------------
require("scripts/globals/status");
require("scripts/globals/magic");

-----------------------------------
-- onAdditionalEffect Action
-----------------------------------
function onAdditionalEffect(player,target,damage)
    local chance = 84;
    if (math.random(0,99) >= chance ) then
        target:addStatusEffect(EFFECT_BLINDNESS, 10, 0, 30);
        return SUBEFFECT_BLIND, 160, EFFECT_BLINDNESS;
    else
        return 0,0,0;
    end
end;
