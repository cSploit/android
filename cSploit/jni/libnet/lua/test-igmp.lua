dofile"setup.lua"

test("+igmp", function()
  local n = net.init()
  local igmpt = {type=0x11, code=0, ip="1.2.3.4"}
  local ipt = {src="1.2.3.4", dst="5.6.7.8", protocol=2}
  local etht = {src="01:02:03:01:02:03", dst="04:05:06:04:05:06"}
  local pkt = "0405060405060102030102030800".. -- eth
              "4500001c0000000040026acd0102030405060708".. -- ip
              "1100eaf901020304" -- igmp

  print"  w/no-payload"

  local gtag = n:igmp(igmpt)
  local itag = n:ipv4(ipt)
  local etag = n:eth(etht)
  local b = n:block()

  dump(n)

  print("want", pkt)
  print("have", h(b))

  assert(pkt == h(b), h(b))

  print"ok"

  print"  w/payload"

  igmpt.payload = "\0\0\0"
  igmpt.ptag = gtag

  local gtag = n:igmp(igmpt)
  local b = n:block()

  dump(n)

  print("want", pkt)
  print("have", h(b))

  assert(pkt.."000000" == h(b), h(b))

  print"ok"

  n:destroy()
end)

