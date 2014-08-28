--------------------------------------------------------------
-- func: @shop
-- auth: forgottenandlost
-- desc: opens a custom shop anywhere in the world
--------------------------------------------------------------

cmdprops =
{
    permission = 0,
    parameters = "i"
};

function onTrigger(player,page)
    if (page == 0 or page == nil) then
        player:PrintToPlayer( "1: Stuck Somewhere?, 2: MogDonaldssss");
    elseif (page == 1) then
        local stock_1 =
        {
            4181,   5000,       -- HQ Fire Crystal
        };
        showShop(player, STATIC, stock_1);

    elseif ( page == 2) then -- Would you like fries with that?
        local stock_2 =
        {
            4271,   2499,       -- Rice Dumpling
            4381,   2100,       -- Meat Mithkabob
            4398,   2100,       -- Fish Mithkabob
            4457,   2100,       -- Eel Kabob
            5166,   2100,       -- Coeurl Sub Sandwich
            4355,   1450,       -- Salmon Sub Sandwich
            5686,   799,        -- Cheese Sandwich
            5561,   799,        -- Moogle Pie
            4270,   999,        -- Sweet Rice Cake
            5944,   2000,       -- Frontier Soda
            5932,   1600,       -- Kitron Juice
            4424,   1100,       -- Melon Juice
            4422,   499         -- Orange Juice
        };
        showShop(player, STATIC, stock_2);

    else
        player:PrintToPlayer( string.format( "Page %i not found.", page ) );
    end
end;