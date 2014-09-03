-----------------------------------------
-- ID: 17545
-- Item: Earth Staff
-- Additional Effect: Earth Damage
-----------------------------------------
require("scripts/globals/status");

-----------------------------------
-- onAdditionalEffect Action
-----------------------------------
function onAdditionalEffect(player,target,damage)
    local chance = 90;
    if (target:getMainLvl() > player:getMainLvl()) then
        chance = chance - 5 * (target:getMainLvl() - player:getMainLvl())
        chance = utils.clamp(chance, 5, 95);
    end
    if (math.random(0,99) >= chance) then 
        return 0,0,0;
    else
        local diff = player:getStat(MOD_INT) - target:getStat(MOD_INT);
        if (diff > 20) then
            diff = 20 + (diff - 20) / 2;
        end
        local damage = diff + (player:getMainLvl() - target:getMainLvl()) + damage/2;
        local params = {};
        params.bonusmab = 0;
        params.includemab = false;
        damage = finalMagicNonSpellAdjustments(player,target,ELE_EARTH,damage);
        return SUBEFFECT_EARTH_DAMAGE, 163, player:addHP(-damage);
    end
end;