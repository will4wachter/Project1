---------------------------------------------------------------------------------------------------
-- func: delkeyitem
-- auth: <Unknown>
-- desc: Deletes the given key item from the player.
---------------------------------------------------------------------------------------------------

cmdprops =
{
    permission = 5,
    parameters = "i"
};

function onTrigger(player, keyId)
    if (keyId == nil or tonumber(keyId) == 0) then
        player:PrintToPlayer("You must enter a valid keyitem id.");
        return;
    end
    player:delKeyItem( keyId );
end