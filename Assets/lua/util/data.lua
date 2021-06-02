local data = {}
data.raw = {}

function data.extend(self, otherdata)
  if type(otherdata) ~= "table" or #otherdata == 0 then
    error("Invalid prototype array " .. serpent.block(otherdata, {maxlevel= 1}))
  end

  for _, e in ipairs(otherdata) do
    if not e.type then
      error("Missing type in the following prototype definition " .. serpent.block(e))
    end

    if not e.name then
      error("Missing name in the following prototype definition " .. serpent.block(e))
    end

    local t = self.raw[e.type]
    if t == nil then
      t = {}
      self.raw[e.type] = t
    end
    t[e.name] = e
  end
end

return data