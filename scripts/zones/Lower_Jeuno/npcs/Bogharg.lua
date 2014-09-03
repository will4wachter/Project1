-----------------------------------
-- Area: Lower Jeuno
-- NPC: Bogharg
-- Standard Info NPC
-----------------------------------

package.loaded["scripts/zones/Lower_Jeuno/TextIDs"] = nil;
require("scripts/zones/Lower_Jeuno/TextIDs");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
end; 

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)

stock = {0x14bc,2500,			-- Uchitake
	 0x14bd,2500,		-- Excal
	 0x14be,2500,		-- Mandau
	 0x14bf,2500,
	 0x14c0,2500,
	 0x14c1,2500,
	 0x14c2,2500,
	 0x14c3,2500,
	 0x14c4,2500,
	 0x14c5,2500,
	 0x14c6,2500,
	 0x14c7,2500}		-- Yochi
 
showShop(player, STATIC, stock);
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



