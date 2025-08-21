# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/misha/esp/v5.4.1/esp-idf/components/bootloader/subproject"
  "/home/misha/Desktop/Projects/AI-Lawn-Mower/ai-lawn-mower/build/bootloader"
  "/home/misha/Desktop/Projects/AI-Lawn-Mower/ai-lawn-mower/build/bootloader-prefix"
  "/home/misha/Desktop/Projects/AI-Lawn-Mower/ai-lawn-mower/build/bootloader-prefix/tmp"
  "/home/misha/Desktop/Projects/AI-Lawn-Mower/ai-lawn-mower/build/bootloader-prefix/src/bootloader-stamp"
  "/home/misha/Desktop/Projects/AI-Lawn-Mower/ai-lawn-mower/build/bootloader-prefix/src"
  "/home/misha/Desktop/Projects/AI-Lawn-Mower/ai-lawn-mower/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/misha/Desktop/Projects/AI-Lawn-Mower/ai-lawn-mower/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/misha/Desktop/Projects/AI-Lawn-Mower/ai-lawn-mower/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
