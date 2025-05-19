# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/asbri/esp-idf/components/bootloader/subproject")
  file(MAKE_DIRECTORY "/Users/asbri/esp-idf/components/bootloader/subproject")
endif()
file(MAKE_DIRECTORY
  "/Users/asbri/esp-idf/examples/wifi/getting_started/station/build/bootloader"
  "/Users/asbri/esp-idf/examples/wifi/getting_started/station/build/bootloader-prefix"
  "/Users/asbri/esp-idf/examples/wifi/getting_started/station/build/bootloader-prefix/tmp"
  "/Users/asbri/esp-idf/examples/wifi/getting_started/station/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/asbri/esp-idf/examples/wifi/getting_started/station/build/bootloader-prefix/src"
  "/Users/asbri/esp-idf/examples/wifi/getting_started/station/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/asbri/esp-idf/examples/wifi/getting_started/station/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/asbri/esp-idf/examples/wifi/getting_started/station/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
