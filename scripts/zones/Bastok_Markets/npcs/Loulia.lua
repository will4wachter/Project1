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

	if(player:hasItem(11811,10293,15793) == false) then
		player:addItem(11811);
		player:addItem(10293);
		player:addItem(15793);
		player:messageSpecial(ITEM_OBTAINED,11811);
		player:messageSpecial(ITEM_OBTAINED,10293);
		player:messageSpecial(ITEM_OBTAINED,15793);
	end
	if(player:hasKeyItem(8,9,138) == false) then
		player:addKeyItem(8);
		player:addKeyItem(9);
		player:addKeyItem(138);
		player:messageSpecial(KEYITEM_OBTAINED,138);
		player:messageSpecial(KEYITEM_OBTAINED,8);
		player:messageSpecial(KEYITEM_OBTAINED,9);
	end
	player:addFame(BASTOK,  BAS_FAME*30);
	player:addFame(SANDORIA,SAN_FAME*30);
	player:addFame(WINDURST,WIN_FAME*30);
	player:addFame(NORG,    NORG_FAME*30);
	player:addFame(JEUNO,	JEUNO_FAME*30);
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

