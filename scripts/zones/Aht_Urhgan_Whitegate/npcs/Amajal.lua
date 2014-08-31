-----------------------------------
-- Area: Aht Urhgan Whitegate
-- NPC: Amajal
-- Standard Info NPC
-----------------------------------
package.loaded["scripts/zones/Aht_Urhgan_Whitegate/TextIDs"] = nil;
-----------------------------------


require("scripts/zones/Aht_Urhgan_Whitegate/TextIDs");

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
stock = {0x4a35,250000000,		-- Burtgang
	 0x4a34,250000000,		-- Vajra
	 0x4a33,250000000,		-- Murgleis
	 0x4a32,250000000,		-- Laevateinn
	 0x4a31,250000000,		-- Yagrush
	 0x4a30,250000000,		-- Glanzfaust
	 0x4a2f,250000000,		-- Conqueror
	 0x4a2e,250000000,		-- Tupsimati
	 0x4a2d,250000000,		-- Terpsichore
	 0x4a36,250000000,		-- Liberator
	 0x4a37,250000000,		-- Aymur
	 0x4a38,250000000,		-- Carnwenhan
	 0x4a39,250000000,		-- Gastraphetes
	 0x4a3a,250000000,		-- Kogarasumaru
	 0x4a3b,250000000,		-- Nagi
	 0x4a3c,250000000,		-- Ryunohige
	 0x4a3d,250000000,		-- Nirvana
	 0x4a3e,250000000,		-- Tizona
	 0x4a3f,250000000,		-- Death Penalty
	 0x4a40,250000000}		-- Kenkonken
 
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



