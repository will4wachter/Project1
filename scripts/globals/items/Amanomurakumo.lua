-----------------------------------------
-- ID: 18312
-- Item: Amanomurakumo
-- Additional Effect: Weakens Attacks
-----------------------------------------
require("scripts/globals/status");
require("scripts/globals/magic");

-----------------------------------
-- onAdditionalEffect Action
-----------------------------------
function onAdditionalEffect(player,target,damage)
    local chance = 84;
    if (math.random(0,99) >= chance ) then
        target:addStatusEffect(EFFECT_ATTACK_DOWN, 20, 0, 60);
        return SUBEFFECT_DEFENSE_DOWN, 160, EFFECT_ATTACK_DOWN;
    else
        return 0,0,0;
    end
end;
