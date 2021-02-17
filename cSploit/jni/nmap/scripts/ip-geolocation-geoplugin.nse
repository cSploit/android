local http = require "http"
local ipOps = require "ipOps"
local json = require "json"
local stdnse = require "stdnse"
local table = require "table"

description = [[
Tries to identify the physical location of an IP address using the
Geoplugin geolocation web service (http://www.geoplugin.com/). There
is no limit on lookups using this service.
]]

---
-- @usage
-- nmap --script ip-geolocation-geoplugin <target>
--
-- @output
-- | ip-geolocation-geoplugin:
-- | 74.207.244.221 (scanme.nmap.org)
-- |   coordinates (lat,lon): 39.4208984375,-74.497703552246
-- |_  state: New Jersey, United States
--

author = "Gorjan Petrovski"
license = "Same as Nmap--See http://nmap.org/book/man-legal.html"
categories = {"discovery","external","safe"}


hostrule = function(host)
  local is_private, err = ipOps.isPrivate( host.ip )
  if is_private == nil then
    stdnse.print_debug( "%s Error in Hostrule: %s.", SCRIPT_NAME, err )
    return false
  end
  return not is_private
end

-- No limit on requests
local geoplugin = function(ip)
  local response = http.get("www.geoplugin.net", 80, "/json.gp?ip="..ip, nil)
  local stat, loc = json.parse(response.body)
  if not stat then return nil end

  local output = {}
  table.insert(output, "coordinates (lat,lon): "..loc.geoplugin_latitude..","..loc.geoplugin_longitude)
  local regionName = (loc.geoplugin_regionName == json.NULL) and "Unknown" or loc.geoplugin_regionName
  table.insert(output,"state: ".. regionName ..", ".. loc.geoplugin_countryName)

  return output
end

action = function(host,port)
  local output = geoplugin(host.ip)

  if(#output~=0) then
    output.name = host.ip
    if host.targetname then
      output.name = output.name.." ("..host.targetname..")"
    end
  end

  return stdnse.format_output(true,output)
end
