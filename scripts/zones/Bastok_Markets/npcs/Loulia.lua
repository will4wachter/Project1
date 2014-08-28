-----------------------------------
--  Area: Bastok Markets
--   NPC: Loulia
--  Type: Room Renters
-- @zone: 235
--  @pos -176.212 -9 -25.049
--
-- Auto-Script: Requires Verification. Validated standard dialog - thrydwolf 12/8/2011
-----------------------------------


package.loaded["scripts/zones/Bastok_Markets/TextIDs"] = nil;
require("scripts/zones/Bastok_Markets/TextIDs");

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

