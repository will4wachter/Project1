-----------------------------------
-- Area: Qulun Dome
-- NPC:  Door
-- Involved in Mission: Limit Break 3
-- @pos 299.999 37.864 47.067 148
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
	
	if(npc:getAnimation() == 9) then
		if(player:getZPos() < 46) then
			if(player:hasKeyItem(45) and player:hasKeyItem(46) and player:hasKeyItem(BLACK_MATINEE_NECKLACE)) then
				player:messageSpecial(THE_3_ITEMS_GLOW_FAINTLY,SILVER_BELL,CORUSCANT_ROSARY,BLACK_MATINEE_NECKLACE);
				npc:openDoor(20); -- retail timed
			else
				player:addKeyItem(46);
				player:addKeyItem(45);
				player:addKeyItem(47);
				player:messageSpecial(KEYITEM_OBTAINED,46);
				player:messageSpecial(KEYITEM_OBTAINED,45);
				player:messageSpecial(KEYITEM_OBTAINED,47);
			end
		else
			player:messageSpecial(CANNOT_BE_OPENED_FROM_THIS_SIDE);
		end
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
end;