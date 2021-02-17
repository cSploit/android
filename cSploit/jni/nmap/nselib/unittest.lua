---
-- Unit testing support for NSE libraries.
--
-- This library will import all NSE libraries looking for a global variable
-- <code>test_suite</code>. This must be a callable that returns true or false
-- and the number of tests that failed. For convenience, the
-- <code>unittest.TestSuite</code> class has this property, and tests can be
-- added with <code>add_test</code>. Example:
--
-- <code>
-- local data = {"foo", "bar", "baz"}
-- test_suite = unittest.TestSuite:new()
-- test_suite:add_test(equal(data[2], "bar"), "data[2] should equal 'bar'")
-- </code>
--
-- The library is driven by the unittest NSE script.
--
-- @args unittest.run Run tests. Causes <code>unittest.testing()</code> to
--                    return true.
--
-- @copyright Same as Nmap--See http://nmap.org/book/man-legal.html

local stdnse = require "stdnse"
local string = require "string"
local table = require "table"
local nmap = require "nmap"
local nsedebug = require "nsedebug"
local listop = require "listop"
_ENV = stdnse.module("unittest", stdnse.seeall)

local libs = {
"afp",
"ajp",
"amqp",
"asn1",
"base32",
"base64",
"bin",
"bitcoin",
"bit",
"bittorrent",
"bjnp",
"brute",
"cassandra",
"citrixxml",
"comm",
"creds",
"cvs",
"datafiles",
"dhcp6",
"dhcp",
"dnsbl",
"dns",
"dnssd",
"drda",
"eap",
"eigrp",
"formulas",
"ftp",
"giop",
"gps",
"http",
"httpspider",
"iax2",
"ike",
"imap",
"informix",
"ipOps",
"ipp",
"iscsi",
"isns",
"jdwp",
"json",
"ldap",
"lfs",
"listop",
"match",
"membase",
"mobileme",
"mongodb",
"msrpc",
"msrpcperformance",
"msrpctypes",
"mssql",
"mysql",
"natpmp",
"ncp",
"ndmp",
"netbios",
"nmap",
"nrpc",
"nsedebug",
"omp2",
"openssl",
"ospf",
"packet",
"pcre",
"pgsql",
"pop3",
"pppoe",
"proxy",
"rdp",
"redis",
"rmi",
"rpcap",
"rpc",
"rsync",
"rtsp",
"sasl",
"shortport",
"sip",
"smbauth",
"smb",
"smtp",
"snmp",
"socks",
"srvloc",
"ssh1",
"ssh2",
"sslcert",
"stdnse",
"strbuf",
"stun",
"tab",
"target",
"tftp",
"tns",
"unicode",
"unittest",
"unpwdb",
"upnp",
"url",
"versant",
"vnc",
"vulns",
"vuzedht",
"wsdd",
"xdmcp",
"xmpp",
}

local am_testing = stdnse.get_script_args('unittest.run')
---Check whether tests are being run
--
-- Libraries can use this function to avoid the overhead of creating tests if
-- the user hasn't chosen to run them.
-- @return true if unittests are being run, false otherwise.
function testing()
  return am_testing
end

---
-- Run tests provided by NSE libraries
-- @param to_test A list (table) of libraries to test. If none is provided, all
--                libraries are tested.
run_tests = function(to_test)
  am_testing = true
  if to_test == nil then
    to_test = libs
  end
  local fails = stdnse.output_table()
  for _,lib in ipairs(to_test) do
    stdnse.print_debug(1, "Testing %s", lib)
    local thelib = require(lib)
    local failed = 0
    if rawget(thelib,"test_suite") ~= nil then
      failed = thelib.test_suite()
    end
    if failed ~= 0 then
      fails[lib] = failed
    end
  end
  return fails
end

--- The TestSuite class
--
-- Holds and runs tests.
TestSuite = {

  --- Creates a new TestSuite object
  --
  -- @name TestSuite.new
  -- @return TestSuite object
  new = function(self)
    local o = {}
    setmetatable(o, self)
    self.__index = self
    o.tests = {}
    return o
  end,

  --- Set up test environment. Override this.
  -- @name TestSuite.setup
  setup = function(self)
    return true
  end,
  --- Tear down test environment. Override this.
  -- @name TestSuite.teardown
  teardown = function(self)
    return true
  end,
  --- Add a test.
  -- @name TestSuite.add_test
  -- @param test Function that will be called with the TestSuite object as its only parameter.
  -- @param description A description of the test being run
  add_test = function(self, test, description)
    self.tests[#self.tests+1] = {test, description}
  end,

  --- Run tests.
  -- Runs all tests in the TestSuite, and returns the number of failures.
  -- @name TestSuite.__call
  -- @return failures The number of tests that failed
  -- @return tests The number of tests run
  __call = function(self)
    local failures = 0
    local passes = 0
    self:setup()
    for _,test in ipairs(self.tests) do
      stdnse.print_debug(2, "| Test: %s...", test[2])
      local status, note = test[1](self)
      local result
      local lvl = 2
      if status then
        result = "Pass"
        passes = passes + 1
      else
        result = "Fail"
        lvl = 1
        if nmap.debugging() < 2 then
          stdnse.print_debug(1, "| Test: %s...", test[2])
        end
        failures = failures + 1
      end
      if note then
        stdnse.print_debug(lvl, "| \\_result: %s (%s)", result, note)
      else
        stdnse.print_debug(lvl, "| \\_result: %s", result)
      end
    end
    stdnse.print_debug(1, "|_%d of %d tests passed", passes, #self.tests)
    self:teardown()
    return failures, #self.tests
  end,
}

--- Test creation helper function.
--  Turns a simple function into a test factory.
--  @param test A function that returns true or false depending on test
--  @param fmt A format string describing the failure condition using the
--             arguments to the test function
--  @return function that generates tests suitable for use in add_test
make_test = function(test, fmt)
  return function(...)
    local args={...}
    local nargs = select("#", ...)
    return function(suite)
      if not test(table.unpack(args,1,nargs)) then
        return false, string.format(fmt, table.unpack(listop.map(nsedebug.tostr, args),1,nargs))
      end
      return true
    end
  end
end

--- Test for nil
-- @param value The value to test
-- @return bool True if the value is nil, false otherwise.
is_nil = function(value)
  return value == nil
end
is_nil = make_test(is_nil, "Expected not nil, got %s")

--- Test for not nil
-- @param value The value to test
-- @return bool True if the value is not nil, false otherwise.
not_nil = function(value)
  return value ~= nil
end
not_nil = make_test(not_nil, "Expected not nil, got %s")

--- Test tables for equality, 1 level deep
-- @param a The first table to test
-- @param b The second table to test
-- @return bool True if #a == #b and a[i] == b[i] for every i<#a, false otherwise.
table_equal = function(a, b)
  return function (suite)
    if #a ~= #b then
      return false, "Length not equal"
    end
    for i, v in ipairs(a) do
      if b[i] ~= v then
        return false, string.format("%s ~= %s at position %d", v, b[i], i)
      end
    end
    return true
  end
end

--- Test for equality
-- @param a The first value to test
-- @param b The second value to test
-- @return bool True if a == b, false otherwise.
equal = function(a, b)
  return a == b
end
equal = make_test(equal, "%s not equal to %s")

--- Test for inequality
-- @param a The first value to test
-- @param b The second value to test
-- @return bool True if a != b, false otherwise.
not_equal = function(a, b)
  return a ~= b
end
not_equal = make_test(not_equal, "%s unexpectedly equal to %s")

--- Test for truth
-- @param value The value to test
-- @return bool True if value is a boolean and true
is_true = function(value)
  return value == true
end
is_true = make_test(is_true, "Expected true, got %s")

--- Test for falsehood
-- @param value The value to test
-- @return bool True if value is a boolean and false
is_false = function(value)
  return value == false
end
is_false = make_test(is_false, "Expected false, got %s")

--- Test less than
-- @param a The first value to test
-- @param b The second value to test
-- @return bool True if a < b, false otherwise.
lt = function(a, b)
  return a < b
end
lt = make_test(lt, "%s not less than %s")

--- Test less than or equal to
-- @param a The first value to test
-- @param b The second value to test
-- @return bool True if a <= b, false otherwise.
lte = function(a, b)
  return a <= b
end
lte = make_test(lte, "%s not less than %s")

--- Test length
-- @param t The table to test
-- @param l The length to test
-- @return bool True if the length of t is l
length_is = function(t, l)
  return #t == l
end
length_is = make_test(length_is, "Length of %s is not %s")

--- Expected failure test
-- @param test The test to run
-- @return function A test for expected failure of the test
expected_failure = function(test)
  return function(suite)
    if test(suite) then
      return true, "Test unexpectedly passed"
    else
      return true, "Test failed as expected"
    end
    return true
  end
end


if not testing() then
  return _ENV
end

-- Self test
test_suite = TestSuite:new()

test_suite:add_test(is_nil(test_suite["asdfdoesnotexist"]), "Nonexistent key does not exist")
test_suite:add_test(equal(1+1336, 7 * 191), "Arithmetically equal expressions are equal")
test_suite:add_test(not_equal( true, "true" ), "Boolean true not equal to string \"true\"")
test_suite:add_test(is_true("test" == "test"), "Boolean expression evaluates to true")
test_suite:add_test(is_false(1.9999 == 2.0), "Boolean expression evaluates to false")
test_suite:add_test(lt(1, 999), "1 < 999")
test_suite:add_test(lte(8, 8), "8 <= 8")
test_suite:add_test(expected_failure(not_nil(nil)), "Test expected to fail fails")
test_suite:add_test(expected_failure(is_nil(nil)), "Test expected to fail succeeds")
test_suite:add_test(length_is(test_suite.tests, 10), "Number of tests is 10")

return _ENV;
