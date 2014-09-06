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

stock = {0x3ADE,125000000,		-- Aegis
	 0x4764,125000000,		-- Excal
	 0x475e,125000000,		-- Mandau
	 0x4758,125000000,		-- Sphari
	 0x476a,125000000,		-- Ragnarok
	 0x4770,125000000,		-- Gutter
	 0x4776,125000000,		-- Bravura
	 0x477c,125000000,		-- Gungnir
	 0x4782,125000000,		-- Apocalypse
	 0x4788,125000000,		-- Kikoku
	 0x478e,125000000,		-- Amano
	 0x4794,125000000,		-- Mjollnir
	 0x479a,125000000,		-- Claustrum
	 0x47a0,125000000,		-- Annihilator
	 0x47a6,125000000,		-- Gjallarhorn
	 0x47ac,125000000}		-- Yochi
 
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



