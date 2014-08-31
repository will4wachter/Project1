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

	if(player:hasItem(11811) == false) then
		player:addItem(11811);
		player:messageSpecial(ITEM_OBTAINED,11811);
	end
	if(player:hasItem(10293) == false) then
		player:addItem(10293);
		player:messageSpecial(ITEM_OBTAINED,10293);
	end
	if(player:hasItem(15793) == false) then
		player:addItem(15793);
		player:messageSpecial(ITEM_OBTAINED,15793);
	end
if(player:hasKeyItem(8,9,138,352,353,354,355,356,357,963,964,965) == false) then
		player:addKeyItem(8);
		player:addKeyItem(9);
		player:addKeyItem(138);
		player:addKeyItem(352);
		player:addKeyItem(353);
		player:addKeyItem(354);
		player:addKeyItem(355);
		player:addKeyItem(356);
		player:addKeyItem(357);
		player:addKeyItem(963);
		player:addKeyItem(964);
		player:addKeyItem(965);
		player:messageSpecial(KEYITEM_OBTAINED,138);
		player:messageSpecial(KEYITEM_OBTAINED,8);
		player:messageSpecial(KEYITEM_OBTAINED,9);
		player:messageSpecial(KEYITEM_OBTAINED,352);
		player:messageSpecial(KEYITEM_OBTAINED,353);	
		player:messageSpecial(KEYITEM_OBTAINED,354);
		player:messageSpecial(KEYITEM_OBTAINED,355);
		player:messageSpecial(KEYITEM_OBTAINED,356);
		player:messageSpecial(KEYITEM_OBTAINED,357);
		player:messageSpecial(KEYITEM_OBTAINED,963);
		player:messageSpecial(KEYITEM_OBTAINED,964);
		player:messageSpecial(KEYITEM_OBTAINED,965);
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



