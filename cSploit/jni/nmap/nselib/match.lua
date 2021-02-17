---
-- Buffered network I/O helper functions.
--
-- The functions in this module can be used for delimiting data received by the
-- <code>nmap.receive_buf</code> function in the Network I/O API (which see).
-- @copyright Same as Nmap--See http://nmap.org/book/man-legal.html

local pcre = require "pcre"
local stdnse = require "stdnse"
_ENV = stdnse.module("match", stdnse.seeall)

--various functions for use with NSE's nsock:receive_buf - function

-- e.g.
-- sock:receive_buf(regex("myregexpattern"), true) - does a match using pcre
--                                                   regular expressions
-- sock:receive_buf(numbytes(80), true) - is the buffered version of
--                                        sock:receive_bytes(80) - i.e. it
--                                        returns exactly 80 bytes and no more

--- Return a function that allows delimiting with a regular expression.
--
-- This function is a wrapper around <code>pcre.exec</code>. Its purpose is to
-- give script developers the ability to use regular expressions for delimiting
-- instead of Lua's string patterns.
-- @param pattern The regex.
-- @usage sock:receive_buf(match.regex("myregexpattern"), true)
-- @see nmap.receive_buf
-- @see pcre.exec
regex = function(pattern)
  local r = pcre.new(pattern, 0,"C")

  return function(buf)
    local s,e = r:exec(buf, 0,0);
    return s,e
  end
end

--- Return a function that allows delimiting at a certain number of bytes.
--
-- This function can be used to get a buffered version of
-- <code>sock:receive_bytes(n)</code> in case a script requires more than one
-- fixed-size chunk, as the unbuffered version may return more bytes than
-- requested and thus would require you to do the parsing on your own.
--
-- The <code>keeppattern</code> parameter to receive_buf should be set to
-- <code>true</code>, otherwise the string returned will be 1 less than
-- <code>num</code>
-- @param num Number of bytes.
-- @usage sock:receive_buf(match.numbytes(80), true)
-- @see nmap.receive_buf
numbytes = function(num)
  local n = num
  return function(buf)
    if(#buf >=n) then
      return n, n
    end
    return nil
  end
end


return _ENV;
