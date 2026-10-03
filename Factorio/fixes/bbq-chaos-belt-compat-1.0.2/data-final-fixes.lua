-- BBQ CHAOS diagnostic/compatibility mod v1.0.2; Factorio 2.0.77.
-- It repairs ONLY turbo-transport-belt's obsolete Bob name.
-- Optional dependencies order this stage AFTER reskins and squeak-through-2.
local legacy = "bob-turbo-transport-belt"
local validOutput = "turbo-transport-belt"
local validInput = "express-transport-belt"
local recipe = data.raw.recipe[validOutput]
local id = "[BBQ CHAOS BELT FIX v1.0.2] "

if not recipe then
  error(id .. "The turbo-transport-belt recipe is missing")
end
local function item_exists(name)
  for _, category in pairs({
      "item","item-with-entity-data","tool","ammo","capsule",
      "module","repair-tool","gun","armor"
  }) do
    if data.raw[category] and data.raw[category][name] then return true end
  end
  return false
end

local function all_paths(value, path, visited, acc)
  if type(value) ~= "table" then
    if value == legacy then
      acc[#acc+1] = path
    end
    return
  end
  if visited[value] then return end
  visited[value] = true
  for k, v in pairs(value) do
    all_paths(v, path .. "." .. tostring(k), visited, acc)
  end
end
local before = {}
all_paths(recipe, "recipe", {}, before)
log(id .. "BEFORE legacy_references=" .. #before)
for _, path in ipairs(before) do log(id .. "BEFORE_PATH " .. path) end

if item_exists(legacy) then
  log(id .. "SKIPPED: legacy Bob item already exists")
  return
end
if not item_exists(validOutput) or not item_exists(validInput) then
  error(id .. "Required turbo/express transport-belt item missing")
end

local changesIn, changesOut, changesMain = 0,0,0
local function fix_nested(container, replacement, isOutput, visited)
  if type(container) ~= "table" then return end
  if visited[container] then return end
  visited[container] = true
  for key, value in pairs(container) do
    if type(value) == "table" then
      fix_nested(value,replacement,isOutput,visited)
    elseif value == legacy and (key == "name" or key == 1 or key == "result") then
      container[key] = replacement
      if isOutput then changesOut=changesOut+1 else changesIn=changesIn+1 end
    end
  end
end

local function fix_variant(v)
  if type(v) ~= "table" then return end
  fix_nested(v.ingredients,validInput,false,{})
  fix_nested(v.results,validOutput,true,{})
  if v.result == legacy then
    v.result = validOutput
    changesOut = changesOut + 1
  end
  if v.main_product == legacy then
    v.main_product = validOutput
    changesMain=changesMain+1
  end
end
fix_variant(recipe)
fix_variant(recipe.normal)
fix_variant(recipe.expensive)

local after={}
all_paths(recipe,"recipe",{},after)
log(id .. "CHANGED ingredients="..changesIn.." results="..changesOut.." main_products="..changesMain)
log(id .. "AFTER legacy_references=" .. #after)
for _, path in ipairs(after) do log(id .. "UNRESOLVED_PATH " .. path) end
