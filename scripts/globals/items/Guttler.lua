-----------------------------------------
-- ID: 18288
-- Item: GUTTER
-- Additional Effect: CHOKE
-----------------------------------------
require("scripts/globals/status");
require("scripts/globals/magic");

-----------------------------------
-- onAdditionalEffect Action
-----------------------------------
function onAdditionalEffect(player,target,damage)
    local chance = 84;
    if (math.random(0,99) >= chance ) then
        target:addStatusEffect(EFFECT_CHOKE, 20, 0, 60);
        return SUBEFFECT_WIND_DAMAGE, 160, EFFECT_CHOKE;
    else
        return 0,0,0;
    end
end;
