-----------------------------------
-- Area: Al'Taieu
-- NPC:  Absolute Virtue
-----------------------------------

require("scripts/globals/titles");

-----------------------------------
-- onMobSpawn Action
-----------------------------------

function OnMobSpawn(mob)
end;

-----------------------------------
-- onMobInitialize Action
-----------------------------------

function onMobInitialize(mob)
end;

function onMobFight(mob, target)
    local battletime = mob:getBattleTime();
    if (mob:getExtraVar(1) == 0) then
        mob:setExtraVar(math.random(30,90));
    end
    if (battletime >= mob:getExtraVar(1)) then
        mob:useMobAbility(1030); 
        mob:setExtraVar(battletime + math.random(60,120));
    end
end;
function onMobFight(mob, target)
	if ((getMobHP() > (getMobMaxHP()*.8))
	mob:useMobAbility(1030)
	end
end;
-----------------------------------
-- onMobFight Action
-----------------------------------

function onMobFight(mob,target)

if (mob:hasStatusEffect(EFFECT_BLOOD_WEAPON) == false and mob:actionQueueEmpty() == true) then
    mob:useMobAbility(439);
end;
-----------------------------------
-- onMobDeath
-----------------------------------

function onMobDeath(mob, killer)
	killer:addTitle(VIRTUOUS_SAINT);
end;