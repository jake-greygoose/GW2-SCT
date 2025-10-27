cmake_minimum_required(VERSION 3.20)

project(gw2-sct LANGUAGES C CXX)
set(TARGET_NAME gw2-sct)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GW2SCTVersion)

gw2sct_seed_version(GW2SCT_VERSION_STRING)
gw2sct_log_version(GW2SCT_VERSION_STRING)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
file(GLOB IMGUI_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/misc/cpp/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/*.c")
list(APPEND SOURCES ${IMGUI_SOURCES})

add_library(${TARGET_NAME} SHARED ${SOURCES} ${HEADERS} "include/SkillFilterStructures.h" "include/Mumblelink.h" "src/Mumblelink.cpp")
target_link_libraries(${TARGET_NAME} PRIVATE version)
target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)

target_compile_definitions(${TARGET_NAME} PRIVATE NOMINMAX)
target_include_directories(${TARGET_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/misc/cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/json/single_include/nlohmann"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/stb"
)

# Normalize optional DXSDK path once so includes/links stay consistent
set(DXSDK_DIR_CLEAN "${DXSDK_DIR}")
string(REPLACE "\"" "" DXSDK_DIR_CLEAN "${DXSDK_DIR_CLEAN}")
if(DXSDK_DIR_CLEAN)
  file(TO_CMAKE_PATH "${DXSDK_DIR_CLEAN}" DXSDK_DIR_CLEAN)
  target_include_directories(${TARGET_NAME} PRIVATE "${DXSDK_DIR_CLEAN}/Include")
endif()

if(MSVC)
  target_compile_options(${TARGET_NAME} PRIVATE "/Zc:preprocessor" "/MP")
endif()

# ---- Embed display version into addon from CMake/BAT (SCT_VERSION_STRING) ----
set(GW2SCT_VERSION_STRING "${GW2SCT_VERSION_STRING}" CACHE STRING "Display version string shown by addon" FORCE)
target_compile_definitions(${TARGET_NAME} PRIVATE SCT_VERSION_STRING=\"${GW2SCT_VERSION_STRING}\")

# ---- Windows VERSIONINFO resource for DLL file properties ----
if(WIN32)
  # Numeric file version seed, e.g., 2025.8.19.826; if not provided, derive from display string
  if(NOT GW2SCT_FILE_VERSION)
    set(GW2SCT_FILE_VERSION "${GW2SCT_VERSION_STRING}")
  endif()
  string(REGEX REPLACE "^v" "" GW2SCT_FILE_VERSION "${GW2SCT_FILE_VERSION}")
  # Split into up to 4 components
  string(REPLACE "." ";" _ver_list "${GW2SCT_FILE_VERSION}")
  set(GW2SCT_VER_MAJOR 0)
  set(GW2SCT_VER_MINOR 0)
  set(GW2SCT_VER_PATCH 0)
  set(GW2SCT_VER_BUILD 0)
  list(LENGTH _ver_list _ver_len)
  if(_ver_len GREATER 0)
    list(GET _ver_list 0 GW2SCT_VER_MAJOR)
  endif()
  if(_ver_len GREATER 1)
    list(GET _ver_list 1 GW2SCT_VER_MINOR)
  endif()
  if(_ver_len GREATER 2)
    list(GET _ver_list 2 GW2SCT_VER_PATCH)
  endif()
  if(_ver_len GREATER 3)
    list(GET _ver_list 3 GW2SCT_VER_BUILD)
  endif()
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc @ONLY)
  target_sources(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
  # Generate RC that embeds only existing PNGs
  set(GW2SCT_RES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")
  set(_RES_GEN "${CMAKE_CURRENT_BINARY_DIR}/resources_gen.rc")
  # Write include with path relative to build dir to avoid spaces issues
  set(_RES_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/resources/resource.h")
  file(RELATIVE_PATH _RES_HEADER_REL "${CMAKE_CURRENT_BINARY_DIR}" "${_RES_HEADER}")
  file(WRITE "${_RES_GEN}" "#include \"${_RES_HEADER_REL}\"\n")

  # Map of skill-id to png filename
  set(_ICON_LIST
    736;bleeding.png
    737;burning.png
    723;poison.png
    861;confusion.png
    873;retaliation.png
    19426;torment.png
    718;regeneration.png
    17495;regeneration.png
    17674;regeneration.png
    12601;Natures_Renewal.png
    69336;Natures_Renewal.png
    12825;water_blast_combo.png
    12836;water_blast_combo.png
  )
  list(LENGTH _ICON_LIST _ICON_LEN)
  math(EXPR _PAIRS "${_ICON_LEN} / 2")
  foreach(_i RANGE 0 ${_PAIRS})
    math(EXPR _idx_id   "${_i}*2")
    math(EXPR _idx_file "${_i}*2+1")
    if(_idx_file LESS _ICON_LEN)
      list(GET _ICON_LIST ${_idx_id}  _skill_id)
      list(GET _ICON_LIST ${_idx_file} _png)
      set(_png_path "${GW2SCT_RES_DIR}/${_png}")
      if(EXISTS "${_png_path}")
        # Use path relative to build dir to avoid spaces in absolute path
        file(RELATIVE_PATH _png_rel "${CMAKE_CURRENT_BINARY_DIR}" "${_png_path}")
        # Escape backslashes for RC string
        string(REPLACE "\\" "\\\\" _png_rel_esc "${_png_rel}")
        file(APPEND "${_RES_GEN}" "${_skill_id} RCDATA \"${_png_rel_esc}\"\n")
      endif()
    endif()
  endforeach()
  target_sources(${TARGET_NAME} PRIVATE ${_RES_GEN})
endif()

# ---- WinHTTP (Windows native HTTP client) ----
target_link_libraries(${TARGET_NAME} PRIVATE winhttp)

# ---- DirectX SDK (June 2010) ----
if(DXSDK_DIR_CLEAN)
  target_link_directories(${TARGET_NAME} PRIVATE "${DXSDK_DIR_CLEAN}/Lib/x64")
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
