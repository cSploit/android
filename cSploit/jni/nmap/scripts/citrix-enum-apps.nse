local bin = require "bin"
local nmap = require "nmap"
local shortport = require "shortport"
local stdnse = require "stdnse"
local string = require "string"
local table = require "table"

description = [[
Extracts a list of published applications from the ICA Browser service.
]]

---
-- @usage sudo ./nmap -sU --script=citrix-enum-apps -p 1604 <host>
--
-- @output
-- PORT     STATE SERVICE
-- 1604/udp open  unknown
-- 1604/udp open  unknown
-- | citrix-enum-apps:
-- |   Notepad
-- |   iexplorer
-- |_  registry editor
--

-- Version 0.2

-- Created 11/24/2009 - v0.1 - created by Patrik Karlsson <patrik@cqure.net>
-- Revised 11/25/2009 - v0.2 - fixed multiple packet response bug

author = "Patrik Karlsson"

license = "Same as Nmap--See http://nmap.org/book/man-legal.html"

categories = {"discovery","safe"}


portrule = shortport.portnumber(1604, "udp")


-- process the response from the server
-- @param response string, complete server response
-- @return string row delimited with \n containing all published applications
function process_pa_response(response)

  local pos, packet_len = bin.unpack("SS", response)
  local app_name
  local pa_list = {}

  if packet_len < 40 then
    return
  end

  -- the list of published applications starts at offset 40
  local offset = 41

  while offset < packet_len do
    pos, app_name = bin.unpack("z", response:sub(offset))
    offset = offset + pos - 1

    table.insert(pa_list, app_name)
  end

  return pa_list

end


action = function(host, port)

  local packet, counter
  local query = {}
  local pa_list = {}

  --
  -- Packets were intercepted from the Citrix Program Neighborhood client
  -- They are used to query a server for it's list of servers
  --
  -- We're really not interested in the responses to the first two packets
  -- The third response contains the list of published applications
  -- I couldn't find any documentation on this protocol so I'm providing
  -- some brief information for the bits and bytes this script uses.
  --
  -- Spec. of response to query[2] that contains a list of published apps
  --
  -- offset size   content
  -- -------------------------
  -- 0      16-bit Length
  -- 12     32-bit Server IP (not used here)
  -- 30     8-bit  Last packet(1), More packets(0)
  -- 40     -      null-separated list of applications
  --
  query[0] = string.char(
    0x1e, 0x00, -- Length: 30
    0x01, 0x30, 0x02, 0xfd, 0xa8, 0xe3, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
  )

  query[1] = string.char(
    0x20, 0x00, -- Length: 32
    0x01, 0x36, 0x02, 0xfd, 0xa8, 0xe3, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  )

  query[2] = string.char(
    0x2a, 0x00, -- Length: 42
    0x01, 0x32, 0x02, 0xfd, 0xa8, 0xe3, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x21, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  )

  counter = 0

  local socket = nmap.new_socket()
  socket:set_timeout(5000)

  local try = nmap.new_try(function() socket:close() end)

  try( socket:connect(host, port) )

  -- send the two first packets and never look back
  repeat
    try( socket:send(query[counter])  )
    packet = try(socket:receive())
    counter = counter + 1
  until (counter>#query)

  -- process the first response
  pa_list = process_pa_response( packet )

  --
  -- the byte at offset 31 in the response has a really magic function
  -- if it is set to zero (0) we have more response packets to process
  -- if it is set to one (1) we have arrived at the last packet of our journey
  --
  while packet:sub(31,31) ~= string.char(0x01) do
    packet = try( socket:receive() )
    local tmp_table = process_pa_response( packet )

    for _,v in pairs(tmp_table) do
      table.insert(pa_list, v)
    end

  end

  -- set port to open
  if #pa_list>0 then
    nmap.set_port_state(host, port, "open")
  end

  socket:close()

  return stdnse.format_output(true, pa_list)

end
