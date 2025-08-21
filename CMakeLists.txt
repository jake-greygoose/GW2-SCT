cmake_minimum_required(VERSION 3.20)

project(gw2-sct LANGUAGES C CXX)
set(TARGET_NAME gw2-sct)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
set(OPENSSL_USE_STATIC_LIBS ON)

file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
file(GLOB IMGUI_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/misc/cpp/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/*.c")
file(GLOB SIMPLEINI_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/simpleini/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/simpleini/*.c")

list(APPEND SOURCES ${IMGUI_SOURCES} ${SIMPLEINI_SOURCES})

add_library(${TARGET_NAME} SHARED ${SOURCES} ${HEADERS})
target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)

target_compile_definitions(${TARGET_NAME} PRIVATE NOMINMAX CPPHTTPLIB_OPENSSL_SUPPORT)
target_include_directories(${TARGET_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/misc/cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/httplib"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/json/single_include/nlohmann"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/simpleini"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/stb"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/object_threadsafe"
  "$<$<BOOL:${DXSDK_DIR}>:${DXSDK_DIR}/Include>"
)

if(MSVC)
  target_compile_options(${TARGET_NAME} PRIVATE "/Zc:preprocessor" "/MP")
endif()

# ---- OpenSSL (prefer CONFIG, fallback to module) ----
set(_OSSL_INCLUDES "")
find_package(OpenSSL CONFIG QUIET)
if(TARGET OpenSSL::SSL AND TARGET OpenSSL::Crypto)
  get_target_property(_ossl_inc OpenSSL::SSL INTERFACE_INCLUDE_DIRECTORIES)
  if(_ossl_inc)
    list(APPEND _OSSL_INCLUDES ${_ossl_inc})
  endif()
  target_link_libraries(${TARGET_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
else()
  find_package(OpenSSL REQUIRED)
  if(OPENSSL_INCLUDE_DIR)
    list(APPEND _OSSL_INCLUDES "${OPENSSL_INCLUDE_DIR}")
  endif()
  if(OPENSSL_INCLUDE_DIRS)
    list(APPEND _OSSL_INCLUDES ${OPENSSL_INCLUDE_DIRS})
  endif()
  if(OPENSSL_LIBRARIES)
    target_link_libraries(${TARGET_NAME} PRIVATE ${OPENSSL_LIBRARIES})
  else()
    target_link_libraries(${TARGET_NAME} PRIVATE ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
  endif()
endif()

# Last-resort include hint if nothing above produced an include dir
if(NOT _OSSL_INCLUDES AND DEFINED OPENSSL_ROOT_DIR)
  list(APPEND _OSSL_INCLUDES "${OPENSSL_ROOT_DIR}/include")
endif()

# Absolute hardening fallback (safe to keep)
list(APPEND _OSSL_INCLUDES "$ENV{USERPROFILE}/vcpkg/installed/x64-windows-static/include")

list(REMOVE_DUPLICATES _OSSL_INCLUDES)
target_include_directories(${TARGET_NAME} PRIVATE ${_OSSL_INCLUDES})

# ---- DirectX SDK (June 2010) ----
if(DXSDK_DIR)
  file(TO_CMAKE_PATH "${DXSDK_DIR}" _dxsdk_norm)
  target_link_directories(${TARGET_NAME} PRIVATE "${_dxsdk_norm}/Lib/x64")
  target_link_libraries(${TARGET_NAME} PRIVATE d3dx11 dxguid d3d11 dxgi)
endif()

# ---- Post-build copy/rename to GW2 ----
if(GW2_PATH AND GW2_SUBPATH)
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -f "${GW2_PATH}${GW2_SUBPATH}/d3d9_arcdps_sct.dll")
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${TARGET_NAME}>" "${GW2_PATH}${GW2_SUBPATH}/")
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rename "${GW2_PATH}${GW2_SUBPATH}/$<TARGET_FILE_NAME:${TARGET_NAME}>" "d3d9_arcdps_sct.dll")
endif()
