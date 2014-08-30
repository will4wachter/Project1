-----------------------------------
-- Area: Aht Urhgan Whitegate
-- NPC: Nijaom
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


stock = {0x3EE4,20000000,		-- Morrigan's Coronal
	 0x38E2,80000000,		-- Morrigan's Robe
	 0x3a81,40000000,		-- Morrigan's Cuffs
	 0x3d19,50000000,		-- Morrigan's Slops
	 0x3d6f,30000000,		-- Morrigan's Pigaches
	 0x3ee0,20000000,		-- Marduk's Tiara
	 0x38de,80000000,		-- Marduk's Jubbah
	 0x3a7d,40000000,		-- Marduk's Dastanas
	 0x3d15,50000000,		-- Marduk's shalwar
	 0x3d6b,30000000,		-- Marduk's Crackows
	 0x3ed4,20000000,		-- Ares's Mask
	 0x38d2,80000000,		-- Ares's Cuirass
	 0x3a71,40000000,		-- Ares's Gauntlets
	 0x3d09,50000000,		-- Ares's Flanchard
	 0x3d5f,30000000,		-- Ares's Sollerets
	 0x3edc,20000000,		-- Usukane Somen
	 0x38da,80000000,		-- Usukane haramaki
	 0x3a79,40000000,		-- Usukane gote
	 0x3d11,50000000,		-- Usukane hizayoroi
	 0x3d67,30000000,		-- Usukane sune-ate
	 0x3ed8,20000000,		-- Skadi's Visor
	 0x38d6,80000000,		-- Skadi's Cuirie
	 0x3a75,40000000,		-- Skadi's Bazubands
	 0x3d0d,50000000,		-- Skadi's Chausses
	 0x3d63,30000000}		-- Skadi's Jambeaux



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



