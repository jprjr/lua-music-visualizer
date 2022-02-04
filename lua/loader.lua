local args = {...}
local modules = args[1] -- table passed from C with all our local modules

local function load_string(str, name)
  local sent = false
  return load(function()
    if not sent then
      sent = true
      return str
    end
    return nil
  end,name)
end

local function lua_loader(name)
  local separator = package.config:sub(1,1)
  name = name:gsub(separator, '.')
  local mod = modules[name]
  if mod then
    if type(mod) == 'string' then
      local chunk, errstr = load_string(mod, name)
      if chunk then
        return chunk
      else
        error('problem loading module: ' .. errstr)
      end
    elseif type(mod) == 'function' then
      return mod
    end
  end
end

table.insert(package.loaders or package.searchers, 2, lua_loader)
