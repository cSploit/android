local http = require "http"
local shortport = require "shortport"
local stdnse = require "stdnse"
local vulns = require "vulns"

description = [[
Detects a denial of service vulnerability in the way the Apache web server
handles requests for multiple overlapping/simple ranges of a page.

References:
* http://seclists.org/fulldisclosure/2011/Aug/175
* http://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2011-3192
* http://nessus.org/plugins/index.php?view=single&id=55976
]]

---
-- @usage
-- nmap --script http-vuln-cve2011-3192.nse [--script-args http-vuln-cve2011-3192.hostname=nmap.scanme.org] -pT:80,443 <host>
--
-- @output
-- Host script results:
-- | http-vuln-cve2011-3192:
-- |   VULNERABLE:
-- |   Apache byterange filter DoS
-- |     State: VULNERABLE
-- |     IDs:  CVE:CVE-2011-3192  OSVDB:74721
-- |     Description:
-- |       The Apache web server is vulnerable to a denial of service attack when numerous
-- |       overlapping byte ranges are requested.
-- |     Disclosure date: 2011-08-19
-- |     References:
-- |       http://seclists.org/fulldisclosure/2011/Aug/175
-- |       http://nessus.org/plugins/index.php?view=single&id=55976
-- |       http://osvdb.org/74721
-- |_      http://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2011-3192
--
-- @args http-vuln-cve2011-3192.hostname  Define the host name to be used in the HEAD request sent to the server
-- @args http-vuln-cve2011-3192.path  Define the request path

-- changelog
-- 2011-08-29 Duarte Silva <duarte.silva@serializing.me>
--   - Removed the "Accept-Encoding" HTTP header
--   - Removed response header printing
--   * Changes based on Henri Doreau and David Fifield suggestions
-- 2011-08-20 Duarte Silva <duarte.silva@serializing.me>
--   * First version ;)
-- 2011-11-07 Henri Doreau
--   * Use the vulns library to report results
-----------------------------------------------------------------------

author = "Duarte Silva <duarte.silva@serializing.me>"
license = "Same as Nmap--See http://nmap.org/book/man-legal.html"
categories = {"vuln", "safe"}


portrule =  shortport.http

action = function(host, port)
  local vuln = {
    title = 'Apache byterange filter DoS',
    state = vulns.STATE.NOT_VULN, -- default
    IDS = {CVE = 'CVE-2011-3192', OSVDB = '74721'},
    description = [[
The Apache web server is vulnerable to a denial of service attack when numerous
overlapping byte ranges are requested.]],
    references = {
      'http://seclists.org/fulldisclosure/2011/Aug/175',
      'http://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2011-3192',
      'http://nessus.org/plugins/index.php?view=single&id=55976',
    },
    dates = {
      disclosure = {year = '2011', month = '08', day = '19'},
    },
  }
  local vuln_report = vulns.Report:new(SCRIPT_NAME, host, port)
  local hostname, path = stdnse.get_script_args('http-vuln-cve2011-3192.hostname',
    'http-vuln-cve2011-3192.path')

  if not path then
    path = '/'

    stdnse.print_debug(1, "Setting the request path to '/' since 'http-vuln-cve2011-3192.path' argument is missing.")
  end

  -- This first request will try to get a code 206 reply from the server by
  -- sending the innocuous header "Range: byte=0-100" in order to detect
  -- whether this functionality is available or not.
  local request_opts = {
    header = {
      Range = "bytes=0-100",
      Connection = "close"
    },
    bypass_cache = true
  }

  if hostname then
    request_opts.header.Host = hostname
  end

  local response = http.head(host, port, path, request_opts)

  if not response.status then
    stdnse.print_debug(1, "%s: Functionality check HEAD request failed for %s (with path '%s').",
      SCRIPT_NAME, hostname or host.ip, path)
  elseif response.status == 206 then
    -- The server handle range requests. Now try to request 11 ranges (one more
    -- than allowed).
    -- Vulnerable servers will reply with another code 206 response. Patched
    -- ones will return a code 200.
    request_opts.header.Range = "bytes=1-0,0-0,1-1,2-2,3-3,4-4,5-5,6-6,7-7,8-8,9-9,10-10"

    response = http.head(host, port, path, request_opts)

    if not response.status then
      stdnse.print_debug(1, "%s: Invalid response from server to the vulnerability check",
        SCRIPT_NAME)
    elseif response.status == 206 then
      vuln.state = vulns.STATE.VULN
    else
      stdnse.print_debug(1, "%s: Server isn't vulnerable (%i status code)",
        SCRIPT_NAME, response.status)
    end
  else
    stdnse.print_debug(1, "%s: Server ignores the range header (%i status code)",
      SCRIPT_NAME, response.status)
  end
  return vuln_report:make_output(vuln)
end

