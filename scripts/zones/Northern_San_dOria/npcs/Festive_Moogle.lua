-----------------------------------
-- Area: Northern San d'Oria
-- NPC:  Festive Moogle
-- @pos 115 0 -11 231
-----------------------------------
package.loaded["scripts/zones/Northern_San_dOria/TextIDs"] = nil;
-----------------------------------

require("scripts/globals/settings");
require("scripts/globals/teleports");
require("scripts/globals/quests");
require("scripts/zones/Northern_San_dOria/TextIDs");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
end; 

----------------------------------- 
-- onTrigger Action 
-----------------------------------
 
function onTrigger(player,npc) 

	player:addItem(11811);
	player:addItem(10293);
	player:addItem(10796);
	player:addItem(4182);
	player:addItem(4198);
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