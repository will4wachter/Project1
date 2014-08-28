-----------------------------------
-- Area: Selbina
-- NPC: Chutarmire
-- Standard Merchant NPC
-----------------------------------
package.loaded["scripts/zones/Selbina/TextIDs"] = nil;
-----------------------------------

require("scripts/zones/Selbina/TextIDs");
require("scripts/globals/shop");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
end; 

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)
	
player:showText(npc,CHUTARMIRE_SHOP_DIALOG);

stock = {0x129f,1249,	-- Stone
	 0x12A0,5751,	-- Scroll of Stone II
	 0x12a1,25321,	-- Stone III
	 0x12a2,98321,	-- Stone IV
	 0x12a9,1890,	-- Water
	 0x12AA,8100,	-- Scroll of Water II
	 0x12ab,39242,	-- Water III
	 0x12ac,113490,	-- Water IV
	 0x129b,9124,	-- Aero
	 0x129B,11970,	-- Scroll of Aero II
	 0x129c,42124,	-- Aero III
	 0x129d,153210,	-- Aero IV
	 0x1290,8732,	-- Fire
	 0x1291,16560,	-- Scroll of Fire II
	 0x1292,53215,	-- Fire III
	 0x1293,142350, -- Fire IV
	 0x1295,8234,	-- Blizzard	
	 0x1296,21870,	-- Scroll of Blizzard II
	 0x1297,47123,	-- Blizzard III
	 0x1298,172312, -- Blizzard IV
	 0x12a4,9912,	-- Thunder
	 0x12A5,27900,	-- Scroll of Thunder II
	 0x12a6,76234,	-- Thunder III
	 0x12a7,193210,	-- Thunder IV
	 0x12BD,1165,	-- Scroll of Stonega
	 0x12C7,2097,	-- Scroll of Waterga
	 0x12B8,4147,	-- Scroll of Aeroga
	 0x12AE,7025,	-- Scroll of Firaga
	 0x12B3,10710,	-- Scroll of Blizzaga
	 0x12C2,15120,	-- Scroll of Thundaga
	 0x12dc,9530,	-- Poison
	 0x12DD,22680,	-- Scroll of Poison II
	 0x12e6,4523,   -- Bio
	 0x12E7,12600,	-- Scroll of Bio II
	 0x12E1,4644}	-- Scroll of Poisonga
 
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



