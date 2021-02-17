---
-- URI parsing, composition, and relative URL resolution.
--
-- A URL is represented as a table with the following entries:
-- * <code>scheme</code>
-- * <code>fragment</code>
-- * <code>query</code>
-- * <code>params</code>
-- * <code>authority</code>
-- * <code>userinfo</code>
-- * <code>path</code>
-- * <code>port</code>
-- * <code>password</code>
-- These correspond to these parts of a URL (some may be <code>nil</code>):
-- <code>
-- scheme://userinfo@password:authority:port/path;params?query#fragment
-- </code>
--
-- @author Diego Nehab
-- @author Eddie Bell <ejlbell@gmail.com>

--[[
URI parsing, composition and relative URL resolution
LuaSocket toolkit.
Author: Diego Nehab
RCS ID: $Id: url.lua,v 1.37 2005/11/22 08:33:29 diego Exp $

parse_query and build_query added For nmap (Eddie Bell <ejlbell@gmail.com>)
--]]

-----------------------------------------------------------------------------
-- Declare module
-----------------------------------------------------------------------------

local _G = require "_G"
local stdnse = require "stdnse"
local string = require "string"
local table = require "table"
local base = _G


_ENV = stdnse.module("url", stdnse.seeall)

_VERSION = "URL 1.0"

--[[ Internal functions --]]

local function make_set(t)
  local s = {}
  for i,v in base.ipairs(t) do
    s[t[i]] = 1
  end
  return s
end

-- these are allowed withing a path segment, along with alphanum
-- other characters must be escaped
local segment_set = make_set {
  "-", "_", ".", "!", "~", "*", "'", "(",
  ")", ":", "@", "&", "=", "+", "$", ",",
}

---
-- Protects a path segment, to prevent it from interfering with the
-- URL parsing.
-- @param s Binary string to be encoded.
-- @return Escaped representation of string.
local function protect_segment(s)
  return string.gsub(s, "([^A-Za-z0-9_])", function (c)
    if segment_set[c] then return c
    else return string.format("%%%02x", string.byte(c)) end
  end)
end

---
-- Builds a path from a base path and a relative path
-- @param base_path A base path.
-- @param relative_path A relative path.
-- @return The corresponding absolute path.
-----------------------------------------------------------------------------
local function absolute_path(base_path, relative_path)
  if string.sub(relative_path, 1, 1) == "/" then return relative_path end
  local path = string.gsub(base_path, "[^/]*$", "")
  path = path .. relative_path
  path = string.gsub(path, "([^/]*%./)", function (s)
    if s ~= "./" then return s else return "" end
  end)
  path = string.gsub(path, "/%.$", "/")
  local reduced
  while reduced ~= path do
    reduced = path
    path = string.gsub(reduced, "([^/]*/%.%./)", function (s)
      if s ~= "../../" then return "" else return s end
    end)
  end
  path = string.gsub(reduced, "([^/]*/%.%.)$", function (s)
    if s ~= "../.." then return "" else return s end
  end)
  return path
end


--[[ External functions --]]

---
-- Encodes a string into its escaped hexadecimal representation.
-- @param s Binary string to be encoded.
-- @return Escaped representation of string.
-----------------------------------------------------------------------------
function escape(s)
  return string.gsub(s, "([^A-Za-z0-9_])", function(c)
    return string.format("%%%02x", string.byte(c))
  end)
end


---
-- Decodes an escaped hexadecimal string.
-- @param s Hexadecimal-encoded string.
-- @return Decoded string.
-----------------------------------------------------------------------------
function unescape(s)
  return string.gsub(s, "%%(%x%x)", function(hex)
    return string.char(base.tonumber(hex, 16))
  end)
end


---
-- Parses a URL and returns a table with all its parts according to RFC 2396.
--
-- The following grammar describes the names given to the URL parts.
-- <code>
-- <url> ::= <scheme>://<authority>/<path>;<params>?<query>#<fragment>
-- <authority> ::= <userinfo>@<host>:<port>
-- <userinfo> ::= <user>[:<password>]
-- <path> :: = {<segment>/}<segment>
-- </code>
--
-- The leading <code>/</code> in <code>/<path></code> is considered part of
-- <code><path></code>.
-- @param url URL of request.
-- @param default Table with default values for each field.
-- @return A table with the following fields, where RFC naming conventions have
--   been preserved:
--     <code>scheme</code>, <code>authority</code>, <code>userinfo</code>,
--     <code>user</code>, <code>password</code>, <code>host</code>,
--     <code>port</code>, <code>path</code>, <code>params</code>,
--     <code>query</code>, and <code>fragment</code>.
-----------------------------------------------------------------------------
function parse(url, default)
  -- initialize default parameters
  local parsed = {}
  for i,v in base.pairs(default or parsed) do parsed[i] = v end
  -- remove whitespace
  -- url = string.gsub(url, "%s", "")
  -- get fragment
  url = string.gsub(url, "#(.*)$", function(f)
    parsed.fragment = f
    return ""
  end)
  -- get scheme. Lower-case according to RFC 3986 section 3.1.
  url = string.gsub(url, "^([%w][%w%+%-%.]*)%:",
  function(s) parsed.scheme = string.lower(s); return "" end)
  -- get authority
  url = string.gsub(url, "^//([^/]*)", function(n)
    parsed.authority = n
    return ""
  end)
  -- get query stringing
  url = string.gsub(url, "%?(.*)", function(q)
    parsed.query = q
    return ""
  end)
  -- get params
  url = string.gsub(url, "%;(.*)", function(p)
    parsed.params = p
    return ""
  end)
  -- path is whatever was left
  parsed.path = url
  local authority = parsed.authority
  if not authority then return parsed end
  authority = string.gsub(authority,"^([^@]*)@",
  function(u) parsed.userinfo = u; return "" end)
  authority = string.gsub(authority, ":([0-9]*)$",
  function(p) if p ~= "" then parsed.port = p end; return "" end)
  if authority ~= "" then parsed.host = authority end
  local userinfo = parsed.userinfo
  if not userinfo then return parsed end
  userinfo = string.gsub(userinfo, ":([^:]*)$",
  function(p) parsed.password = p; return "" end)
  parsed.user = userinfo
  return parsed
end

---
-- Rebuilds a parsed URL from its components.
--
-- Components are protected if any reserved or disallowed characters are found.
-- @param parsed Parsed URL, as returned by parse.
-- @return A string with the corresponding URL.
-----------------------------------------------------------------------------
function build(parsed)
  local ppath = parse_path(parsed.path or "")
  local url = build_path(ppath)
  if parsed.params then url = url .. ";" .. parsed.params end
  if parsed.query then url = url .. "?" .. parsed.query end
  local authority = parsed.authority
  if parsed.host then
    authority = parsed.host
    if parsed.port then authority = authority .. ":" .. parsed.port end
    local userinfo = parsed.userinfo
    if parsed.user then
      userinfo = parsed.user
      if parsed.password then
        userinfo = userinfo .. ":" .. parsed.password
      end
    end
    if userinfo then authority = userinfo .. "@" .. authority end
  end
  if authority then url = "//" .. authority .. url end
  if parsed.scheme then url = parsed.scheme .. ":" .. url end
  if parsed.fragment then url = url .. "#" .. parsed.fragment end
  -- url = string.gsub(url, "%s", "")
  return url
end

---
-- Builds an absolute URL from a base and a relative URL according to RFC 2396.
-- @param base_url A base URL.
-- @param relative_url A relative URL.
-- @return The corresponding absolute URL.
-----------------------------------------------------------------------------
function absolute(base_url, relative_url)
  local base_parsed;
  if type(base_url) == "table" then
    base_parsed = base_url
    base_url = build(base_parsed)
  else
    base_parsed = parse(base_url)
  end
  local relative_parsed = parse(relative_url)
  if not base_parsed then return relative_url
  elseif not relative_parsed then return base_url
  elseif relative_parsed.scheme then return relative_url
  else
    relative_parsed.scheme = base_parsed.scheme
    if not relative_parsed.authority then
      relative_parsed.authority = base_parsed.authority
      if not relative_parsed.path then
        relative_parsed.path = base_parsed.path
        if not relative_parsed.params then
          relative_parsed.params = base_parsed.params
          if not relative_parsed.query then
            relative_parsed.query = base_parsed.query
          end
        end
      else
        relative_parsed.path = absolute_path(base_parsed.path or "",
        relative_parsed.path)
      end
    end
    return build(relative_parsed)
  end
end

---
-- Breaks a path into its segments, unescaping the segments.
-- @param path A path to break.
-- @return A table with one entry per segment.
-----------------------------------------------------------------------------
function parse_path(path)
  local parsed = {}
  path = path or ""
  --path = string.gsub(path, "%s", "")
  string.gsub(path, "([^/]+)", function (s) table.insert(parsed, s) end)
  for i, v in ipairs(parsed) do
    parsed[i] = unescape(v)
  end
  if string.sub(path, 1, 1) == "/" then parsed.is_absolute = 1 end
  if string.sub(path, -1, -1) == "/" then parsed.is_directory = 1 end
  return parsed
end

---
-- Builds a path component from its segments, escaping protected characters.
-- @param parsed Path segments.
-- @param unsafe If true, segments are not protected before path is built.
-- @return The corresponding path string
-----------------------------------------------------------------------------
function build_path(parsed, unsafe)
  local path = {}
  if parsed.is_absolute then path[#path+1] = "/" end
  local n = #parsed
  if unsafe then
    for i = 1, n-1 do
      path[#path+1] = parsed[i] .. "/"
    end
    if n > 0 then
      path[#path+1] = parsed[n]
      if parsed.is_directory then path[#path+1] = "/" end
    end
  else
    for i = 1, n-1 do
      path[#path+1] = protect_segment(parsed[i]) .. "/"
    end
    if n > 0 then
      path[#path+1] = protect_segment(parsed[n])
      if parsed.is_directory then path[#path+1] = "/" end
    end
  end
  return table.concat(path)
end

---
-- Breaks a query string into name/value pairs.
--
-- This function takes a <code><query></code> of the form
-- <code>"name1=value1&name2=value2"</code>
-- and returns a table containing the name-value pairs, with the name as the key
-- and the value as its associated value.
-- @param query Query string.
-- @return A table of name-value pairs following the pattern
-- <code>table["name"]</code> = <code>value</code>.
-----------------------------------------------------------------------------
function parse_query(query)
  local parsed = {}
  local pos = 0

  query = string.gsub(query, "&amp;", "&")
  query = string.gsub(query, "&lt;", "<")
  query = string.gsub(query, "&gt;", ">")

  local function ginsert(qstr)
    local first, last = string.find(qstr, "=")
    if first then
      parsed[string.sub(qstr, 0, first-1)] = string.sub(qstr, first+1)
    end
  end

  while true do
    local first, last = string.find(query, "&", pos)
    if first then
      ginsert(string.sub(query, pos, first-1));
      pos = last+1
    else
      ginsert(string.sub(query, pos));
      break;
    end
  end
  return parsed
end

---
-- Builds a query string from a table.
--
-- This is the inverse of <code>parse_query</code>.
-- @param query A dictionary table where <code>table['name']</code> =
-- <code>value</code>.
-- @return A query string (like <code>"name=value2&name=value2"</code>).
-----------------------------------------------------------------------------
function build_query(query)
  local qstr = ""

  for i,v in pairs(query) do
    qstr = qstr .. i .. '=' .. v .. '&'
  end
  return string.sub(qstr, 0, #qstr-1)
end

return _ENV;
