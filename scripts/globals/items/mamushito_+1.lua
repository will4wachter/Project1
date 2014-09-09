-----------------------------------------
-- ID: 18270
-- Item: Mandau
-- Additional Effect: POISON
-----------------------------------------
require("scripts/globals/status");
require("scripts/globals/magic");

-----------------------------------
-- onAdditionalEffect Action
-----------------------------------
function onAdditionalEffect(player,target,damage)
    local chance = 90;
    if (math.random(0,99) >= chance ) then
        target:addStatusEffect(EFFECT_STUN, 5, 0, 60);
        return SUBEFFECT_STUN, 160, EFFECT_STUN;
    else
        return 0,0,0;
    end
end;
