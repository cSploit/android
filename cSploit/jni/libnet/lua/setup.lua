print"============================================"

local keepgoing = nil

for _,v in ipairs(arg) do
  if v == "-k" then
    keepgoing = true
  end
end

require"net"
require"pcap"

DEV="en0"

-- Quote a string into lua form (including the non-printable characters from
-- 0-31, and from 127-255).
function q(_)
  local fmt = string.format
  local _ = fmt("%q", _)

  _ = string.gsub(_, "\\\n", "\\n")
  _ = string.gsub(_, "[%z\1-\31,\127-\255]", function (x)
    --print("x=", x)
    return fmt("\\%03d",string.byte(x))
  end)

  return _
end

function h(_)
  local fmt = string.format
  _ = string.gsub(_, ".", function (x)
    return fmt("%02x",string.byte(x))
  end)

  return _
end

function dump(n, size)
  local b = n:block()
  print(">")
  print(n:dump())
  print("size="..#b)
  --print("q=[["..q(b).."]]")
  print("h=[["..h(b).."]]")

  if _dumper then
      assert(_dumper:dump(b))
      assert(_dumper:flush())
  end

  if size then
    assert(#b == size, "block's size is not expected, "..size)
  end
end

function pcap_dumper(fname)
    local cap = assert(pcap.open_dead(pcap.DLT.EN10MB))
    local dmp = assert(cap:dump_open(fname))
    assert(dmp:flush())
    cap:close()
    return dmp
end

function test(n, f)
    local strip = {
        ["+"]="-";
        [" "]="-";
        ["/"]="-";
        [","]="";
    }
    local pcap_name = "out"..string.gsub(n, ".", strip)..".pcap"
    _dumper = pcap_dumper(pcap_name)

    print""
    print""
    print("=test: "..n.." ("..pcap_name..")")

    if not keepgoing then
        f()
        print("+pass: "..n)
    else
        local ok, emsg = pcall(f)
        if not ok then
            print("! FAIL: "..n)
        else
            print("+pass: "..n)
        end
    end
    _dumper = _dumper:close()
end

function hex_dump(s)
    print(h(s))
end

