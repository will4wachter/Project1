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
    if player:hasStatusEffect(EFFECT_FOOD) then	
	player:delStatusEffect(EFFECT_FOOD);
	end
end;
