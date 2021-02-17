dofile"setup.lua"

test("+udp", function()
  local n = net.init()
  local datat = {payload="xxx"}
  local udpt = {spo="xxx"}
  local ipt = {src="1.2.3.4", dst="5.6.7.8", protocol=17, options="AAA"}
  local etht = {src="01:02:03:01:02:03", dst="04:05:06:04:05:06"}
  local pkt = "0405060405060102030102030800".. -- eth
              "4600001b000000004011e77d0102030405060708"..
              h"AAA".."00".. -- ipo, padded
              "787878" -- payload

  local dtag = n:data(datat)
  local itag = n:ipv4(ipt)
  local etag = n:eth(etht)
  local b = n:block()

  dump(n)

  assert(pkt == h(b), h(b))

  n:clear()

  ipt.payload = datat.payload

  local itag = n:ipv4(ipt)
  local etag = n:eth(etht)
  local b = n:block()

  dump(n)

  print("i", pkt)
  print("o", h(b))

  assert(pkt == h(b), h(b))

  n:clear()

  local pkt = "0405060405060102030102030800".. -- eth
              "450000170000000040116ac30102030405060708"..
              "787878" -- payload
  ipt.options = nil

  local itag = n:ipv4(ipt)
  local etag = n:eth(etht)
  local b = n:block()

  dump(n)

  print("i", pkt)
  print("o", h(b))

  assert(pkt == h(b), h(b))

  n:clear()

  local pkt = "0405060405060102030102030800".. -- eth
              "450000140000000040116ac60102030405060708"
  ipt.options = nil
  ipt.payload = nil

  local itag = n:ipv4(ipt)
  local etag = n:eth(etht)
  local b = n:block()

  dump(n)

  print("i", pkt)
  print("o", h(b))

  assert(pkt == h(b), h(b))

  local pkt = "0405060405060102030102030800".. -- eth
              "46000018000000004011e7800102030405060708"..
              h"AAA".."00" -- ipo, padded

  ipt.options = "AAA"
  ipt.ptag = itag

  assert(itag == n:ipv4(ipt))
  local b = n:block()

  dump(n)

  print("i", pkt)
  print("o", h(b))

  assert(pkt == h(b), h(b))

  local pkt = "0405060405060102030102030800".. -- eth
              "4600001b000000004011e77d0102030405060708"..
              h"AAA".."00".. -- ipo, padded
              "787878" -- payload

  ipt.payload = "xxx"

  assert(itag == n:ipv4(ipt))
  local b = n:block()

  dump(n)

  print("i", pkt)
  print("o", h(b))

  assert(pkt == h(b), h(b))

  local pkt = "0405060405060102030102030800".. -- eth
              "4500001a0000000040116ac00102030405060708"..
              "787878" -- payload

  ipt.options = nil

  assert(itag == n:ipv4(ipt))
  local b = n:block()

  dump(n)

  print("i", pkt)
  print("o", h(b))

  assert(pkt == h(b), h(b))

end)

test("+ipv4, replace eth with ipv4", function()
  local n = net.init()
  local eth = n:eth{src="01:02:03:04:05:01", dst="01:02:03:04:05:02"}
  local ok,emsg=pcall(n.ipv4, n, {src="1.2.3.1", dst="1.2.3.2", protocol=2, len=20+4, options="AAAA", ptag = eth})
  assert(not ok, emsg)
  print("successfully failed", emsg)
end)


test("+ipv4 w/options mutation", function()
  local n = net.init()
  local ptag = n:ipv4{src="1.2.3.1", dst="1.2.3.2", protocol=2, len=20+4, options="AAAA"}
  n:eth{src="01:02:03:04:05:01", dst="01:02:03:04:05:02"}

  dump(n, 14+24)

  n:ipv4{src="1.2.3.1", dst="1.2.3.2", protocol=2, len=20+4, options="BB", ptag=ptag}

  dump(n, 14+24)

  n:ipv4{src="1.2.3.1", dst="1.2.3.2", protocol=2, len=20+0, ptag=ptag}

  dump(n, 14+20)

  n:ipv4{src="1.2.3.1", dst="1.2.3.2", protocol=2, len=20+8, options="DDDDD", ptag=ptag}

  dump(n, 14+28)
end)


test("-ipv4 with invalid ptag", function()

    local n = net.init()
    assert(not pcall(
        n.ipv4, n, {src="1.2.3.1", dst="1.2.3.2", protocol=2, len=20+8, options="DDDDD", ptag=999}
        )
    )
end)

