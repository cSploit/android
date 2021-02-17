local dhcp = require "dhcp"
local math = require "math"
local nmap = require "nmap"
local shortport = require "shortport"
local stdnse = require "stdnse"
local string = require "string"
local table = require "table"

description = [[
Sends a DHCPINFORM request to a host on UDP port 67 to obtain all the local configuration parameters
without allocating a new address.

DHCPINFORM is a DHCP request that returns useful information from a DHCP server, without allocating an IP
address. The request sends a list of which fields it wants to know (a handful by default, every field if
verbosity is turned on), and the server responds with the fields that were requested. It should be noted
that the server doesn't have to return every field, nor does it have to return them in the same order,
or honour the request at all. A Linksys WRT54g, for example, completely ignores the list of requested
fields and returns a few standard ones. This script displays every field it receives.

With script arguments, the type of DHCP request can be changed, which can lead to interesting results.
Additionally, the MAC address can be randomized, which in should override the cache on the DHCP server and
assign a new IP address. Extra requests can also be sent to exhaust the IP address range more quickly.

Some of the more useful fields:
* DHCP Server (the address of the server that responded)
* Subnet Mask
* Router
* DNS Servers
* Hostname
]]

---
-- @args dhcptype The type of DHCP request to make. By default,  DHCPINFORM is sent, but this
--                argument can change it to DHCPOFFER,  DHCPREQUEST, DHCPDECLINE, DHCPACK, DHCPNAK,
--                DHCPRELEASE or DHCPINFORM. Not all types will evoke a response from all servers,
--                and many require different fields to contain specific values.
-- @args randomize_mac Set to <code>true</code> or <code>1</code> to  send a random MAC address with
--                the request (keep in mind that you may  not see the response). This should
--                cause the router to reserve a new  IP address each time.
-- @args requests Set to an integer to make up to  that many requests (and display the results).
--
-- @usage
-- nmap -sU -p 67 --script=dhcp-discover <target>
-- @output
-- Interesting ports on 192.168.1.1:
-- PORT   STATE SERVICE
-- 67/udp open  dhcps
-- |  dhcp-discover:
-- |  |  DHCP Message Type: DHCPACK
-- |  |  Server Identifier: 192.168.1.1
-- |  |  IP Address Lease Time: 1 day, 0:00:00
-- |  |  Subnet Mask: 255.255.255.0
-- |  |  Router: 192.168.1.1
-- |_ |_ Domain Name Server: 208.81.7.10, 208.81.7.14
--

--
-- 2011-12-28 - Revised by Patrik Karlsson <patrik@cqure.net>
--   o Removed DoS code and placed script into discovery and safe categories
--
-- 2011-12-27 - Revised by Patrik Karlsson <patrik@cqure.net>
--   o Changed script to use DHCPINFORM instead of DHCPDISCOVER
--


author = "Ron Bowes"

license = "Same as Nmap--See http://nmap.org/book/man-legal.html"

categories = {"discovery", "safe"}


-- We want to run against a specific host if UDP/67 is open
function portrule(host, port)
  if nmap.address_family() ~= 'inet' then
    stdnse.print_debug("%s is IPv4 compatible only.", SCRIPT_NAME)
    return false
  end

  return shortport.portnumber(67, "udp")(host, port)
end

local function go(host, port)

  -- Build and send a DHCP request using the specified request type, or DHCPINFORM
  local requests = tonumber(nmap.registry.args.requests or 1)
  local results = {}
  for i = 1, requests, 1 do
    -- Decide which type of request to make
    local request_type = dhcp.request_types[nmap.registry.args.dhcptype or "DHCPINFORM"]
    if(request_type == nil) then
      return false, "Valid request types: " .. stdnse.strjoin(", ", dhcp.request_types_str)
    end

    -- Generate the MAC address, if it's random
    local mac_addr = host.mac_addr_src
    if(nmap.registry.args.randomize_mac == 'true' or nmap.registry.args.randomize_mac == '1') then
      stdnse.print_debug(2, "dhcp-discover: Generating a random MAC address")
      mac_addr = ""
      for j=1, 6, 1 do
        mac_addr = mac_addr .. string.char(math.random(1, 255))
      end
    end

    local iface, err = nmap.get_interface_info(host.interface)
    if ( not(iface) or not(iface.address) ) then
      return false, "Couldn't determine local ip for interface: " .. host.interface
    end

    local status, result = dhcp.make_request(host.ip, request_type, iface.address, mac_addr)
    if( not(status) ) then
      stdnse.print_debug(1, "dhcp-discover: Couldn't send DHCP request: %s", result)
      return false, result
    end

    table.insert(results, result)
  end

  -- Done!
  return true, results
end

action = function(host, port)
  local status, results = go(host, port)


  if(not(status)) then
    return stdnse.format_output(false, results)
  end

  if(not(results)) then
    return nil
  end

  -- Set the port state to open
  if(host) then
    nmap.set_port_state(host, port, "open")
  end

  local response = {}

  -- Display the results
  for i, result in ipairs(results) do
    local result_table = {}

    if ( nmap.registry.args.dhcptype and
      "DHCPINFORM" ~= nmap.registry.args.dhcptype ) then
      table.insert(result_table, string.format("IP Offered: %s", result.yiaddr_str))
    end
    for _, v in ipairs(result.options) do
      if(type(v['value']) == 'table') then
        table.insert(result_table, string.format("%s: %s", v['name'], stdnse.strjoin(", ", v['value'])))
      else
        table.insert(result_table, string.format("%s: %s\n", v['name'], v['value']))
      end
    end

    if(#results == 1) then
      response = result_table
    else
      result_table['name'] = string.format("Result %d of %d", i, #results)
      table.insert(response, result_table)
    end
  end

  return stdnse.format_output(true, response)
end




