-----------------------------------
-- Area: Selbina
-- NPC: Quelpia
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

player:showText(npc,QUELPIA_SHOP_DIALOG);

stock = {0x1200,2510,		-- Scroll of Cure
	 0x1202,5850,		-- Scroll of Cure II
	 0x1203,32610,		-- Scroll of Cure III
	 0x1204,123210,		-- Scroll of Cure IV
	 0x1205,503340,		-- Scroll of Cure V
	 0x1207,35023,		-- Scroll of Curaga
	 0x1208,100800,		-- Scroll of Curaga II
	 0x1209,287092,		-- Scroll of Curaga III
	 0x120C,51780,		-- Scroll of Raise
	 0x120d,175930,		-- Scroll of Raise II
	 0x1215,315000,		-- Scroll of Holy
	 0x1218,100800,		-- Scroll of Dia II
	 0x121D,81000,		-- Scroll of Banish II
	 0x122b,4302,		-- Scroll of Protect
	 0x122C,13660,		-- Scroll of Protect II
	 0x122d,35010,		-- Scroll of Protect III
	 0x122e,89120,		-- Scroll of Protect IV
	 0x1230,3540,		-- Scroll of Shell
	 0x1231,18400,		-- Scroll of Shell II
	 0x1232,56290,		-- Scroll of Shell III	
	 0x1233,178320,		-- Scroll of Shell IV
	 0x126c,3420,		-- Scroll of Regen
	 0x126e,48320,		-- Scroll of Regen II
	 0x126f,186323,		-- Scroll of Regen III
	 0x126d,1020321,	-- Scroll of Refresh
	 0x1239,180000,		-- Scroll of Haste
	 0x126a,192830,		-- Scroll of Phalanx
	 0x1236,57830,		-- Scroll of Stoneskin
	 0x1235,48230}		-- Scroll of Blink

	

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



