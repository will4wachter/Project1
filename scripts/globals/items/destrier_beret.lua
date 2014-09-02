-----------------------------------------
--	ID: 11811
--	Destrier Beret
--	Enchantment: 60Min, Chocobo Egg-Chocobo
-----------------------------------------

require("scripts/globals/status");

-----------------------------------------
-- OnItemCheck
-----------------------------------------

function onItemCheck(target)
	return target:canUseCostume();
end;

-----------------------------------------
-- OnItemUse
-----------------------------------------

function onItemUse(target)
	target:addStatusEffect(EFFECT_COSTUME,1996,0,3600);

end;