-- BBQ CHAOS v1.0.3 / Factorio 2.0.77
-- Bob's Logistics 2.1.0 removed the bob- prefix from the Turbo belt.
-- Only repair recipe item refs to the proven missing legacy ID.
-- For the turbo-transport-belt recipe itself, prevent self-dependency.
local old = "bob-turbo-transport-belt"
local current = "turbo-transport-belt"
local previousTier = "express-transport-belt"
local prefix = "[BBQ CHAOS BELT FIX v1.0.3] "

local function item_exists(name)
  for _, t in ipairs({
    "item", "item-with-entity-data", "tool", "ammo",
    "capsule", "module", "repair-tool", "gun", "armor"
  }) do
    if data.raw[t] and data.raw[t][name] then return true end
  end
  return false
end

if item_exists(old) then
  log(prefix .. "SKIP: legacy item still exists; no changes")
  return
end
if not item_exists(current) or not item_exists(previousTier) then
  error(prefix .. "Expected Space Age turbo and vanilla express belt items do not exist")
end

local updatedRecipes, inputs, outputs, mainProducts, skippedFluid = 0, 0, 0, 0, 0
local modified = {}
local function process(recipeName, variant, suffix)
  if type(variant) ~= "table" then return 0 end
  local changes = 0
  local inputItem = (recipeName == current) and previousTier or current

  local function patch(array, replacement, isOutput)
    if type(array) ~= "table" then return end
    for _, entry in pairs(array) do
      if type(entry) == "table" then
        -- Factorio prototypes can use {type="item",name="..."} or {"...",count}.
        local field
        if entry.name == old then field = "name"
        elseif entry[1] == old then field = 1 end
        if field then
          if entry.type == "fluid" then
            skippedFluid = skippedFluid + 1
          else
            entry[field] = replacement
            changes = changes + 1
            if isOutput then outputs = outputs + 1 else inputs = inputs + 1 end
          end
        end
      end
    end
  end

  patch(variant.ingredients, inputItem, false)
  patch(variant.results, current, true)
  if variant.result == old then
    variant.result = current
    changes = changes + 1
    outputs = outputs + 1
  end
  if variant.main_product == old then
    variant.main_product = current
    changes = changes + 1
    mainProducts = mainProducts + 1
  end
  if changes > 0 then
    log(prefix .. recipeName .. suffix .. " changed=" .. changes ..
      " ingredient_replacement=" .. inputItem)
  end
  return changes
end

for name, recipe in pairs(data.raw.recipe) do
  local n = 0
  n = n + process(name, recipe, "")
  n = n + process(name, recipe.normal, ".normal")
  n = n + process(name, recipe.expensive, ".expensive")
  if n > 0 then
    updatedRecipes = updatedRecipes + 1
    modified[#modified + 1] = name
  end
end
table.sort(modified)
log(prefix .. "SUMMARY recipes=" .. updatedRecipes ..
  " inputs=" .. inputs .. " outputs=" .. outputs ..
  " main_products=" .. mainProducts .. " skipped_fluid=" .. skippedFluid)
for _, name in ipairs(modified) do
  log(prefix .. "MODIFIED_RECIPE " .. name)
end
