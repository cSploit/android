# Try to find dpdk
#
# Once done, this will define
#
# dpdk_FOUND
# dpdk_INCLUDE_DIRS
# dpdk_LIBRARIES

if(PKG_CONFIG_FOUND)
  pkg_check_modules(dpdk QUIET libdpdk)
endif()
message(STATUS "Executing Finddpdk")
if(NOT dpdk_INCLUDE_DIRS)
  message(STATUS "Executing find_path")
  find_path(dpdk_config_INCLUDE_DIR rte_config.h
    HINTS
      ENV DPDK_DIR
    PATH_SUFFIXES
      dpdk
      include
)
  find_path(dpdk_common_INCLUDE_DIR rte_common.h
    HINTS
      ENC DPDK_DIR
    PATH_SUFFIXES
      dpdk
      include
)
  set(dpdk_INCLUDE_DIRS "${dpdk_config_INCLUDE_DIR}")
  if(NOT dpdk_config_INCLUDE_DIR EQUAL dpdk_common_INCLUDE_DIR)
    list(APPEND dpdk_INCLUDE_DIRS "${dpdk_common_INCLUDE_DIR}")
  endif()

  set(components
    bus_pci
    cmdline
    eal
    ethdev
    hash
    kvargs
    mbuf
    mempool
    mempool_ring
    mempool_stack
    pci
    pmd_af_packet
    pmd_bond
    pmd_i40e
    pmd_ixgbe
    pmd_mlx5
    pmd_ring
    pmd_vmxnet3_uio
    ring)

  set(dpdk_LIBRARIES)

  foreach(c ${components})
    find_library(DPDK_rte_${c}_LIBRARY rte_${c}
      HINTS
        ENV DPDK_DIR
        ${dpdk_LIBRARY_DIRS}
      PATH_SUFFIXES lib)
    if(DPDK_rte_${c}_LIBRARY)
      set(dpdk_lib dpdk::${c})
      if (NOT TARGET ${dpdk_lib})
        add_library(${dpdk_lib} UNKNOWN IMPORTED)
        set_target_properties(${dpdk_lib} PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${dpdk_INCLUDE_DIRS}"
          IMPORTED_LOCATION "${DPDK_rte_${c}_LIBRARY}")
        if(c STREQUAL pmd_mlx5)
          find_package(verbs QUIET)
          if(verbs_FOUND)
            target_link_libraries(${dpdk_lib} INTERFACE IBVerbs::verbs)
          endif()
        endif()
      endif()
      list(APPEND dpdk_LIBRARIES ${dpdk_lib})
    endif()
  endforeach()

  #
  # Where the heck did this list come from?  libdpdk on Ubuntu 20.04,
  # for example, doesn't even *have* -ldpdk; that's why we go with
  # pkg-config, in the hopes that it provides a correct set of flags
  # for this tangled mess.
  #
  list(APPEND dpdk_LIBRARIES dpdk rt m numo dl)
endif()

mark_as_advanced(dpdk_INCLUDE_DIRS ${dpdk_LIBRARIES})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(dpdk DEFAULT_MSG
  dpdk_INCLUDE_DIRS
  dpdk_LIBRARIES)

if(dpdk_FOUND)
  if(NOT TARGET dpdk::cflags)
     if(CMAKE_SYSTEM_PROCESSOR MATCHES "amd64|x86_64|AMD64")
      set(rte_cflags "-march=core2")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|ARM")
      set(rte_cflags "-march=armv7-a")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|AARCH64")
      set(rte_cflags "-march=armv8-a+crc")
    endif()
    add_library(dpdk::cflags INTERFACE IMPORTED)
    if (rte_cflags)
      set_target_properties(dpdk::cflags PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${rte_cflags}")
    endif()
  endif()

  if(NOT TARGET dpdk::dpdk)
    add_library(dpdk::dpdk INTERFACE IMPORTED)
    find_package(Threads QUIET)
    list(APPEND dpdk_LIBRARIES
      Threads::Threads
      dpdk::cflags)
    set_target_properties(dpdk::dpdk PROPERTIES
      INTERFACE_LINK_LIBRARIES "${dpdk_LIBRARIES}"
      INTERFACE_INCLUDE_DIRECTORIES "${dpdk_INCLUDE_DIRS}")
  endif()
endif()
