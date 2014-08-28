-----------------------------------
-- Area: The Shrine of Ru'Avitau
-- NPC:  ??? (Spawn Kirin)
-- @pos -81 32 2 178
-----------------------------------
package.loaded["scripts/zones/The_Shrine_of_RuAvitau/TextIDs"] = nil;
-----------------------------------

require("scripts/zones/The_Shrine_of_RuAvitau/TextIDs");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
    -- Ensure Kirin is not alive..
    if (GetMobAction( 17506670 ) == 0) then
        -- Validate traded items are all needed seals..
        if (trade:hasItemQty( 1404, 1 ) and trade:hasItemQty( 1405, 1 ) and trade:hasItemQty( 1406, 1 ) and trade:hasItemQty( 1407, 1 ) and trade:getItemCount() == 4) then
            -- Complete the trade..
            player:tradeComplete();
            
            -- Spawn Kirin..
            local mob = SpawnMob( 17506670, 180 );
            mob:setExtraVar( 0 );
            player:showText( npc, KIRIN_OFFSET );
            mob:updateEnmity( player );
        end    
    end
end;

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)
		stock = {
			 0x57c,3000000,		--Seal of Genbu
			 0x57d,3000000,		--Seal of Seiryu
			 0x57e,3000000,		--Seal of Byakko
			 0x57f,3000000}		--Seal of Suzaku

showShop(player, STATIC ,stock);

end;

-----------------------------------
-- onEventUpdate
-----------------------------------

function onEventUpdate(player,csid,option)
--printf("CSID: %u",csid);
--printf("RESULT: %u",option);
end;

-----------------------------------
-- onEventFinish
-----------------------------------

function onEventFinish(player,csid,option)
--printf("CSID: %u",csid);
--printf("RESULT: %u",option);
end;