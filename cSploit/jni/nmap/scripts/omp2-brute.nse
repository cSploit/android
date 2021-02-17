local brute = require "brute"
local creds = require "creds"
local omp2 = require "omp2"
local shortport = require "shortport"

description = [[
Performs brute force password auditing against the OpenVAS manager using OMPv2.
]]

---
-- @usage
-- nmap -p 9390 --script omp2-brute <target>
--
-- @output
-- PORT     STATE SERVICE REASON
-- 9390/tcp open  openvas syn-ack
-- | svn-brute:
-- |   Accounts
-- |_    admin:secret => Valid credentials
--

author = "Henri Doreau"
license = "Same as Nmap--See http://nmap.org/book/man-legal.html"
categories = {"brute", "intrusive"}


portrule = shortport.port_or_service(9390, "openvas")


Driver = {
  new = function(self, host, port)
    local o = {}
    setmetatable(o, self)
    self.__index = self
    o.host = host
    o.port = port
    o.session = omp2.Session:new()
    return o
  end,

  --- Connects to the OpenVAS Manager
  --
  -- @return status boolean for connection success/failure
  -- @return err string describing the error on failure
  connect = function(self)
    return self.session:connect(self.host, self.port)
  end,

  --- Closes connection
  --
  -- @return status boolean for closing success/failure
  disconnect = function(self)
    return self.session:close()
  end,

  --- Attempts to login the the OpenVAS Manager using a given username/password
  -- couple. Store the credentials in the registry on success.
  --
  -- @param username string containing the login username
  -- @param password string containing the login password
  -- @return status boolean for login success/failure
  -- @return err string describing the error on failure
  login = function(self, username, password)
    if self.session:authenticate(username, password) then
      -- store the account for possible future use
      omp2.add_account(self.host, username, password)
      return true, brute.Account:new(username, password, creds.State.VALID)
    else
      return false, brute.Error:new("login failed")
    end
  end,

}

action = function(host, port)
  local engine = brute.Engine:new(Driver, host, port)
  engine.options.script_name = SCRIPT_NAME
  local status, result = engine:start()
  return result
end

