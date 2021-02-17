dofile"setup.lua"

test("+eth", function()
  local n = net.init()
  local datat={payload="xxx"}
  local etht = {src="01:02:03:01:02:03", dst="04:05:06:04:05:06"}
  local pkt = "0405060405060102030102030800787878"

  local dtag = n:data(datat)
  local etag = n:eth(etht)
  local b = n:block()

  dump(n)

  assert(pkt == h(b), h(b))

  etht.ptag = etag
  datat.ptag = dtag

  local dtag = n:data(datat)
  local etag = n:eth(etht)
  local b = n:block()

  dump(n)

  assert(pkt == h(b), h(b))

  n:clear()
  etht.ptag = nil
  datat.ptag = nil

  etht.payload = datat.payload

  local etag = n:eth(etht)
  local b = n:block()

  dump(n)

  assert(pkt == h(b), h(b))

  etht.ptag = etag

  local _ = n:eth(etht)
  local b = n:block()

  dump(n)

  assert(_ == etag, _)
  assert(pkt == h(b), h(b))

  n:clear()

end)

