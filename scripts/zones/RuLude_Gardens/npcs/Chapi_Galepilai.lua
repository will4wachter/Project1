-----------------------------------
--	Area: Ru'Lud Gardens
--	NPC:  Chapi Galepilai
--	Standard Info NPC
-----------------------------------

require("scripts/globals/settings");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
		if(trade:hasItemQty(65535,1) and trade:getItemCount() == 1) then 
			player:setPos(27.424, -6, -123.792, 192, 0x32); -- {R}
		end
end; 

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)
	player:startEvent(0x0094);
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



