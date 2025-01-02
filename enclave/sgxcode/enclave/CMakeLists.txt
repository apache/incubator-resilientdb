# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

# Use the edger8r to generate C bindings from the EDL file.
add_custom_command(
  OUTPUT sgx_cpp_t.h sgx_cpp_t.c sgx_cpp_args.h
  DEPENDS ${CMAKE_SOURCE_DIR}/enclave/sgx_cpp.edl
  COMMAND
    openenclave::oeedger8r --trusted ${CMAKE_SOURCE_DIR}/enclave/sgx_cpp.edl
    --search-path ${OE_INCLUDEDIR} --search-path
    ${OE_INCLUDEDIR}/openenclave/edl/sgx)

set(COUNTER_SRC simulated_counter_src)
set(RANDOM_SRC random_src)
set(DECRYPTOR_SRC decryptor_src)

add_executable(
  enclave common/ecalls.cpp common/dispatcher.cpp 
          ${COUNTER_SRC}/simulated_counter.cpp
          ${RANDOM_SRC}/random.cpp
          ${DECRYPTOR_SRC}/decryptor.cpp
          ${CMAKE_CURRENT_BINARY_DIR}/sgx_cpp_t.c)
if (WIN32)
  maybe_build_using_clangw(enclave)
endif ()

target_compile_definitions(enclave PUBLIC OE_API_VERSION=2)

target_include_directories(
  enclave
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} # Needed for #include "../shared.h"
          ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR})

if (OE_CRYPTO_LIB STREQUAL "openssl_3")
  message(STATUS "openssl_3 enable.")
else()
  message(STATUS "openssl_3 not enable. ${OE_CRYPTO_LIB}")
endif ()

target_link_libraries(
  enclave openenclave::oeenclave openenclave::oecryptoopenssl
  openenclave::oelibcxx)
