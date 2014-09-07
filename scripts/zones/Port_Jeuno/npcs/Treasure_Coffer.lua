-----------------------------------
-- Area: Port Jeuno
-- NPC:  Treasure Coffer
-- @pos  -52 0 -11 246
-----------------------------------
package.loaded["scripts/zones/Port_Jeuno/TextIDs"] = nil;
require("scripts/zones/Port_Jeuno/TextIDs");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
	 
	
	if((trade:hasItemQty(1024,1) or 
	   trade:hasItemQty(1025,1) or 
	   trade:hasItemQty(1026,1) or 
	   trade:hasItemQty(1027,1) or 
 	   trade:hasItemQty(1028,1) or 
	   trade:hasItemQty(1029,1) or 
	   trade:hasItemQty(1030,1) or 
	   trade:hasItemQty(1031,1) or 
	   trade:hasItemQty(1032,1) or 
	   trade:hasItemQty(1033,1) or 
	   trade:hasItemQty(1034,1) or 
	   trade:hasItemQty(1035,1) or 
	   trade:hasItemQty(1036,1) or 
	   trade:hasItemQty(1037,1) or 
	   trade:hasItemQty(1038,1) or 
	   trade:hasItemQty(1039,1) or 
	   trade:hasItemQty(1040,1) or
	   trade:hasItemQty(1056,1) or
	   trade:hasItemQty(1061,1) or
	   trade:hasItemQty(1062,1) or
	   trade:hasItemQty(1064,1) or 
	   trade:hasItemQty(1055,1)) and 
	   trade:getItemCount() == 1) then
		player:tradeComplete();
		player:addGil(math.random(100000,500000));
		player:messageSpecial(GIL_OBTAINED,math.random(100000,500000));

	elseif((trade:hasItemQty(1024,1) or 
	   trade:hasItemQty(1042,1) or 
	   trade:hasItemQty(1043,1) or 
	   trade:hasItemQty(1044,1) or 
 	   trade:hasItemQty(1045,1) or 
	   trade:hasItemQty(1046,1) or 
	   trade:hasItemQty(1047,1) or 
	   trade:hasItemQty(1048,1) or 
	   trade:hasItemQty(1049,1) or 
	   trade:hasItemQty(1050,1) or 
	   trade:hasItemQty(1051,1) or 
	   trade:hasItemQty(1052,1) or 
	   trade:hasItemQty(1053,1) or 
	   trade:hasItemQty(1054,1) or 
	   trade:hasItemQty(1057,1) or 
	   trade:hasItemQty(1058,1) or 
	   trade:hasItemQty(1059,1) or
	   trade:hasItemQty(1060,1) or
	   trade:hasItemQty(1063,1)) and 
	   trade:getItemCount() == 1) then
		player:tradeComplete();
		player:addGil(math.random(1000000,2000000));
		player:messageSpecial(GIL_OBTAINED,math.random(1000000,2000000));
	end

end;

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)

	player:messageSpecial(CHEST_IS_EMPTY);
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