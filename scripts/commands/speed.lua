---------------------------------------------------------------------------------------------------
-- func: speed
-- auth: <Unknown> 
-- desc: Sets the players movement speed.
---------------------------------------------------------------------------------------------------

cmdprops =
{
    permission = 3,
    parameters = "i"
};

function onTrigger(player, speed)
    player:speed( speed );
end