-----------------------------------------
-- ID: 18300
-- Item: GUNGNIR
-- Additional Effect: Weakens Defense
-----------------------------------------
require("scripts/globals/status");
require("scripts/globals/magic");

-----------------------------------
-- onAdditionalEffect Action
-----------------------------------
function onAdditionalEffect(player,target,damage)
    local chance = 84;
    if (math.random(0,99) >= chance ) then
        target:addStatusEffect(EFFECT_DEFENSE_DOWN, 20, 0, 60);
        return SUBEFFECT_DEFENSE_DOWN, 160, EFFECT_DEFENSE_DOWN;
    else
        return 0,0,0;
    end
end;
