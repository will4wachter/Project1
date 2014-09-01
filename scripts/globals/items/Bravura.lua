-----------------------------------------
-- ID: 18642
-- Item: Bravura
-- Additional Effect: EVASION DOWN
-----------------------------------------
require("scripts/globals/status");
require("scripts/globals/magic");

-----------------------------------
-- onAdditionalEffect Action
-----------------------------------
function onAdditionalEffect(player,target,damage)
    local chance = 84;
    if (math.random(0,99) >= chance ) then
        target:addStatusEffect(EFFECT_EVASION_DOWN, 20, 0, 60);
        return SUBEFFECT_DEFENSE_DOWN, 160, EFFECT_EVASION_DOWN;
    else
        return 0,0,0;
    end
end;
