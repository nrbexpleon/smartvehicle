cmake_minimum_required(VERSION 3.20)
project(SmartVehicle VERSION 0.2.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(SMARTVEHICLE_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)
option(SMARTVEHICLE_ENABLE_OPENSSL "Build the OpenSSL production crypto adapter" ON)

set(SMARTVEHICLE_SOURCES
  src/platform.cpp
  src/hal/socketcan.cpp
  src/ota/ab_slot.cpp)
if(SMARTVEHICLE_ENABLE_OPENSSL)
  find_package(OpenSSL 3.0 REQUIRED COMPONENTS Crypto)
  list(APPEND SMARTVEHICLE_SOURCES src/security/openssl_crypto.cpp)
endif()

add_library(smartvehicle ${SMARTVEHICLE_SOURCES})
target_include_directories(smartvehicle PUBLIC include)
target_compile_definitions(smartvehicle PUBLIC
  $<$<BOOL:${SMARTVEHICLE_ENABLE_OPENSSL}>:SMARTVEHICLE_HAS_OPENSSL=1>)
if(SMARTVEHICLE_ENABLE_OPENSSL)
  target_link_libraries(smartvehicle PUBLIC OpenSSL::Crypto)
endif()
target_compile_options(smartvehicle PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
  $<$<AND:$<BOOL:${SMARTVEHICLE_WARNINGS_AS_ERRORS}>,$<CXX_COMPILER_ID:GNU,Clang>>:-Werror>)

add_executable(vehicle_status examples/vehicle_status/main.cpp)
target_link_libraries(vehicle_status PRIVATE smartvehicle)
add_executable(ota_simulation examples/ota_simulation/main.cpp)
target_link_libraries(ota_simulation PRIVATE smartvehicle)

enable_testing()
add_executable(smartvehicle_tests tests/test_main.cpp)
target_link_libraries(smartvehicle_tests PRIVATE smartvehicle)
add_test(NAME smartvehicle_tests COMMAND smartvehicle_tests)

install(TARGETS smartvehicle vehicle_status ota_simulation
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
set(CPACK_PACKAGE_NAME smartvehicle)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
include(CPack)
