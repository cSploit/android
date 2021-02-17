local bin = require "bin"
local bit = require "bit"
local nmap = require "nmap"
local rpc = require "rpc"
local shortport = require "shortport"
local stdnse = require "stdnse"
local table = require "table"

description = [[
Detects vulnerabilities and gathers information (such as version
numbers and hardware support) from VxWorks Wind DeBug agents.

Wind DeBug is a SunRPC-type service that is enabled by default on many devices
that use the popular VxWorks real-time embedded operating system. H.D. Moore
of Metasploit has identified several security vulnerabilities and design flaws
with the service, including weakly-hashed passwords and raw memory dumping.

See also:
http://www.kb.cert.org/vuls/id/362332
]]

---
-- @usage
-- nmap -sU -p 17185 --script wdb-version <target>
-- @output
-- 17185/udp open  wdb  Wind DeBug Agent 2.0
-- | wdb-version:
-- |   VULNERABLE: Wind River Systems VxWorks debug service enabled. See http://www.kb.cert.org/vuls/id/362332
-- |   Agent version: 2.0
-- |   VxWorks version: VxWorks5.4.2
-- |   Board Support Package: PCD ARM940T REV 1
-- |   Boot line: host:vxWorks.z
--@xmloutput
-- <elem>VULNERABLE: Wind River Systems VxWorks debug service enabled. See http://www.kb.cert.org/vuls/id/362332</elem>
-- <elem key="Agent version">2.0</elem>
-- <elem key="VxWorks version">5.4</elem>
-- <elem key="Board Support Package">Alcatel CMM MPC8245/100</elem>
-- <elem key="Boot line">lanswitchCmm:</elem>

author = "Daniel Miller"
license = "Same as Nmap--See http://nmap.org/book/man-legal.html"
-- may also be "safe", but need testing to determine
categories = {"default", "version", "discovery", "vuln"}


-- WDB protocol information
-- http://www.vxdev.com/docs/vx55man/tornado-api/wdbpcl/wdb.html
-- http://www.verysource.com/code/2817990_1/wdb.h.html
-- Metasploit scanner module
-- http://www.metasploit.com/redmine/projects/framework/repository/entry/lib/msf/core/exploit/wdbrpc.rb

portrule = shortport.version_port_or_service(17185, "wdbrpc", {"udp"} )

rpc.RPC_version["wdb"] = { min=1, max=1 }

local WDB_Procedure = {
  ["WDB_TARGET_PING"] = 0,
  ["WDB_TARGET_CONNECT"] = 1,
  ["WDB_TARGET_DISCONNECT"] = 2,
  ["WDB_TARGET_MODE_SET"] = 3,
  ["WDB_TARGET_MODE_GET"] = 4,
}

local function checksum(data)
  local sum = 0
  local p = 0
  local _
  p, _ = bin.unpack(">I", data)
  while p < data:len() do
    local c
    p, c = bin.unpack(">S", data, p)
    sum = sum + c
  end
  sum = bit.band(sum, 0xffff) + bit.rshift(sum, 16)
  return bit.bnot( sum )
end

local seqnum = 0
local function seqno()
  seqnum = seqnum + 1
  return seqnum
end

local function request(comm, procedure, data)
  local packet = comm:EncodePacket( nil, procedure, {type = rpc.Portmap.AuthType.NULL}, nil )
  local wdbwrapper = bin.pack( ">I2", data:len() + packet:len() + 8, seqno() )
  local sum = checksum(packet..bin.pack(">I", 0x00000000)..wdbwrapper..data)
  return packet .. bin.pack(">S2", 0xffff, sum) .. wdbwrapper .. data
end

local function stripnull(str)
  local e = -1
  while str:byte(e) == 0 do
    e = e - 1
  end
  return str:sub(1,e)
end

local function decode_reply(data, pos)
  local wdberr, len
  local done = data:len()
  local info = {}
  local _
  pos, _ = rpc.Util.unmarshall_uint32(data, pos)
  pos, _ = rpc.Util.unmarshall_uint32(data, pos)
  pos, wdberr = rpc.Util.unmarshall_uint32(data, pos)
  info["error"] = bit.band(wdberr, 0xc0000000)
  if (info["error"] ~= 0x00000000 ) then
    stdnse.print_debug(1,"Error from decode_reply: %x", info["error"])
    return nil, info
  end
  pos, len = rpc.Util.unmarshall_uint32(data, pos)
  if (len ~= 0) then
    pos, info["agent_ver"] = rpc.Util.unmarshall_vopaque(len, data, pos)
  end
  pos, info["agent_mtu"] = rpc.Util.unmarshall_uint32(data, pos)
  pos, info["agent_mod"] = rpc.Util.unmarshall_uint32(data, pos)
  pos, info["rt_type"]          = rpc.Util.unmarshall_uint32(data, pos)
  pos, len = rpc.Util.unmarshall_uint32(data, pos)
  if (pos == done) then return pos, info end
  if (len ~= 0) then
    pos, info["rt_vers"]          = rpc.Util.unmarshall_vopaque(len, data, pos)
  end
  pos, info["rt_cpu_type"]      = rpc.Util.unmarshall_uint32(data, pos)
  pos, len       = rpc.Util.unmarshall_uint32(data, pos)
  info["rt_has_fpp"]       = ( len ~= 0 )
  pos, len       = rpc.Util.unmarshall_uint32(data, pos)
  info["rt_has_wp"]       = ( len ~= 0 )
  pos, info["rt_page_size"]     = rpc.Util.unmarshall_uint32(data, pos)
  pos, info["rt_endian"]        = rpc.Util.unmarshall_uint32(data, pos)
  pos, len = rpc.Util.unmarshall_uint32(data, pos)
  if (len ~= 0) then
    pos, info["rt_bsp_name"]      = rpc.Util.unmarshall_vopaque(len, data, pos)
  end
  pos, len = rpc.Util.unmarshall_uint32(data, pos)
  if (len ~= 0) then
    pos, info["rt_bootline"]      = rpc.Util.unmarshall_vopaque(len, data, pos)
  end
  if (pos == done) then return pos, info end
  pos, info["rt_membase"]       = rpc.Util.unmarshall_uint32(data, pos)
  if (pos == done) then return pos, info end
  pos, info["rt_memsize"]       = rpc.Util.unmarshall_uint32(data, pos)
  if (pos == done) then return pos, info end
  pos, info["rt_region_count"]  = rpc.Util.unmarshall_uint32(data, pos)
  if (pos == done) then return pos, info end
  pos, len = rpc.Util.unmarshall_uint32(data, pos)
  if (len ~= 0) then
    info["rt_regions"] = {}
    for i = 1, len do
      pos, info["rt_regions"][i] = rpc.Util.unmarshall_uint32(data, pos)
    end
  end
  if (pos == done) then return pos, info end
  pos, len = rpc.Util.unmarshall_uint32(data, pos)
  if (len == nil) then return pos, info end
  if (len ~= 0) then
    pos, info["rt_hostpool_base"] = rpc.Util.unmarshall_vopaque(len, data, pos)
  end
  if (pos == done) then return pos, info end
  pos, len = rpc.Util.unmarshall_uint32(data, pos)
  if (len ~= 0) then
    pos, info["rt_hostpool_size"] = rpc.Util.unmarshall_vopaque(len, data, pos)
  end
  return pos, info
end

action = function(host, port)
  local comm = rpc.Comm:new("wdb", 1)
  local status, err, data, pos, header
  local info = {}
  status, err = comm:Connect(host, port)
  if (not(status)) then
    return stdnse.format_output(false, err)
  end
  comm.socket:set_timeout(3000)
  local packet = request(comm, WDB_Procedure["WDB_TARGET_CONNECT"], bin.pack(">I3", 0x00000002, 0x00000000, 0x00000000))
  if (not(comm:SendPacket(packet))) then
    return stdnse.format_output(false, "Failed to send request")
  end

  status, data = comm:ReceivePacket()
  if (not(status)) then
    --return stdnse.format_output(false, "Failed to read data")
    return nil
  end
  nmap.set_port_state(host, port, "open")

  pos = 0
  pos, header = comm:DecodeHeader(data, pos)
  if not header then
    return stdnse.format_output(false, "Failed to decode header")
  end

  if ( pos == data:len() ) then
    return stdnse.format_output(false, "No WDB data in reply")
  end

  pos, info = decode_reply(data, pos)
  if not pos then
    return stdnse.format_output(false, "WDB error: "..info.error)
  end
  port.version.name = "wdb"
  port.version.name_confidence = 10
  port.version.product = "Wind DeBug Agent"
  port.version.version = stripnull(info["agent_ver"])
  if (port.version.ostype ~= nil) then
    port.version.ostype = "VxWorks " .. stripnull(info["rt_vers"])
  end
  nmap.set_port_version(host, port)
  -- Clean up (some agents will continue to send data until we disconnect)
  packet = request(comm, WDB_Procedure["WDB_TARGET_DISCONNECT"], bin.pack(">I3", 0x00000002, 0x00000000, 0x00000000))
  if (not(comm:SendPacket(packet))) then
    return stdnse.format_output(false, "Failed to send request")
  end

  local o = stdnse.output_table()
  table.insert(o, "VULNERABLE: Wind River Systems VxWorks debug service enabled. See http://www.kb.cert.org/vuls/id/362332")
  if (info.agent_ver) then
    o["Agent version"] = stripnull(info.agent_ver)
  end
  --table.insert(o, "Agent MTU: " .. info.agent_mtu)
  if (info.rt_vers) then
    o["VxWorks version"] = stripnull(info.rt_vers)
  end
  -- rt_cpu_type is an enum type, but I don't have access to
  -- cputypes.h, where it is defined
  --table.insert(o, "CPU Type: " .. info.rt_cpu_type)
  if (info.rt_bsp_name) then
    o["Board Support Package"] = stripnull(info.rt_bsp_name)
  end
  if (info.rt_bootline) then
    o["Boot line"] = stripnull(info.rt_bootline)
  end
  return o
end
