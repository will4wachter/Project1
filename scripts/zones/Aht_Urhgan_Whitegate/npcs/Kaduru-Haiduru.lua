-----------------------------------
--	Area: Aht Urgan Whitegate
--	NPC:  Kaduru-Haiduru
--	Working 100%
-----------------------------------

require("scripts/globals/settings");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
	if (trade:hasItemQty(65535,1) and trade:getItemCount() == 1) then 
			player:setPos(-0.041, 2.999, -6.067, 192, 243); -- H-8  {R}
	end 
end; 

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)

	
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



