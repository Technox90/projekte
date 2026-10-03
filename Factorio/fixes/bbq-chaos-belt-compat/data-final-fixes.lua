-- Targeted late-load belt compatibility for Factorio 2.0.77.
-- No intervention when a Bob prefixed belt exists.
local missing = "bob-turbo-transport-belt"
local exists = data.raw.item[missing]
  or (data.raw["item-with-entity-data"] and data.raw["item-with-entity-data"][missing])
if exists then
  log("[BBQ CHAOS] bob-turbo-transport-belt exists, no compatibility fix necessary")
  return
end

local target = data.raw.recipe["turbo-transport-belt"]
local replacement = data.raw.item["express-transport-belt"]
if not target or not replacement then
  log("[BBQ CHAOS] turbo-transport-belt or express-transport-belt missing, fix skipped")
  return
end

local changes = 0
local function fix_ingredients(ingredients)
  if type(ingredients) ~= "table" then return end
  for _, ingredient in pairs(ingredients) do
    if type(ingredient) == "table" then
      if ingredient.name == missing then
        ingredient.name = "express-transport-belt"
        changes = changes + 1
      elseif ingredient[1] == missing then
        ingredient[1] = "express-transport-belt"
        changes = changes + 1
      end
    end
  end
end

fix_ingredients(target.ingredients)
if target.normal then fix_ingredients(target.normal.ingredients) end
if target.expensive then fix_ingredients(target.expensive.ingredients) end
log("[BBQ CHAOS] Bob turbo belt recipe ingredients repaired: " .. changes)
