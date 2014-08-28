---------------------------------------------------------------------------------------------------
-- func: capallskills
-- auth: <Unknown>
-- desc: Caps all the players skills.
---------------------------------------------------------------------------------------------------

cmdprops =
{
    permission = 5,
    parameters = ""
};

function onTrigger(player)
    player:capAllSkills();
    player:PrintToPlayer( 'All skills capped!' );
end