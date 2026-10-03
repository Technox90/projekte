-- BBQ CHAOS test compatibility fix for Factorio 2.0.77.
-- Bob's Logistics >= 2.1 renamed bob-turbo-transport-belt to turbo-transport-belt.
-- If a legacy ingredient/product remains, replace only the invalid reference
-- in the single recipe named turbo-transport-belt.
local missing = "bob-turbo-transport-belt"
local recipeName = "turbo-transport-belt"
local currentItem = "turbo-transport-belt"
local precursorItem = "express-transport-belt"

local function has_item(name)
  for _, itemType in pairs({
    "item", "item-with-entity-data", "tool", "ammo", "capsule",
    "module", "repair-tool", "gun", "armor"
  }) do
    if data.raw[itemType] and data.raw[itemType][name] then
      return true
    end
  end
  return false
end

local recipe = data.raw.recipe[recipeName]
if not recipe then
  log("[BBQ CHAOS BELT FIX v1.0.1] SKIPPED: turbo-transport-belt recipe missing")
  return
end
if has_item(missing) then
  log("[BBQ CHAOS BELT FIX v1.0.1] SKIPPED: bob-turbo-transport-belt still exists")
  return
end
if not has_item(currentItem) then
  error("[BBQ CHAOS BELT FIX v1.0.1] turbo-transport-belt item is missing; cannot repair recipe")
end
if not has_item(precursorItem) then
  error("[BBQ CHAOS BELT FIX v1.0.1] express-transport-belt item is missing; cannot repair ingredient")
end

local changedIn = 0
local changedOut = 0
local changedMain = 0

local function replace(array, to, isOutput)
  if type(array) ~= "table" then return end
  for _, ref in pairs(array) do
    if type(ref) == "table" then
      if ref.name == missing then
        ref.name = to
        if isOutput then changedOut = changedOut + 1 else changedIn = changedIn + 1 end
      elseif ref[1] == missing then
        ref[1] = to
        if isOutput then changedOut = changedOut + 1 else changedIn = changedIn + 1 end
      end
    end
  end
end

local function repair_variant(variant)
  if type(variant) ~= "table" then return end
  replace(variant.ingredients, precursorItem, false)
  replace(variant.results, currentItem, true)
  if variant.result == missing then
    variant.result = currentItem
    changedOut = changedOut + 1
  end
  if variant.main_product == missing then
    variant.main_product = currentItem
    changedMain = changedMain + 1
  end
end

repair_variant(recipe)
repair_variant(recipe.normal)
repair_variant(recipe.expensive)

log("[BBQ CHAOS BELT FIX v1.0.1] recipe=" .. recipeName
  .. " ingredients=" .. changedIn
  .. " results=" .. changedOut
  .. " main_products=" .. changedMain)

-- Do not guess at further fixes: the final validity check remains Factorio's.
