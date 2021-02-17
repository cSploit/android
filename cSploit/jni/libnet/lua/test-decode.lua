dofile"setup.lua"

do
    print"test: decode ipv4"

    local n = net.init()

    n:udp{src=1, dst=2, payload=" "}
    n:ipv4{src="1.2.3.1", dst="1.2.3.2", protocol=17, options="AAAA"}

    local b0 = n:block()

    print"= constructed:"
    print(n:dump())
    print("b0", h(b0))

    n:clear()
    print(n:dump())

    print"= decoded:"
    assert(n:decode_ipv4(b0))

    local ip1 = n:block(n:tag_below())
    local b1 = n:block()
    print(n:dump())
    print("b1", h(b1))
    print""

    assert(b0 == b1)

    local bot = assert(n:tag_below())
    local top = assert(n:tag_above())

    assert(bot == 3)
    assert(n:tag_below(bot) == nil)
    assert(n:tag_above(bot) == 2)

    assert(top == 1)
    assert(n:tag_above(top) == nil)
    assert(n:tag_below(top) == 2)

    assert(n:tag_type(bot) == "ipv4", n:tag_type(bot))
    assert(n:tag_type(top) == "udp", n:tag_type(top))

    local udpt = n:get_udp()

    udpt.payload = "\0"

    assert(n:udp(udpt))

    local b2 = n:block()
    print(n:dump())
    print("b2", h(b2))

    -- everything up to the checksum should be the same
    assert(b1:sub(1, 20+4+6) == b2:sub(1, 20+4+6))
    assert(b1:sub(20+4+7, 20+4+8) ~= b2:sub(20+4+7, 20+4+8))

    assert(n:block(n:tag_below()) == ip1)

    print"+pass"
end

print""

