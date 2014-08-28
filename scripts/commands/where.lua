---------------------------------------------------------------------------------------------------
-- func: where
-- auth: <Unknown>
-- desc: Tells the player about their current position.
---------------------------------------------------------------------------------------------------

cmdprops =
{
    permission = 3,
    parameters = ""
};

function onTrigger(player)
    player:showPosition();
end