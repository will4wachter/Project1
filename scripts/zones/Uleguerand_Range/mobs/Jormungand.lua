-----------------------------------
-- Area: Uleguaerand Range
-- NPC:  Jormungand
-----------------------------------

require("scripts/globals/status");
require("scripts/globals/titles");

-----------------------------------
-- onMobInitialize Action
-----------------------------------

function onMobInitialize(mob)
    mob:addMod(MOD_REGEN, 30);
end;

-----------------------------------
-- onMobSpawn Action
-----------------------------------

function onMobSpawn(mob)
end;

-----------------------------------
-- onMobFight Action
-----------------------------------

function onMobFight(mob,target)
	
    if (mob:hasStatusEffect(EFFECT_BLOOD_WEAPON) == false and mob:actionQueueEmpty() == true) then
        local changeTime, twohourTime = mob:getExtraVar(2);

        if (twohourTime == 0) then
            twohourTime = math.random(8, 14);
            mob:setExtraVar(changeTime, twohourTime);
        end
        
        if (mob:AnimationSub() == 2 and mob:getBattleTime()/15 > twohourTime) then
            mob:useMobAbility(439);
            mob:setExtraVar(changeTime, (mob:getBattleTime()/15)+20);
        elseif(mob:AnimationSub() == 0 and mob:getBattleTime() - changeTime > 60) then
            mob:AnimationSub(1);
            mob:addStatusEffectEx(EFFECT_ALL_MISS, 0, 1, 0, 0);
            mob:SetMobSkillAttack(true);
            --and record the time this phase was started
            mob:setExtraVar(mob:getBattleTime(), twohourTime);
        -- subanimation 1 is flight, so check if he should land
        elseif(mob:AnimationSub() == 1 and 
                mob:getBattleTime() - changeTime > 30) then
            mob:useMobAbility(1036);
            mob:setExtraVar(mob:getBattleTime(), twohourTime);
        -- subanimation 2 is grounded mode, so check if he should take off
        elseif(mob:AnimationSub() == 2 and 
                mob:getBattleTime() - changeTime > 60) then
            mob:AnimationSub(1);
            mob:addStatusEffectEx(EFFECT_ALL_MISS, 0, 1, 0, 0);
            mob:SetMobSkillAttack(true);
            mob:setExtraVar(mob:getBattleTime(), twohourTime);
        end
	end
end;

-----------------------------------
-- onMobDeath
-----------------------------------

function onMobDeath(mob, killer)
	killer:addTitle(WORLD_SERPENT_SLAYER);
	killer:addGil(math.random(75000000,100000000));
    UpdateNMSpawnPoint(mob:getID());
    mob:setRespawnTime(math.random((180),(360)));	-- 3 to 5 days
end;