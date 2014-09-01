-----------------------------------------
-- ID: 15881
-- Item: Talisman obi
-- MP +12, Enmity -2
-----------------------------------------

require("scripts/globals/status");

-----------------------------------------
-- OnItemCheck
-----------------------------------------
function onItemCheck(target)
	return 0;
end;
-----------------------------------------
-- OnItemUse
-----------------------------------------

function onItemUse(target)
	target:addMod(MOD_MP, 12,1200);
	target:addMod(MOD_ENMITY, -2,1200);
end;


