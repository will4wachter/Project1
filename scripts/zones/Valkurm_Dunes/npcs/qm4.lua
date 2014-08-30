-----------------------------------
-- Area: Valkurm Dunes
-- NPC:  qm4 (???)
-- Involved in quest: Pirate's Chart
-- @pos -160 4 -131 103
-----------------------------------
package.loaded["scripts/zones/Valkurm_Dunes/TextIDs"] = nil;
-----------------------------------

require("scripts/zones/Valkurm_Dunes/TextIDs");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
	-- Trade Anniversary Ring
		-- Trade Anniversary Ring
	if(trade:hasItemQty(15793,1) and trade:getItemCount() == 1) then
			player:tradeComplete();
			player:addStatusEffect(EFFECT_DEDICATION,1000,0,3600,0,30000);
			
	end

end;

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)
	player:messageSpecial(MONSTERS_KILLED_ADVENTURERS);
end;

-----------------------------------
-- onEventUpdate
-----------------------------------

function onEventUpdate(player,csid,option)
-- printf("CSID: %u",csid);
-- printf("RESULT: %u",option);
end;

-----------------------------------
-- onEventFinish
-----------------------------------

function onEventFinish(player,csid,option)
-- printf("CSID: %u",csid);
-- printf("RESULT: %u",option);
end;