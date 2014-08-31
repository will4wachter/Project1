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

stock = {0x3ADE,250000000,		-- Aegis
	 0x4764,250000000,		-- Excal
	 0x475e,250000000,		-- Mandau
	 0x4758,250000000,		-- Sphari
	 0x476a,250000000,		-- Ragnarok
	 0x4770,250000000,		-- Gutter
	 0x4776,250000000,		-- Bravura
	 0x477c,250000000,		-- Gungnir
	 0x4782,250000000,		-- Apocalypse
	 0x4788,250000000,		-- Kikoku
	 0x478e,250000000,		-- Amano
	 0x4794,250000000,		-- Mjollnir
	 0x479a,250000000,		-- Claustrum
	 0x47a0,250000000,		-- Annihilator
	 0x47a6,250000000,		-- Gjallarhorn
	 0x47ac,250000000}		-- Yochi
 
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



