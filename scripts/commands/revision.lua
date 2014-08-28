---------------------------------------------------------------------------------------------------
-- func: revision
-- auth: Whasf
-- desc: Sends the Git revision to the player
---------------------------------------------------------------------------------------------------

cmdprops =
{
    permission = 5,
	parameters = ""
};

function onTrigger(player)
    player:SendRevision();
end