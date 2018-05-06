cmake_minimum_required(VERSION 3.8)

project(minhook LANGUAGES C)

foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
  if(${flag_var} MATCHES "/MD")
    string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
  endif(${flag_var} MATCHES "/MD")
endforeach(flag_var)

add_library(
  minhook STATIC 
  "minhook/src/buffer.c"
  "minhook/src/hook.c"
  "minhook/src/trampoline.c"
  "minhook/src/hde/hde32.c"
  "minhook/src/hde/hde64.c"
)

add_definitions(-D_UNICODE -DUNICODE)

target_include_directories(minhook PUBLIC ${PROJECT_SOURCE_DIR}/minhook/include)

if(WIN32 AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set_target_properties(minhook PROPERTIES
                        LINK_FLAGS_RELEASE "/NODEFAULTLIB:libucrt.lib ucrt.lib"
                        LINK_FLAGS_MINSIZEREL "/NODEFAULTLIB:libucrt.lib ucrt.lib"
                        LINK_FLAGS_RELWITHDEBINFO "/NODEFAULTLIB:libucrt.lib ucrt.lib"
                        LINK_FLAGS_DEBUG "/NODEFAULTLIB:libucrtd.lib ucrtd.lib")
else()
  message(SEND_ERROR "Only Windows and the MSVC compiler are supported.")
endif()

project(gsl)
add_library(gsl INTERFACE)
target_include_directories(gsl INTERFACE gsl/include)