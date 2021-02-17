LOCAL_PATH := $(call my-dir)

#original path: liblua/liblua.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -fno-strict-aliasing -DLUA_USE_POSIX -DLUA_USE_DLOPEN 
# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections


LOCAL_SRC_FILES:= \
	liblua/lapi.c\
	liblua/lcode.c\
	liblua/lctype.c\
	liblua/ldebug.c\
	liblua/ldo.c\
	liblua/ldump.c\
	liblua/lfunc.c\
	liblua/lgc.c\
	liblua/llex.c\
	liblua/lmem.c\
	liblua/lobject.c\
	liblua/lopcodes.c\
	liblua/lparser.c\
	liblua/lstate.c\
	liblua/lstring.c\
	liblua/ltable.c\
	liblua/ltm.c\
	liblua/lundump.c\
	liblua/lvm.c\
	liblua/lzio.c\
	liblua/lauxlib.c\
	liblua/lbaselib.c\
	liblua/lbitlib.c\
	liblua/lcorolib.c\
	liblua/ldblib.c\
	liblua/liolib.c\
	liblua/lmathlib.c\
	liblua/loslib.c\
	liblua/lstrlib.c\
	liblua/ltablib.c\
	liblua/loadlib.c\
	liblua/linit.c
LOCAL_MODULE := liblua

include $(BUILD_STATIC_LIBRARY)


#original path: liblinear/liblinear.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -fno-strict-aliasing 
# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections


LOCAL_SRC_FILES:= \
	liblinear/linear.cpp\
	liblinear/tron.cpp\
	liblinear/blas/dnrm2.c\
	liblinear/blas/daxpy.c\
	liblinear/blas/ddot.c\
	liblinear/blas/dscal.c
LOCAL_MODULE := liblinear

include $(BUILD_STATIC_LIBRARY)


#original path: libdnet-stripped/src/libdnet.la
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H 
# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/libdnet-stripped/src\
	$(LOCAL_PATH)/libdnet-stripped/include
LOCAL_SRC_FILES:= \
	libdnet-stripped/src/addr-util.c\
	libdnet-stripped/src/addr.c\
	libdnet-stripped/src/blob.c\
	libdnet-stripped/src/ip-util.c\
	libdnet-stripped/src/ip6.c\
	libdnet-stripped/src/rand.c\
	libdnet-stripped/src/memcmp.c\
	libdnet-stripped/src/arp-none.c\
	libdnet-stripped/src/eth-linux.c\
	libdnet-stripped/src/fw-none.c\
	libdnet-stripped/src/intf.c\
	libdnet-stripped/src/ip.c\
	libdnet-stripped/src/route-linux.c\
	libdnet-stripped/src/tun-linux.c
LOCAL_MODULE := libdnet

include $(BUILD_STATIC_LIBRARY)

#original path: nbase/libnbase.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -D_FORTIFY_SOURCE=2 
# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections


LOCAL_SRC_FILES:= \
	nbase/snprintf.c\
	nbase/nbase_str.c\
	nbase/nbase_misc.c\
	nbase/nbase_memalloc.c\
	nbase/nbase_rnd.c\
	nbase/nbase_addrset.c
LOCAL_MODULE := libnbase

include $(BUILD_STATIC_LIBRARY)


#original path: nsock/src/libnsock.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DNSOCK_VERSION=\"0.02\" -D_FORTIFY_SOURCE=2
# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/nbase\
	$(LOCAL_PATH)/nsock/include\
	external/openssl/include\
	external/libpcap
LOCAL_SRC_FILES:= \
	nsock/src/error.c\
	nsock/src/filespace.c\
	nsock/src/gh_heap.c\
	nsock/src/nsock_connect.c\
	nsock/src/nsock_core.c\
	nsock/src/nsock_iod.c\
	nsock/src/nsock_read.c\
	nsock/src/nsock_timers.c\
	nsock/src/nsock_write.c\
	nsock/src/nsock_ssl.c\
	nsock/src/nsock_event.c\
	nsock/src/nsock_pool.c\
	nsock/src/netutils.c\
	nsock/src/nsock_pcap.c\
	nsock/src/nsock_engines.c\
	nsock/src/engine_select.c\
	nsock/src/engine_epoll.c\
	nsock/src/engine_kqueue.c\
	nsock/src/engine_poll.c\
	nsock/src/nsock_proxy.c\
	nsock/src/nsock_log.c\
	nsock/src/proxy_http.c\
	nsock/src/proxy_socks4.c
LOCAL_STATIC_LIBRARIES:= \
	libnbase\
	libpcap\
	libssl_static\
	libcrypto_static
LOCAL_MODULE := libnsock

include $(BUILD_STATIC_LIBRARY)


#original path: libnetutil/libnetutil.a
include $(CLEAR_VARS)

# fixed flags
LOCAL_CFLAGS+= -static -ffunction-sections -fdata-sections

LOCAL_CPPFLAGS:= -DHAVE_CONFIG_H -D_FORTIFY_SOURCE=2 -fno-strict-aliasing 

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/liblinear\
	$(LOCAL_PATH)/liblua\
	$(LOCAL_PATH)/libdnet-stripped/include\
	$(LOCAL_PATH)/nbase\
	$(LOCAL_PATH)/nsock/include\
	external/libpcre\
	external/libpcap\
	external/openssl/include
LOCAL_SRC_FILES:= \
	libnetutil/netutil.cc\
	libnetutil/PacketElement.cc\
	libnetutil/NetworkLayerElement.cc\
	libnetutil/TransportLayerElement.cc\
	libnetutil/ARPHeader.cc\
	libnetutil/EthernetHeader.cc\
	libnetutil/ICMPv4Header.cc\
	libnetutil/ICMPv6Header.cc\
	libnetutil/IPv4Header.cc\
	libnetutil/IPv6Header.cc\
	libnetutil/TCPHeader.cc\
	libnetutil/UDPHeader.cc\
	libnetutil/RawData.cc\
	libnetutil/HopByHopHeader.cc\
	libnetutil/DestOptsHeader.cc\
	libnetutil/FragmentHeader.cc\
	libnetutil/RoutingHeader.cc\
	libnetutil/PacketParser.cc
LOCAL_STATIC_LIBRARIES:= \
	libnbase\
	libdnet\
	libpcre\
	libpcap\
	libssl_static\
	libcrypto_static
LOCAL_MODULE := libnetutil

include $(BUILD_STATIC_LIBRARY)


#original path: nmap
include $(CLEAR_VARS)

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_CPPFLAGS:= -DHAVE_CONFIG_H -DNMAP_NAME=\"Nmap\" \
-DNMAP_URL=\"http://nmap.org\" \
-DNMAP_PLATFORM=\"arm-unknown-linux-androideabi\" \
-DNMAPDATADIR=\"/opt/android-ndk/platforms/android-18/arch-arm/usr/share/nmap\" \
-D_FORTIFY_SOURCE=2 -fno-strict-aliasing -frtti

LOCAL_LDFLAGS:=  -Wl,--gc-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/liblinear\
	$(LOCAL_PATH)/liblua\
	$(LOCAL_PATH)/libdnet-stripped/include\
	$(LOCAL_PATH)/nbase\
	$(LOCAL_PATH)/nsock/include\
	external/libpcre\
	external/libpcap\
	external/openssl/include
LOCAL_SRC_FILES:= \
	charpool.cc\
	FingerPrintResults.cc\
	FPEngine.cc\
	FPModel.cc\
	idle_scan.cc\
	MACLookup.cc\
	main.cc\
	nmap_ftp.cc\
	nmap_dns.cc\
	nmap_error.cc\
	nmap.cc\
	NmapOps.cc\
	NmapOutputTable.cc\
	nmap_tty.cc\
	osscan2.cc\
	osscan.cc\
	output.cc\
	payload.cc\
	portlist.cc\
	portreasons.cc\
	protocols.cc\
	scan_engine.cc\
	service_scan.cc\
	services.cc\
	TargetGroup.cc\
	Target.cc\
	targets.cc\
	tcpip.cc\
	timing.cc\
	traceroute.cc\
	utils.cc\
	xml.cc\
	nse_main.cc\
	nse_utility.cc\
	nse_nsock.cc\
	nse_dnet.cc\
	nse_fs.cc\
	nse_nmaplib.cc\
	nse_debug.cc\
	nse_pcrelib.cc\
	nse_binlib.cc\
	nse_bit.cc\
	nse_openssl.cc\
	nse_ssl_cert.cc
LOCAL_STATIC_LIBRARIES:= \
	libnbase\
	libnsock\
	libpcre\
	libpcap\
	libnetutil\
	libdnet\
	liblua\
	liblinear\
	libssl_static\
	libcrypto_static
	
LOCAL_MODULE := nmap

include $(BUILD_EXECUTABLE)
