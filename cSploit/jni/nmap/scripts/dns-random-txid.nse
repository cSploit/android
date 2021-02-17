local bit = require "bit"
local comm = require "comm"
local nmap = require "nmap"
local shortport = require "shortport"
local string = require "string"

description = [[
Checks a DNS server for the predictable-TXID DNS recursion
vulnerability.  Predictable TXID values can make a DNS server vulnerable to
cache poisoning attacks (see CVE-2008-1447).

The script works by querying txidtest.dns-oarc.net (see
https://www.dns-oarc.net/oarc/services/txidtest).  Be aware that any
targets against which this script is run will be sent to and
potentially recorded by one or more DNS servers and the txidtest
server. In addition your IP address will be sent along with the
txidtest query to the DNS server running on the target.
]]

license = "Same as Nmap--See http://nmap.org/book/man-legal.html"

author = [[
Script: Brandon Enright <bmenrigh@ucsd.edu>
txidtest.dns-oarc.net: Duane Wessels <wessels@dns-oarc.net>
]]

---
-- @usage
-- nmap -sU -p 53 --script=dns-random-txid <target>
-- @output
-- PORT   STATE SERVICE REASON
-- 53/udp open  domain  udp-response
-- |_dns-random-txid: X.X.X.X is GREAT: 27 queries in 61.5 seconds from 27 txids with std dev 20509

-- This script uses (with permission) Duane Wessels' txidtest.dns-oarc.net
-- service.  Duane/OARC believe the service is valuable to the community
-- and have no plans to ever turn the service off.
-- The likely long-term availability makes this script a good candidate
-- for inclusion in Nmap proper.

categories = {"external", "intrusive"}


portrule = shortport.portnumber(53, "udp")

action = function(host, port)

  -- TXID: 0xbabe
  -- Flags: 0x0100
  -- Questions: 1
  -- Answer RRs: 0
  -- Authority RRs: 0
  -- Additional RRs: 0

  -- Query:
  -- Name: txidtest, dns-oarc, net
  -- Type: TXT (0x0010)
  -- Class: IN (0x0001)

  local query =   string.char(    0xba, 0xbe, -- TXID
    0x01, 0x00, -- Flags
    0x00, 0x01, -- Questions
    0x00, 0x00, -- Answer RRs
    0x00, 0x00, -- Authority RRs
    0x00, 0x00, -- Additional RRs
    0x08) .. "txidtest" ..
  string.char(    0x08) .. "dns-oarc" ..
  string.char(    0x03) .. "net" ..
  string.char(    0x00, -- Name terminator
    0x00, 0x10, -- Type (TXT)
    0x00, 0x01) -- Class (IN)

  local status, result = comm.exchange(host, port, query, {proto="udp",
    timeout=20000})

  -- Fail gracefully
  if not status then
    if (nmap.verbosity() >= 2 or nmap.debugging() >= 1) then
      return "ERROR: TIMEOUT"
    else
      return
    end
  end

  -- Update the port
  nmap.set_port_state(host, port, "open")

  -- Now we need to "parse" the results to check to see if they are good

  -- We need a minimum of 5 bytes...
  if (#result < 5) then
    if (nmap.verbosity() >= 2 or nmap.debugging() >= 1) then
      return "ERROR: Malformed response"
    else
      return
    end
  end

  -- Check TXID
  if (string.byte(result, 1) ~= 0xba
      or string.byte(result, 2) ~= 0xbe) then
    if (nmap.verbosity() >= 2 or nmap.debugging() >= 1) then
      return "ERROR: Invalid Transaction ID"
    else
      return
    end
  end

  -- Check response flag and recursion
  if not (bit.band(string.byte(result, 3), 0x80) == 0x80
      and bit.band(string.byte(result, 4), 0x80) == 0x80) then
    if (nmap.verbosity() >= 1 or nmap.debugging() >= 1) then
      return "ERROR: Server refused recursion"
    else
      return
    end
  end

  -- Check error flag
  if (bit.band(string.byte(result, 4), 0x0F) ~= 0x00) then
    if (nmap.verbosity() >= 1 or nmap.debugging() >= 1) then
      return "ERROR: Server failure"
    else
      return
    end
  end

  -- Check for two Answer RRs and 1 Authority RR
  if (string.byte(result, 5) ~= 0x00
      or string.byte(result, 6) ~= 0x01
      or string.byte(result, 7) ~= 0x00
      or string.byte(result, 8) ~= 0x02) then
    if (nmap.verbosity() >= 2 or nmap.debugging() >= 1) then
      return "ERROR: Response did not include expected answers"
    else
      return
    end
  end

  -- We need a minimum of 128 bytes...
  if (#result < 128) then
    if (nmap.verbosity() >= 2 or nmap.debugging() >= 1) then
      return "ERROR: Truncated response"
    else
      return
    end
  end

  -- Here is the really fragile part.  If the DNS response changes
  -- in any way, this won't work and will fail.
  -- Jump to second answer and check to see that it is TXT, IN
  -- then grab the length and display that text...

  -- Check for TXT
  if (string.byte(result, 118) ~= 0x00
      or string.byte(result, 119) ~= 0x10)
    then
    if (nmap.verbosity() >= 2 or nmap.debugging() >= 1) then
      return "ERROR: Answer record not of type TXT"
    else
      return
    end
  end

  -- Check for IN
  if (string.byte(result, 120) ~= 0x00
      or string.byte(result, 121) ~= 0x01) then
    if (nmap.verbosity() >= 2 or nmap.debugging() >= 1) then
      return "ERROR: Answer record not of type IN"
    else
      return
    end
  end

  -- Get TXT length
  local txtlen = string.byte(result, 128)

  -- We now need a minimum of 128 + txtlen bytes + 1...
  if (#result < 128 + txtlen) then
    if (nmap.verbosity() >= 2 or nmap.debugging() >= 1) then
      return "ERROR: Truncated response"
    else
      return
    end
  end

  -- GET TXT record
  local txtrd = string.sub(result, 129, 128 + txtlen)

  return txtrd
end
