cmake_minimum_required(VERSION 3.16.3)
# or use: /opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-cmake
include(/opt/devkitpro/cmake/Switch.cmake)
project(ezremote-client VERSION 1.0.0 LANGUAGES C CXX)
set(TITLE_NAME "ezRemote Client")
set(PROJECT_AUTHOR "cy33hc")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")

set(TARGET ${PROJECT_NAME})
set(IMGUI_DIR third_parties/imgui)
add_definitions("-D_U_=")
add_definitions("-DHAVE_STRING_H -DHAVE_STDLIB_H -DHAVE_NETINET_IN_H -DHAVE_NETINET_TCP_H")

include_directories(
  source
  source/pugixml
)

add_executable(${TARGET}
  source/main.cpp
  source/inifile.c
  source/lang.cpp
  source/fs.cpp
  source/pugixml/pugixml.cpp
  source/webdav/callback.cpp
  source/webdav/client.cpp
  source/webdav/fsinfo.cpp
  source/webdav/header.cpp
  source/webdav/request.cpp
  source/webdav/urn.cpp
  source/clients/smbclient.cpp
  source/clients/ftpclient.cpp
  source/clients/webdavclient.cpp
  source/actions.cpp
  source/gui.cpp
  source/windows.cpp
  source/config.cpp
  source/ime_dialog.cpp
  source/imgui_impl_switch.cpp
  source/clients/ftpclient.cpp
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
  )
  
target_compile_features(${TARGET} PUBLIC cxx_std_17)
target_compile_definitions(${TARGET} PUBLIC
  IMGUI_DISABLE_OBSOLETE_KEYIO
  IMGUI_DISABLE_OBSOLETE_FUNCTIONS
  IMGUI_IMPL_OPENGL_LOADER_GLAD
  IMGUI_DISABLE_DEMO_WINDOWS
  IMGUI_DISABLE_DEBUG_TOOLS
  IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS
  IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
  IMGUI_DISABLE_WIN32_FUNCTIONS
  IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
  IMGUI_ENABLE_FREETYPE
  )

target_link_libraries(${TARGET} PUBLIC
  glad
  EGL
  glapi
  drm_nouveau
  freetype
  bz2
  png
  curl
  ssl
  crypto
  m
  z
  smb2
)

target_include_directories(${TARGET} PUBLIC
  ${DEVKITPRO}/portlibs/switch/include
  ${DEVKITPRO}/portlibs/switch/include/freetype2
  ${IMGUI_DIR})

file(COPY icons DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/romfs)
file(COPY lang DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/romfs)
dkp_add_asset_target(${TARGET}_romfs ${CMAKE_CURRENT_BINARY_DIR}/romfs)

nx_generate_nacp(${TARGET}.nacp
  NAME ${TITLE_NAME}
  AUTHOR ${PROJECT_AUTHOR}
  )
nx_create_nro(${TARGET}
  NACP ${TARGET}.nacp
  ROMFS ${TARGET}_romfs
  ICON ${CMAKE_SOURCE_DIR}/icon.jpg
  )
