local bin = require "bin"
local http = require "http"
local shortport = require "shortport"
local stdnse = require "stdnse"
local base64 = require "base64"
local smbauth = require "smbauth"
local string = require "string"
local table = require "table"


description = [[
This script enumerates information from remote HTTP services with NTLM
authentication enabled.

By sending a HTTP NTLM authentication request with null domain and user
credentials (passed in the 'Authorization' header), the remote service will
respond with a NTLMSSP message (encoded within the 'WWW-Authenticate' header)
and disclose information to include NetBIOS, DNS, and OS build version if
available.
]]


---
-- @usage
-- nmap -p 80 --script http-ntlm-info --script-args http-ntlm-info.root=/root/ <target>
--
-- @args http-ntlm-info.root The URI path to request
--
-- @output
-- 80/tcp   open     http
-- | http-ntlm-info:
-- |   Target_Name: ACTIVEWEB
-- |   NetBIOS_Domain_Name: ACTIVEWEB
-- |   NetBIOS_Computer_Name: WEB-TEST2
-- |   DNS_Domain_Name: somedomain.com
-- |   DNS_Computer_Name: web-test2.somedomain.com
-- |   DNS_Tree_Name: somedomain.com
-- |_  OS_Version: 6.1 (Build 7601)
--
--@xmloutput
-- <elem key="Target_Name">TELME</elem>
-- <elem key="NetBIOS_Domain_Name">TELME</elem>
-- <elem key="NetBIOS_Computer_Name">GT4</elem>
-- <elem key="DNS_Domain_Name">telme.somedomain.com</elem>
-- <elem key="DNS_Computer_Name">gt4.telme.somedomain.com</elem>
-- <elem key="Product_Version">5.0 (Build 2195)</elem>


author = "Justin Cacak"
license = "Same as Nmap--See http://nmap.org/book/man-legal.html"
categories = {"default", "discovery", "safe"}


portrule = shortport.http

-- TlRMTVNTUAABAAAAB4IIogAAAAAAAAAAAAAAAAAAAAAGAbEdAAAADw==
-- Ref: http://davenport.sourceforge.net/ntlm.html#theType1Message
local auth_blob = base64.enc(
  "NTLMSSP\x00" ..
  "\x01\x00\x00\x00" .. -- NTLM Type 1 message
  bin.pack("<I", --flags
    0x00000001 + -- Negotiate Unicode
    0x00000002 + -- Negotiate OEM strings
    0x00000004 + -- Request Target
    0x00000200 + -- Negotiate NTLM
    0x00008000 + -- Negotiate Always Sign
    0x00080000 + -- Negotiate NTLM2 Key
    0x02000000 + -- Unknown
    0x20000000 + -- Negotiate 128
    0x80000000 -- Negotiate 56
    ) ..
  string.rep("\x00", 16) .. -- Supplied Domain and Workstation (empty)
  bin.pack("CC<S", -- OS version info
    6, 1, 7601) .. -- 6.1.7601, Win 7 SP1 or Server 2008 R2 SP1
  "\x00\x00\x00\x0f" -- OS version info end (static 0x0000000f)
  )

action = function(host, port)

  local output = stdnse.output_table()
  local root = stdnse.get_script_args(SCRIPT_NAME .. ".root") or "/"

  -- Inject NTLM authorization header with null domain and user credentials
  local opts = { header = { Authorization = "NTLM " .. auth_blob } }

  local response = http.get( host, port, root, opts )

  -- Continue only if correct header (www-authenticate) and NTLM response are included
  if response.header["www-authenticate"] and string.match(response.header["www-authenticate"], "NTLM (.*)") then

    -- Extract NTLMSSP response and base64 decode
    local data = base64.dec(string.match(response.header["www-authenticate"], "NTLM (.*)"))

    -- Leverage smbauth.get_host_info_from_security_blob() for decoding
    local ntlm_decoded = smbauth.get_host_info_from_security_blob(data)

    -- Target Name will always be returned under any implementation
    output.Target_Name = ntlm_decoded.target_realm

    -- Only display information returned (varies especially with open source implementations)
    -- Additionally ignore responses with null values (very rare)
    if ntlm_decoded.netbios_domain_name and #ntlm_decoded.netbios_domain_name > 0 then
      output.NetBIOS_Domain_Name = ntlm_decoded.netbios_domain_name
    end

    if ntlm_decoded.netbios_computer_name and #ntlm_decoded.netbios_computer_name > 0 then
      output.NetBIOS_Computer_Name = ntlm_decoded.netbios_computer_name
    end

    if ntlm_decoded.dns_domain_name and #ntlm_decoded.dns_domain_name > 0 then
      output.DNS_Domain_Name = ntlm_decoded.dns_domain_name
    end

    if ntlm_decoded.fqdn and #ntlm_decoded.fqdn > 0 then
      output.DNS_Computer_Name = ntlm_decoded.fqdn
    end

    if ntlm_decoded.dns_forest_name and #ntlm_decoded.dns_forest_name > 0 then
      output.DNS_Tree_Name = ntlm_decoded.dns_forest_name
    end

    -- Query product build version if available (typically OS version under Windows)
    -- Use this method as certain open source HTTP NTLM implementations do not set correct flags
    -- Compute offset for Target Name
    local target_offset = data:sub(17, 21)
    local pos, target_offset_dec = bin.unpack("<I", target_offset)

    if #data > 48 and target_offset_dec ~= 48 then
      -- Get product major version
      local major_version = data:sub(49, 50)
      local pos, major_version_dec = bin.unpack("C", major_version)

      -- Get product minor version
      local minor_version = data:sub(50, 51)
      local pos, minor_version_dec = bin.unpack("C", minor_version)

      -- Get product build version
      local build = data:sub(51, 53)
      local pos, build_dec = bin.unpack("<S", build)

      output.Product_Version = major_version_dec .. "." .. minor_version_dec .. " (Build " .. build_dec .. ")"
    end

    return output

  end

end
