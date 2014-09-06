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
stock = {0x4a35,125000000,		-- Burtgang
	 0x4a34,125000000,		-- Vajra
	 0x4a33,125000000,		-- Murgleis
	 0x4a32,125000000,		-- Laevateinn
	 0x4a31,125000000,		-- Yagrush
	 0x4a30,125000000,		-- Glanzfaust
	 0x4a2f,125000000,		-- Conqueror
	 0x4a2e,125000000,		-- Tupsimati
	 0x4a2d,125000000,		-- Terpsichore
	 0x4a36,125000000,		-- Liberator
	 0x4a37,125000000,		-- Aymur
	 0x4a38,125000000,		-- Carnwenhan
	 0x4a39,125000000,		-- Gastraphetes
	 0x4a3a,125000000,		-- Kogarasumaru
	 0x4a3b,125000000,		-- Nagi
	 0x4a3c,125000000,		-- Ryunohige
	 0x4a3d,125000000,		-- Nirvana
	 0x4a3e,125000000,		-- Tizona
	 0x4a3f,125000000,		-- Death Penalty
	 0x4a40,125000000}		-- Kenkonken
 
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



