--[[
Matias Guijarro wrote:

> I would like to change the default behaviour of Lua when
> representing a table through the tostring() function ; I
> would prefer to have something more like in Python for
> example (instead of having "table: 0x8062ac0", I would
> like to see the contents, for example { 1, 2, 3 }).
>
> So first of all I tried to set a new metatable for tables,

As Shmuel pointed out, table metatables are per-table, so
they don't really work for what you want to do.

  Lua 5.1.1  Copyright (C) 1994-2006 Lua.org, PUC-Rio
  > Tbl = setmetatable({}, {__tostring = function(t)
  >>                                        return "Hello!"
  >>                                      end})
  > print(Tbl) -- Works for this table,
  Hello!
  > print({}) -- but doesn't work for other tables.
  table: 0x493378

A way that will work is to simply replace the tostring
function itself.  Replace it with a function that calls your
table-to-string function for tables, and calls the original
tostring for everything else.

Roberto's Programming in Lua book explains how to write the
table-to-string function.  (The first edition is available
online: <http://www.lua.org/pil/12.1.html>.)

As it happens, I happened to write an implementation of just
the functionality you're looking for a few days ago.  It is
below, but I encourage you to try whipping together at least
a simple version of your own before looking at mine.

- Aaron
]]

-- This script makes tostring convert tables to a
-- representation of their contents.

-- The real tostring:
local _tostring = tostring

-- Characters that have non-numeric backslash-escaped versions:
local BsChars = {
  ["\a"] = "\\a",
  ["\b"] = "\\b",
  ["\f"] = "\\f",
  ["\n"] = "\\n",
  ["\r"] = "\\r",
  ["\t"] = "\\t",
  ["\v"] = "\\v",
  ["\""] = "\\\"",
  ["\\"] = "\\\\"}

-- Is Str an "escapeable" character (a non-printing character other than
-- space, a backslash, or a double quote)?
local function IsEscapeable(Char)
  return string.find(Char, "[^%w%p]") -- Non-alphanumeric, non-punct.
    and Char ~= " " -- Don't count spaces.
    or string.find(Char, '[\\"]') -- A backslash or quote.
end

-- Converts an "escapeable" character (a non-printing character,
-- backslash, or double quote) to its backslash-escaped version; the
-- second argument is used so that numeric character codes can have one
-- or two digits unless three are necessary, which means that the
-- returned value may represent both the character in question and the
-- digit after it:
local function EscapeableToEscaped(Char, FollowingDigit)
  if IsEscapeable(Char) then
    local Format = FollowingDigit == ""
      and "\\%d"
      or "\\%03d" .. FollowingDigit
    return BsChars[Char]
      or string.format(Format, string.byte(Char))
  else
    return Char .. FollowingDigit
  end
end

-- Quotes a string in a Lua- and human-readable way.  (This is a
-- replacement for string.format's %q placeholder, whose result
-- isn't always human readable.)
local function StrToStr(Str)
  return '"' .. string.gsub(Str, "(.)(%d?)",
    EscapeableToEscaped) .. '"'
end

-- Quote a string into lua form (including the non-printable characters from
-- 0-31, and from 127-255).
local function quote(_)
  local fmt = string.format
  local _ = fmt("%q", _)

  _ = string.gsub(_, "\\\n", "\\n")
  _ = string.gsub(_, "[%z\1-\31,\127-\255]", function (x)
    --print("x=", x)
    return fmt("\\%03d",string.byte(x))
  end)

  return _
end
StrToStr = quote

-- Lua keywords:
local Keywords = {["and"] = true, ["break"] = true, ["do"] = true,
  ["else"] = true, ["elseif"] = true, ["end"] = true, ["false"] = true,
  ["for"] = true, ["function"] = true, ["if"] = true, ["in"] = true,
  ["local"] = true, ["nil"] = true, ["not"] = true, ["or"] = true,
  ["repeat"] = true, ["return"] = true, ["then"] = true,
  ["true"] = true, ["until"] = true, ["while"] = true}

-- Is Str an identifier?
local function IsIdent(Str)
  return not Keywords[Str] and string.find(Str, "^[%a_][%w_]*$")
end

-- Converts a non-table to a Lua- and human-readable string:
local function ScalarToStr(Val)
  local Ret
  local Type = type(Val)
  if Type == "string" then
    Ret = StrToStr(Val)
  elseif Type == "function" or Type == "userdata" or Type == "thread" then
    -- Punt:
    Ret = "<" .. _tostring(Val) .. ">"
  else
    Ret = _tostring(Val)
  end -- if
  return Ret
end

-- Converts a table to a Lua- and human-readable string.
local function TblToStr(Tbl, Seen)
  Seen = Seen or {}
  local Ret = {}
  if not Seen[Tbl] then
    Seen[Tbl] = true
    local LastArrayKey = 0
    for Key, Val in pairs(Tbl) do
      if type(Key) == "table" then
        Key = "[" .. TblToStr(Key, Seen) .. "]"
      elseif not IsIdent(Key) then
        if type(Key) == "number" and Key == LastArrayKey + 1 then
          -- Don't mess with Key if it's an array key.
          LastArrayKey = Key
        else
          Key = "[" .. ScalarToStr(Key) .. "]"
        end
      end
      if type(Val) == "table" then
        Val = TblToStr(Val, Seen)
      else
        Val = ScalarToStr(Val)
      end
      Ret[#Ret + 1] =
        (type(Key) == "string"
          and (Key .. " = ") -- Explicit key.
          or "") -- Implicit array key.
        .. Val
    end
    Ret = "{\n" .. table.concat(Ret, ", ") .. "\n}"
  else
    Ret = "<cycle to " .. _tostring(Tbl) .. ">"
  end
  return Ret
end

-- A replacement for tostring that prints tables in Lua- and
-- human-readable format:
function tostring(Val)
  return type(Val) == "table"
    and TblToStr(Val)
    or _tostring(Val)
end

--[=[

-- A test function:
local function TblToStrTest()
  local Fnc = function() end
  local Tbl = {}
  Tbl[Tbl] = Tbl
  local Tbls = {
    {},
    {1, true, 3},
    {Foo = "Bar"},
    {["*Foo*"] = "Bar"},
    {Fnc},
    {"\0\1\0011\a\b\f\n\r\t\v\"\\"},
    {[{}] = {}},
    Tbl,
  }
  local Strs = {
    "{}",
    "{1, true, 3}",
    '{Foo = "Bar"}',
    '{["*Foo*"] = "Bar"}',
    '{<' .. _tostring(Fnc) .. '>}',
    [[{"\0\1\0011\a\b\f\n\r\t\v\"\\"}]],
    "{[{}] = {}}",
    "{[<cycle to " .. _tostring(Tbl) .. ">] = <cycle to "
      .. _tostring(Tbl) .. ">}",
  }
  for I, Tbl in ipairs(Tbls) do
    assert(tostring(Tbl) == Strs[I],
      "Assertion failed on " .. _tostring(tostring(Tbl)))
  end
end

TblToStrTest()
]=]

