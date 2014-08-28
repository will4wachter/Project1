-----------------------------------
--	Area: Windurst Woods
--	NPC:  Seven of Spades
--	Working 100%
-----------------------------------

require("scripts/globals/settings");

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
	player:addItem(15793);
	player:messageSpecial(ITEM_OBTAINED,11811);
	player:messageSpecial(ITEM_OBTAINED,10293);
	player:messageSpecial(ITEM_OBTAINED,15793);
	player:addFame(BASTOK,  BAS_FAME*30);
	player:addFame(SANDORIA,SAN_FAME*30);
	player:addFame(WINDURST,WIN_FAME*30);
	player:addFame(NORG,    NORG_FAME*30);
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



