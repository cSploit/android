local nmap = require "nmap"
local shortport = require "shortport"
local stdnse = require "stdnse"
local table = require "table"
local bin = require "bin"
local os = require "os"
local tls = require "tls"

description = [[
Enumerates a TLS server's supported protocols by using the next protocol negotiation extension.

This works by adding the next protocol negotiation extension in the client hello
packet and parsing the returned server hello's NPN extension data.

For more information , see:
    * https://tools.ietf.org/html/draft-agl-tls-nextprotoneg-03
]]

---
-- @usage
-- nmap --script=tls-nextprotoneg <targets>
--
--@output
-- 443/tcp open  https
-- | tls-nextprotoneg:
-- |   spdy/3
-- |   spdy/2
-- |_  http/1.1
--
-- @xmloutput
-- <elem>spdy/4a4</elem>
-- <elem>spdy/3.1</elem>
-- <elem>spdy/3</elem>
-- <elem>http/1.1</elem>


author = "Hani Benhabiles"

license = "Same as Nmap--See http://nmap.org/book/man-legal.html"

categories = {"discovery", "safe", "default"}

portrule = shortport.ssl


--- Function that sends a client hello packet with the TLS NPN extension to the
-- target host and returns the response
--@args host The target host table.
--@args port The target port table.
--@return status true if response, false else.
--@return response if status is true.
local client_hello = function(host, port)
  local sock, status, response, err, cli_h

  cli_h = tls.client_hello({
    ["protocol"] = "TLSv1.0",
    ["ciphers"] = {
      "TLS_ECDHE_RSA_WITH_RC4_128_SHA",
      "TLS_DHE_RSA_WITH_AES_256_CBC_SHA",
      "TLS_RSA_WITH_RC4_128_MD5",
    },
    ["compressors"] = {"NULL"},
    ["extensions"] = {
      ["next_protocol_negotiation"] = "",
    },
  })

  -- Connect to the target server
  sock = nmap.new_socket()
  sock:set_timeout(5000)
  status, err = sock:connect(host, port)
  if not status then
    sock:close()
    stdnse.print_debug("Can't send: %s", err)
    return false
  end

  -- Send Client Hello to the target server
  status, err = sock:send(cli_h)
  if not status then
    stdnse.print_debug("Couldn't send: %s", err)
    sock:close()
    return false
  end

  -- Read response
  status, response, err = tls.record_buffer(sock)
  if not status then
    stdnse.print_debug("Couldn't receive: %s", err)
    sock:close()
    return false
  end

  return true, response
end

--- Function that checks for the returned protocols to a npn extension request.
--@args response Response to parse.
--@return results List of found protocols.
local check_npn = function(response)
  local i, record = tls.record_read(response, 0)
  if record == nil then
    stdnse.print_debug("%s: Unknown response from server", SCRIPT_NAME)
    return nil
  end

  if record.type == "handshake" and record.body[1].type == "server_hello" then
    if record.body[1].extensions == nil then
      stdnse.print_debug("%s: Server does not support TLS NPN extension.", SCRIPT_NAME)
      return nil
    end
    local results = {}
    local npndata = record.body[1].extensions["next_protocol_negotiation"]
    if npndata == nil then
      stdnse.print_debug("%s: Server does not support TLS NPN extension.", SCRIPT_NAME)
      return nil
    end
    -- Parse data
    i = 0
    local protocol
    while i < #npndata do
      i, protocol = bin.unpack(">p", npndata, i)
      table.insert(results, protocol)
    end

    return results
  else
    stdnse.print_debug("%s: Server response was not server_hello", SCRIPT_NAME)
    return nil
  end
end

action = function(host, port)
  local status, response

  -- Send crafted client hello
  status, response = client_hello(host, port)
  if status and response then
    -- Analyze response
    local results = check_npn(response)
    return results
  end
end
