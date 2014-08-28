-----------------------------------
-- Area: Selbina
-- NPC: Falgima
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

player:showText(npc,FALGIMA_SHOP_DIALOG);

stock = {0x1288,15351,		-- Scroll of Invisible
	 0x1289,12325,		-- Scroll of Sneak
	 0x128A,11204,		-- Scroll of Deoderize	
	 0x127d,5830,		-- Scroll of Protectra
	 0x127e,27123,		-- Scroll of Protectra II
	 0x127f,83239,		-- Scroll of Protectra III
	 0x1280,213920,		-- Scroll of Protectra IV
	 0x1282,9112,		-- Shellra
	 0x1283,39123,		-- Shellra II
	 0x1284,94213,		-- Shellra III
	 0x1285,221332,		-- Shellra IV
	 0x1287,104320,		-- Reraise
	 0x120c,58239,		-- Raise
	 0x120d,210343,		-- Raise II
	 0x120e,17234,		-- Poisona
	 0x120f,16231,	 	-- Paralyna
	 0x1210,12932,		-- Blindna
	 0x1211,13423,		-- Silena
	 0x1212,19356,		-- Stona
	 0x1213,17422,		-- Viruna
	 0x1214,21234,		-- Cursna
	 0x1238,78323,		-- Slow
	 0x123a,58423,		-- Paralyze
	 0x123b,63231,		-- Silence
	 0x128f,134943,		-- Erase
	 0x12d8,253210,		-- Gravity
	 0x1304,219342}		-- Dispel	

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