---------------------------------------------------------------------------------------------------
-- func: reset
-- auth: <Unknown>
-- desc: Resets the players recasts.
---------------------------------------------------------------------------------------------------

cmdprops =
{
    permission = 5,
    parameters = ""
};

function onTrigger(player)
    player:resetRecasts();
end