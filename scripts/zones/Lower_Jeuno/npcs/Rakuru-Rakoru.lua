-----------------------------------
-- Area: Lower Jeuno
-- NPC: Rakuru-Rakoru
-- Standard Merchant NPC
-----------------------------------

require("scripts/globals/shop");
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
	
player:showText(npc,PHAMELISE_OPEN_DIALOG);

stock = {0x3ADE,100000000,		-- Aegis
	 0x4764,100000000,		-- Excal
	 0x475e,100000000,		-- Mandau
	 0x4758,100000000,		-- Sphari
	 0x476a,100000000,		-- Ragnarok
	 0x4770,100000000,		-- Gutter
	 0x4776,100000000,		-- Bravura
	 0x477c,100000000,		-- Gungnir
	 0x4782,100000000,		-- Apocalypse
	 0x4788,100000000,		-- Kikoku
	 0x478e,100000000,		-- Amano
	 0x4794,100000000,		-- Mjollnir
	 0x479a,100000000,		-- Claustrum
	 0x47a0,100000000,		-- Annihilator
	 0x47a6,100000000,		-- Gjallarhorn
	 0x47ac,100000000}		-- Yochi
 
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



