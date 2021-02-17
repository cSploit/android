dofile"setup.lua"

n = net.init()

print(q(net.pton("01:02:03:04:05:06")))
print(q(net.pton("1.2.3.4")))
print(q(net.pton("::1")))

n:udp{src=3, dst=5, payload="XXX"}
ipv4 = n:ipv4{src="1.2.3.1", dst="1.2.3.2", protocol=17, len=20+8+3}
n:eth{src="01:02:03:04:05:01", dst="01:02:03:04:05:02"}

print(n:dump())

b1 = n:block()

print(#b1, q(b1))

n:ipv4{src="1.2.3.1", dst="1.2.3.2", protocol=17, len=20+8+3, ptag=ipv4}

--print(q({src="1.2.3.1", dst="1.2.3.2", protocol=17, len=20+8+3, ptag=ipv4}))

print(n:dump())

b2 = n:block()

print("b1=", q(b1))
print("b2=", q(b2))

assert(b1 == b2)


