local comm = require "comm"
local shortport = require "shortport"
local stdnse = require "stdnse"
local bin = require "bin"
local nmap = require "nmap"
local os = require "os"

description = [[
Retrieves the day and time from the Time service.
]]

---
-- @output
-- PORT   STATE SERVICE
-- 37/tcp open  time
-- |_rfc868-time: 2013-10-23T10:33:00

author = "Daniel Miller"

license = "Same as Nmap--See http://nmap.org/book/man-legal.html"

categories = {"discovery", "safe", "version"}


portrule = shortport.port_or_service(37, "time", {"tcp", "udp"})

action = function(host, port)
  local status, result = comm.exchange(host, port, "", {bytes=4, proto=port.protocol})

  if status then
    local _, stamp
    local width = #result
    if width == 4 then
      _, stamp = bin.unpack(">I", result)
      port.version.extrainfo = "32 bits"
    elseif width == 8 then
      _, stamp = bin.unpack(">I", result)
      port.version.extrainfo = "64 bits"
    else
      stdnse.print_debug(1, "Odd response: %s", stdnse.filename_escape(result))
      return nil
    end

    -- RFC 868 epoch is Jan 1, 1900
    stamp = stamp - 2208988800

    -- Make sure we don't stomp a more-likely service detection.
    if port.version.name == "time" then
      local diff = os.difftime(stamp,os.time())
      if diff < 0 then diff = -diff end
      -- confidence decreases by 1 for each year the time is off.
      stdnse.print_debug(1, "Time difference: %d seconds (%0.2f years)", diff, diff / 31556926)
      local confidence = 10 - diff / 31556926
      if confidence < 0 then confidence = 0 end
      port.version.name_confidence = confidence
      nmap.set_port_version(host, port, "hardmatched")
    end

    return stdnse.format_timestamp(stamp)
  end
end
