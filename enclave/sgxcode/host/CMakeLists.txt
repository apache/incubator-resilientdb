# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

add_custom_command(
  OUTPUT sgx_cpp_u.h sgx_cpp_u.c sgx_cpp_args.h
  DEPENDS ${CMAKE_SOURCE_DIR}/enclave/sgx_cpp.edl
  COMMAND
    openenclave::oeedger8r --untrusted ${CMAKE_SOURCE_DIR}/enclave/sgx_cpp.edl
    --search-path ${OE_INCLUDEDIR} --search-path
    ${OE_INCLUDEDIR}/openenclave/edl/sgx)

add_executable(sgxhost
               host.cpp ${CMAKE_CURRENT_BINARY_DIR}/sgx_cpp_u.c)

if (WIN32)
  copy_oedebugrt_target(sgxhost_oedebugrt)
  add_dependencies(sgxhost sgxhost_oedebugrt)
endif ()

target_include_directories(
  sgxhost
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} # Needed for #include "../shared.h"
          ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(sgxhost openenclave::oehost)
