-----------------------------------
--      Area: Northern San d'Oria
--      NPC: Attarena
--      Only sells when San d'Oria controlls Li'Telor Region
-----------------------------------
package.loaded["scripts/zones/Northern_San_dOria/TextIDs"] = nil;
-----------------------------------

require("scripts/globals/harvest_festivals");
require("scripts/globals/settings");
require("scripts/globals/shop");
require("scripts/globals/quests");
require("scripts/globals/conquest");
require("scripts/zones/Northern_San_dOria/TextIDs");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
	-- "Flyers for Regine" conditional script
	if (player:getQuestStatus(SANDORIA,FLYERS_FOR_REGINE) == 1) then
		if (trade:hasItemQty(532,1) == true and trade:getItemCount() == 1) then
			player:messageSpecial(FLYER_REFUSED);
		end
	else
		onHalloweenTrade(player,trade,npc);
	end
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

