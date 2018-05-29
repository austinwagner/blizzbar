cmake_minimum_required(VERSION 3.8)

foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
  if(${flag_var} MATCHES "/MD")
    string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
  endif(${flag_var} MATCHES "/MD")
endforeach(flag_var)

add_executable(
  blizzbar
  "src/Handle.h"
  "src/Handle.cpp"
  "src/main.cpp"
  "src/NotifyIcon.h"
  "src/NotifyIcon.cpp"
  "src/NotifyIconWindow.h"
  "src/NotifyIconWindow.cpp"
  "src/Win32Exception.h"
  "src/Win32Exception.cpp"
  "src/WindowClass.h"
  "src/WindowClass.cpp"
  "res/resource.h"
  "res/resource.rc"
  "res/blizzbar.ico"
  "res/strings.h"
  "res/blizzbar.exe.manifest"
)

add_definitions(-D_UNICODE -DUNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

target_link_libraries(blizzbar blizzbar-common gsl)
target_compile_features(blizzbar PRIVATE cxx_std_17)

if(WIN32)
  if (${CMAKE_SYSTEM_VERSION} GREATER_EQUAL 6.1)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set_target_properties(blizzbar PROPERTIES SUFFIX "-surrogate.exe")
      elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set_target_properties(blizzbar PROPERTIES SUFFIX ".exe")
      else()
        message(SEND_ERROR "Target system has unsupported pointer size.")
      endif()

      set_target_properties(blizzbar PROPERTIES
                            LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /NODEFAULTLIB:libucrt.lib ucrt.lib"
                            LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS /NODEFAULTLIB:libucrt.lib ucrt.lib"
                            LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS /NODEFAULTLIB:libucrt.lib ucrt.lib"
                            LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS /NODEFAULTLIB:libucrtd.lib ucrtd.lib")
    else()
      message(SEND_ERROR "Only the MSVC compiler is supported.")
    endif()
  else()
    message(SEND_ERROR "Come on, Windows didn't have taskbar pinning before Windows 7. You can't target anything earlier than that.")
  endif()
else()
  message(SEND_ERROR "This isn't a portable program. What the heck are you doing trying to build this for a non-Windows target?")
endif()
