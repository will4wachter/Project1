-----------------------------------
--      Area: Southern San d'Oria
--      NPC: Phamelise
--      Only sells when San d'Oria controlls Zulkheim Region
-----------------------------------
package.loaded["scripts/zones/Southern_San_dOria/TextIDs"] = nil;
-----------------------------------

require("scripts/globals/harvest_festivals");
require("scripts/globals/settings");
require("scripts/globals/shop");
require("scripts/globals/quests");
require("scripts/globals/conquest");
require("scripts/zones/Southern_San_dOria/TextIDs");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
	-- "Flyers for Regine" conditional script
	if (player:getQuestStatus(SANDORIA,FLYERS_FOR_REGINE) == 1) then
		if (trade:hasItemQty(532,1) == true and trade:getItemCount() == 1) then
				player:messageSpecial(FLYER_REFUSED);
		end
	else
		onHalloweenTrade(player,trade,npc);
	end
end;

-----------------------------------
-- onTrigger Action
-----------------------------------


function onTrigger(player,npc)
	
player:showText(npc,PHAMELISE_OPEN_DIALOG);

		stock = {0x466,20202, -- Beastmen Seal
			 0x467,50505,       --Kindreds seal
			 0xD0B,7500000,		--Fafnir pop
			 0xD0C,15000000,       --Nidhogg pop
			 0xD0D,1000000,		--Behemoth pop
			 0xD0E,15000000,	--KB Pop
			 0xD0F,1000000,		--Adaman pop
			 0xD10,15000000}	--Aspid pop

showShop(player, STATIC ,stock);

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


