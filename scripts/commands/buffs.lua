---------------------------------------------------------------------------------------------------
-- func: buffs
-- desc: Toggles Buffs on the player; granting them several special abilities.
---------------------------------------------------------------------------------------------------

cmdprops =
{
    permission = 0,
    parameters = ""
};

function onTrigger(player)
    if (player:getVar("Buff") == 0) then
        -- Toggle Buffs on..
        player:setVar("Buff", 1);
        
        -- Add bonus effects to the player..
        player:addStatusEffect(EFFECT_REGAIN,10,1,0);
        player:addStatusEffect(EFFECT_REFRESH,10,0,0);
        player:addStatusEffect(EFFECT_REGEN,10,0,0);
	player:addStatusEffect(EFFECT_HASTE,100,0,0);
        
        -- Add bonus mods to the player..
        player:addMod(MOD_RACC,150);
        player:addMod(MOD_RATT,150);
        player:addMod(MOD_ACC,150);
        player:addMod(MOD_ATT,150);
        player:addMod(MOD_MATT,150);
        player:addMod(MOD_MACC,150);
        player:addMod(MOD_RDEF,150);
        player:addMod(MOD_DEF,150);
        player:addMod(MOD_MDEF,150);
    else
        -- Toggle Buffs off..
        player:setVar("Buff", 0);

        -- Remove bonus effects..
        player:delStatusEffect(EFFECT_REGAIN);
        player:delStatusEffect(EFFECT_REFRESH);
        player:delStatusEffect(EFFECT_REGEN);
	player:delStatusEffect(EFFECT_HASTE);
        
        player:delMod(MOD_RACC,150);
        player:delMod(MOD_RATT,150);
        player:delMod(MOD_ACC,150);
        player:delMod(MOD_ATT,150);
        player:delMod(MOD_MATT,150);
        player:delMod(MOD_MACC,150);
        player:delMod(MOD_RDEF,150);
        player:delMod(MOD_DEF,150);
        player:delMod(MOD_MDEF,150);
    end	
end;
