require"pcap"
require"net"
require"tostring"

-- Quote a string into lua form (including the non-printable characters from
-- 0-31, and from 127-255).
function quote(_)
    local fmt = string.format
    local _ = fmt("%q", _)

    _ = string.gsub(_, "\\\n", "\\n")
    _ = string.gsub(_, "[%z\1-\31,\127-\255]", function (x)
        --print("x=", x)
        return fmt("\\%03d",string.byte(x))
    end)

    return _
end

q = quote

-- binary to hex
-- binary to hex
function h(s)
    if s == nil then
        return "nil"
    end
    local function hex(s)
        return string.format("%02x", string.byte(s))
    end
    return "["..#s.."] "..string.gsub(s, ".", hex)
end

-- hex to binary
function b(s)
    if not s then
        return s
    end

    local function cvt (hexpair)
        n = string.char(tonumber(hexpair, 16))
        return n
    end

    local s = s:gsub("(..)", cvt)
    return s
end

function countdiff(s0, s1)
  assert(#s0 == #s1)
  local count = 0
  for i=1,#s0 do
    if string.byte(s0, i) ~= string.byte(s1, i) then
      count = count + 1
    end
  end
  return count
end

function assertmostlyeql(threshold, s0, s1)
  if #s0 ~= #s1 then
      print("s0", h(s0))
      print("s1", h(s1))
  end
  assert(#s0 == #s1)

  local diff = countdiff(s0, s1)
  if diff > threshold then
      print("s0", h(s0))
      print("s1", h(s1))
  end
  assert(diff <= threshold, diff.." is less than threshold "..threshold)
end

function pcaprecode(incap, outcap)
    if not outcap then
        outcap = "recoded-"..incap
    end
    os.remove(outcap)

    local cap = assert(pcap.open_offline(incap))
    local dmp = assert(cap:dump_open(outcap))
    local n = assert(net.init())
    local i = 0
    for pkt, time, len in cap.next, cap do
        i = i + 1
        print("packet", i, "wirelen", len, "timestamp", time, os.date("!%c", time))
        assert(n:clear())
        assert(n:decode_eth(pkt))
        assert(dmp:dump(n:block(), time, len))
        --print(n:dump())
    end
    dmp:close()
    cap:close()
    n:destroy()
    return outcap
end

function assertpcapsimilar(threshold, file0, file1)
    local n0 = assert(net.init())
    local n1 = assert(net.init())
    local cap0 = assert(pcap.open_offline(file0))
    local cap1 = assert(pcap.open_offline(file1))
    local i = 0
    for pkt0, time0, len0 in cap0.next, cap0 do
        local pkt1, time1, len1 = assert(cap1:next())
        i = i + 1

        print("        "..file0, i, "wirelen", len0, "timestamp", time0, os.date("!%c", time0))
        print(file1, i, "wirelen", len1, "timestamp", time1, os.date("!%c", time1))

        assert(len0 == len1)
        assert(time0 == time1, string.format("%.7f ~= %.7f", time0, time1))

        local _threshold = threshold
        if type(threshold) == "table" then
            if threshold[file0] then
                if type(threshold[file0]) == "table" then
                    if threshold[file0][i] then
                        _threshold = threshold[file0][i]
                    end
                else
                    _threshold = threshold[file0]
                end
            else
                _threshold = 0
            end
        end

        assertmostlyeql(_threshold, pkt0, pkt1)
    end
    assert(cap1:next() == nil)
    n0:destroy()
    n1:destroy()
end

