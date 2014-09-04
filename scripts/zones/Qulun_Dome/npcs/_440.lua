-----------------------------------
-- Area: Qulun Dome
-- NPC:  Door
-- Involved in Mission: Magicite
-- @pos 60 24 -2 148
-----------------------------------
package.loaded["scripts/zones/Qulun_Dome/TextIDs"] = nil;
-----------------------------------

require("scripts/globals/keyitems");
require("scripts/zones/Qulun_Dome/TextIDs");

-----------------------------------
-- onTrade Action
-----------------------------------

function onTrade(player,npc,trade)
end;

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)
	
	if(player:hasKeyItem(SILVER_BELL) and player:hasKeyItem(CORUSCANT_ROSARY) and player:hasKeyItem(BLACK_MATINEE_NECKLACE)) then
		if(player:getZPos() < -7.2) then
			player:startEvent(0x0033);
		else
			player:startEvent(0x0032);
		end
	else
		player:addKeyItem(46);
		player:messageSpecial(KEYITEM_OBTAINED,46);
	end
	
	return 1;
	
end;

-----------------------------------
-- onEventUpdate
-----------------------------------

function onEventUpdate(player,csid,option)
-- printf("CSID: %u",csid);
-- printf("RESULT: %u",option);
end;

-----------------------------------
-- onEventFinish
-----------------------------------

function onEventFinish(player,csid,option)
-- printf("CSID: %u",csid);
-- printf("RESULT: %u",option);	
	
	if((csid == 0x0032 or csid == 0x0033) and option == 1) then
		player:messageSpecial(THE_3_ITEMS_GLOW_FAINTLY,SILVER_BELL,CORUSCANT_ROSARY,BLACK_MATINEE_NECKLACE);
	end
	
end;