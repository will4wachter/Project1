-----------------------------------
-- Area:  Castle Oztroja
-- NPC:   _477 (Brass Door)
-- Notes: Open by 4 torch stands
-- @pos -100 -71 -49 151
-----------------------------------
package.loaded["scripts/zones/Castle_Oztroja/TextIDs"] = nil;
-----------------------------------

require("scripts/zones/Castle_Oztroja/TextIDs");

-----------------------------------
-- onTrigger Action
-----------------------------------

function onTrigger(player,npc)
	
	local DoorID = npc:getID() - 1;
	local DoorA = GetNPCByID(DoorID):getAnimation();

	if(DoorA == 9 and npc:getAnimation() == 9 then
		npc:openDoor(6.5);
		GetNPCByID(DoorID):openDoor(4.5);
	end
end;

-----------------------------------
-- onEventUpdate
-----------------------------------

function onEventUpdate(player,csid,option)
--printf("upCSID: %u",csid);
--printf("upRESULT: %u",option);
end;

-----------------------------------
-- onEventFinish Action
-----------------------------------

function onEventFinish(player,csid,option)
--printf("CSID: %u",csid);
--printf("RESULT: %u",option);
end;