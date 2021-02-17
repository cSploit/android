local brute = require "brute"
local creds = require "creds"
local nmap = require "nmap"
local shortport = require "shortport"
local stdnse = require "stdnse"
local string = require "string"
local bit = require "bit"
local bin = require "bin"
local table = require "table"
description = [[
Performs brute force password auditing against the pcAnywhere remote access protocol.

Due to certain limitations of the protocol, bruteforcing
is limited to single thread at a time.
After a valid login pair is guessed the script waits
some time until server becomes available again.

]]

---
-- @usage
-- nmap --script=pcanywhere-brute <target>
--
-- @output
-- 5631/tcp open  pcanywheredata syn-ack
-- | pcanywhere-brute:
-- |   Accounts
-- |     administrator:administrator - Valid credentials
-- |   Statistics
-- |_    Performed 2 guesses in 55 seconds, average tps: 0
--
-- @args pcanywhere-brute.timeout socket timeout for connecting to PCAnywhere (default 10s)


author = "Aleksandar Nikolic"
license = "Same as Nmap--See http://nmap.org/book/man-legal.html"
categories = {"intrusive", "brute"}


portrule = shortport.port_or_service(5631, "pcanywheredata")

local arg_timeout = stdnse.parse_timespec(stdnse.get_script_args(SCRIPT_NAME .. ".timeout"))
arg_timeout = (arg_timeout or 10) * 1000

-- implements simple xor based encryption which the server expects
local function encrypt(data)
  local result = {}
  local xor_key = 0xab
  local k = 0
  if data then
    result[1] = bit.bxor(string.byte(data),xor_key)
    for i = 2,string.len(data) do
      result[i] = bit.bxor(result[i-1],string.byte(data,i),i-2)
    end
  end
  return string.char(table.unpack(result))
end

local retry = false -- true means we found valid login and need to wait

Driver = {

  new = function(self, host, port)
    local o = {}
    setmetatable(o, self)
    self.__index = self
    o.host = host
    o.port = port
    return o
  end,

  connect = function( self )
    self.socket = nmap.new_socket()
    local response
    local err
    local status = false

    stdnse.sleep(2)
    -- when we hit a valid login pair, server enters some kind of locked state
    -- so we need to wait for some time before trying next pair
    -- variable "retry" signifies if we need to wait or this is just not pcAnywhere server
    while not status do
      status, err = self.socket:connect(self.host, self.port)
      self.socket:set_timeout(arg_timeout)
      if(not(status)) then
        return false, brute.Error:new( "Couldn't connect to host: " .. err )
      end
      status, err = self.socket:send(bin.pack("H","00000000")) --initial hello
      status, response = self.socket:receive_bytes(0)
      if not status and not retry then
        break
      end
      stdnse.print_debug("in a loop")
      stdnse.sleep(2) -- needs relatively big timeout between retries
    end
    if not status or string.find(response,"Please press <Enter>") == nil then
      --probably not pcanywhere
      stdnse.print_debug(1, "%s: not pcAnywhere", SCRIPT_NAME)
      return false, brute.Error:new( "Probably not pcAnywhere." )
    end
    retry = false
    status, err = self.socket:send(bin.pack("H","6f06ff")) -- downgrade into legacy mode
    status, response = self.socket:receive_bytes(0)

    status, err = self.socket:send(bin.pack("H","6f61000900fe0000ffff00000000")) -- auth capabilities I
    status, response = self.socket:receive_bytes(0)

    status, err = self.socket:send(bin.pack("H","6f620102000000")) -- auth capabilities II
    status, response = self.socket:receive_bytes(0)
    if not status or (string.find(response,"Enter user name") == nil and string.find(response,"Enter login name") == nil) then
      stdnse.print_debug(1, "%s: handshake failed", SCRIPT_NAME)
      return false, brute.Error:new( "Handshake failed." )
    end
    return true
  end,

  login = function (self, user, pass)
    local response
    local err
    local status
    stdnse.print_debug( "Trying %s/%s ...", user, pass )
    -- send username and password
    -- both are prefixed with 0x06, size and are encrypted
    status, err = self.socket:send(bin.pack("C",0x06) .. bin.pack("C",string.len(user)) .. encrypt(user) ) -- send username
    status, response = self.socket:receive_bytes(0)
    if not status or string.find(response,"Enter password") == nil then
      stdnse.print_debug(1, "%s: Sending username failed", SCRIPT_NAME)
      return false, brute.Error:new( "Sending username failed." )
    end
    -- send password
    status, err = self.socket:send(bin.pack("C",0x06) .. bin.pack("C",string.len(pass)) .. encrypt(pass) ) -- send password
    status, response = self.socket:receive_bytes(0)
    if not status or string.find(response,"Login unsuccessful") or string.find(response,"Invalid login.")then
      stdnse.print_debug(1, "%s: Incorrect username or password", SCRIPT_NAME)
      return false, brute.Error:new( "Incorrect username or password." )
    end

    if status then
      retry = true -- now the server is in "locked mode", we need to retry next connection a few times
      return true, brute.Account:new( user, pass, creds.State.VALID)
    end
    return false,brute.Error:new( "Incorrect password" )
  end,

  disconnect = function( self )
    self.socket:close()
    return true
  end

}

action = function( host, port )

  local status, result
  local engine = brute.Engine:new(Driver, host, port)
  engine.options.script_name = SCRIPT_NAME
  engine.max_threads = 1 -- pcAnywhere supports only one login at a time
  status, result = engine:start()

  return result
end
