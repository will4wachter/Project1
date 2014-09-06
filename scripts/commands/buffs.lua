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
	player:addStatusEffect(EFFECT_ATTACK_BOOST,200,0,0);
	player:addStatusEffect(EFFECT_DEFENSE_BOOST,200,0,0);
        

    else
        -- Toggle Buffs off..
        player:setVar("Buff", 0);

        -- Remove bonus effects..
        player:delStatusEffect(EFFECT_REGAIN);
        player:delStatusEffect(EFFECT_REFRESH);
        player:delStatusEffect(EFFECT_REGEN);
	player:delStatusEffect(EFFECT_HASTE);
	player:delStatusEffect(EFFECT_ATTACK_BOOST);
	player:delStatusEffect(EFFECT_DEFENSE_BOOST);
        
    end	
end;
