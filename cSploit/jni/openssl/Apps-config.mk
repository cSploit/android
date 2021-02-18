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


common_c_flags := \
  -DMONOLITH \

common_src_files := \
  apps/app_rand.c \
  apps/apps.c \
  apps/asn1pars.c \
  apps/ca.c \
  apps/ciphers.c \
  apps/crl.c \
  apps/crl2p7.c \
  apps/dgst.c \
  apps/dh.c \
  apps/dhparam.c \
  apps/dsa.c \
  apps/dsaparam.c \
  apps/ec.c \
  apps/ecparam.c \
  apps/enc.c \
  apps/engine.c \
  apps/errstr.c \
  apps/gendh.c \
  apps/gendsa.c \
  apps/genpkey.c \
  apps/genrsa.c \
  apps/nseq.c \
  apps/ocsp.c \
  apps/openssl.c \
  apps/passwd.c \
  apps/pkcs12.c \
  apps/pkcs7.c \
  apps/pkcs8.c \
  apps/pkey.c \
  apps/pkeyparam.c \
  apps/pkeyutl.c \
  apps/prime.c \
  apps/rand.c \
  apps/req.c \
  apps/rsa.c \
  apps/rsautl.c \
  apps/s_cb.c \
  apps/s_client.c \
  apps/s_server.c \
  apps/s_socket.c \
  apps/s_time.c \
  apps/sess_id.c \
  apps/smime.c \
  apps/speed.c \
  apps/spkac.c \
  apps/srp.c \
  apps/verify.c \
  apps/version.c \
  apps/x509.c \

common_c_includes := \
  . \
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

local_additional_dependencies += $(LOCAL_PATH)/Apps-config.mk

