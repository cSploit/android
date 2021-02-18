# Auto-generated - DO NOT EDIT!
# To regenerate, edit openssl.config, then run:
#     ./import_openssl.sh import /path/to/openssl-1.0.1e.tar.gz
#
# Before including this file, the local Android.mk must define the following
# variables:
#
#    local_c_flags
#    local_c_includes
#    local_additional_dependencies
#
# This script will define the following variables:
#
#    target_c_flags
#    target_c_includes
#    target_src_files
#
#    host_c_flags
#    host_c_includes
#    host_src_files
#

# Ensure these are empty.
unknown_arch_c_flags :=
unknown_arch_src_files :=
unknown_arch_exclude_files :=


common_c_flags :=

common_src_files := \
  ssl/bio_ssl.c \
  ssl/d1_both.c \
  ssl/d1_enc.c \
  ssl/d1_lib.c \
  ssl/d1_pkt.c \
  ssl/d1_srtp.c \
  ssl/kssl.c \
  ssl/s23_clnt.c \
  ssl/s23_lib.c \
  ssl/s23_meth.c \
  ssl/s23_pkt.c \
  ssl/s23_srvr.c \
  ssl/s2_clnt.c \
  ssl/s2_enc.c \
  ssl/s2_lib.c \
  ssl/s2_meth.c \
  ssl/s2_pkt.c \
  ssl/s2_srvr.c \
  ssl/s3_both.c \
  ssl/s3_cbc.c \
  ssl/s3_clnt.c \
  ssl/s3_enc.c \
  ssl/s3_lib.c \
  ssl/s3_meth.c \
  ssl/s3_pkt.c \
  ssl/s3_srvr.c \
  ssl/ssl_algs.c \
  ssl/ssl_asn1.c \
  ssl/ssl_cert.c \
  ssl/ssl_ciph.c \
  ssl/ssl_err.c \
  ssl/ssl_err2.c \
  ssl/ssl_lib.c \
  ssl/ssl_rsa.c \
  ssl/ssl_sess.c \
  ssl/ssl_stat.c \
  ssl/ssl_txt.c \
  ssl/t1_clnt.c \
  ssl/t1_enc.c \
  ssl/t1_lib.c \
  ssl/t1_meth.c \
  ssl/t1_reneg.c \
  ssl/t1_srvr.c \
  ssl/tls_srp.c \

common_c_includes := \
  . \
  crypto \
  include \

arm_c_flags :=

arm_src_files :=

arm_exclude_files :=

x86_c_flags :=

x86_src_files :=

x86_exclude_files :=

x86_64_c_flags :=

x86_64_src_files :=

x86_64_exclude_files :=

mips_c_flags :=

mips_src_files :=

mips_exclude_files :=

target_arch := $(TARGET_ARCH)
ifeq ($(target_arch)-$(TARGET_HAS_BIGENDIAN),mips-true)
target_arch := unknown_arch
endif

target_c_flags    := $(common_c_flags) $($(target_arch)_c_flags) $(local_c_flags)
target_c_includes := $(addprefix external/openssl/,$(common_c_includes)) $(local_c_includes)
target_src_files  := $(common_src_files) $($(target_arch)_src_files)
target_src_files  := $(filter-out $($(target_arch)_exclude_files), $(target_src_files))

ifeq ($(HOST_OS)-$(HOST_ARCH),linux-x86)
host_arch := x86
else
host_arch := unknown_arch
endif

host_c_flags    := $(common_c_flags) $($(host_arch)_c_flags) $(local_c_flags)
host_c_includes := $(addprefix external/openssl/,$(common_c_includes)) $(local_c_includes)
host_src_files  := $(common_src_files) $($(host_arch)_src_files)
host_src_files  := $(filter-out $($(host_arch)_exclude_files), $(host_src_files))

local_additional_dependencies += $(LOCAL_PATH)/Ssl-config.mk

