cmake_minimum_required(VERSION 3.20)

project(gw2-sct LANGUAGES C CXX)
set(TARGET_NAME gw2-sct)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

function(gw2sct_pad VALUE WIDTH OUTPUT_VAR)
    set(_val "${VALUE}")
    string(LENGTH "${_val}" _len)
    set(_width "${WIDTH}")
    while(_len LESS _width)
        set(_val "0${_val}")
        math(EXPR _len "${_len} + 1")
    endwhile()
    set(${OUTPUT_VAR} "${_val}" PARENT_SCOPE)
endfunction()

set(_VERSION_PART_NAMES VER_MAJOR VER_MINOR VER_BUILD VER_REVISION)
set(_GW2SCT_VERSION_INPUT "")
if(DEFINED GW2SCT_VERSION_STRING AND NOT "${GW2SCT_VERSION_STRING}" STREQUAL "")
    set(_GW2SCT_VERSION_INPUT "${GW2SCT_VERSION_STRING}")
elseif(NOT "$ENV{GW2SCT_VERSION_STRING}" STREQUAL "")
    set(_GW2SCT_VERSION_INPUT "$ENV{GW2SCT_VERSION_STRING}")
endif()

if(_GW2SCT_VERSION_INPUT STREQUAL "")
    string(TIMESTAMP CURRENT_YEAR "%Y")
    string(TIMESTAMP CURRENT_MONTH "%m")
    string(TIMESTAMP CURRENT_DAY "%d")
    string(TIMESTAMP CURRENT_HOUR "%H")
    string(TIMESTAMP CURRENT_MINUTE "%M")

    math(EXPR VER_MAJOR "${CURRENT_YEAR}")
    math(EXPR VER_MINOR "${CURRENT_MONTH}")
    math(EXPR VER_BUILD "${CURRENT_DAY}")

    set(_REV_SEED "")
    foreach(_candidate "${GW2SCT_BUILD_NUMBER}" "$ENV{GW2SCT_BUILD_NUMBER}" "$ENV{GITHUB_RUN_NUMBER}" "$ENV{BUILD_BUILDNUMBER}" "$ENV{BUILD_NUMBER}")
        if(NOT "${_candidate}" STREQUAL "")
            set(_REV_SEED "${_candidate}")
            break()
        endif()
    endforeach()
    if(_REV_SEED STREQUAL "")
        math(EXPR _REV_SEED "(${CURRENT_HOUR} * 60) + ${CURRENT_MINUTE}")
    endif()
    math(EXPR VER_REVISION "${_REV_SEED}")
else()
    string(REGEX REPLACE "^[Vv]" "" _GW2SCT_VERSION_INPUT "${_GW2SCT_VERSION_INPUT}")
    string(REPLACE "." ";" VERSION_LIST "${_GW2SCT_VERSION_INPUT}")
    list(LENGTH VERSION_LIST VERSION_PART_COUNT)
    if(NOT VERSION_PART_COUNT EQUAL 4)
        message(FATAL_ERROR "GW2SCT_VERSION_STRING must be in the form YYYY.MM.DD.RRRR")
    endif()

    foreach(index RANGE 0 3)
        list(GET VERSION_LIST ${index} component)
        if(NOT component MATCHES "^[0-9]+$")
            message(FATAL_ERROR "Version component '${component}' is not numeric.")
        endif()
        string(REGEX REPLACE "^0*([0-9][0-9]*)$" "\\1" stripped "${component}")
        if(stripped STREQUAL "")
            set(stripped 0)
        endif()
        list(GET _VERSION_PART_NAMES ${index} component_name)
        set(${component_name} ${stripped})
    endforeach()
endif()

set(GW2SCT_VERSION_STRING_RAW "${VER_MAJOR}.${VER_MINOR}.${VER_BUILD}.${VER_REVISION}")
gw2sct_pad("${VER_MINOR}" 2 VER_MINOR_PAD)
gw2sct_pad("${VER_BUILD}" 2 VER_BUILD_PAD)
gw2sct_pad("${VER_REVISION}" 4 VER_REVISION_PAD)
set(GW2SCT_VERSION_STRING "${VER_MAJOR}.${VER_MINOR_PAD}.${VER_BUILD_PAD}.${VER_REVISION_PAD}")
set(GW2SCT_VERSION_CSV "${VER_MAJOR},${VER_MINOR},${VER_BUILD},${VER_REVISION}")
message(STATUS "Configuring GW2-SCT Version: ${GW2SCT_VERSION_STRING} (raw ${GW2SCT_VERSION_STRING_RAW})")
set(GW2SCT_VERSION_STRING "${GW2SCT_VERSION_STRING}" CACHE STRING "Display version string shown by addon" FORCE)
set(GW2SCT_VERSION_STRING_RAW "${GW2SCT_VERSION_STRING_RAW}" CACHE STRING "Unpadded version string shown externally" FORCE)

set(AUTOVERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/autoversion.h")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/autoversion.h.in"
    "${AUTOVERSION_HEADER}"
    @ONLY
)

set(AUTOVERSION_RC "")
if(WIN32)
    set(AUTOVERSION_RC "${CMAKE_CURRENT_BINARY_DIR}/autoversion.rc")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/src/autoversion.rc.in"
        "${AUTOVERSION_RC}"
        @ONLY
    )
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
file(GLOB IMGUI_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/misc/cpp/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/*.c")
list(APPEND SOURCES ${IMGUI_SOURCES})

add_library(${TARGET_NAME} SHARED ${SOURCES} ${HEADERS} "include/SkillFilterStructures.h" "include/Mumblelink.h" "src/Mumblelink.cpp")
if(AUTOVERSION_RC)
  target_sources(${TARGET_NAME} PRIVATE "${AUTOVERSION_RC}")
endif()
target_link_libraries(${TARGET_NAME} PRIVATE version)
target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)

target_compile_definitions(${TARGET_NAME} PRIVATE NOMINMAX)
target_include_directories(${TARGET_NAME} PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/misc/cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/json/single_include/nlohmann"
  "${CMAKE_CURRENT_SOURCE_DIR}/submodules/stb"
)

if(MSVC)
  target_compile_options(${TARGET_NAME} PRIVATE "/Zc:preprocessor" "/MP")
  # Ensure Release builds also emit Program Database files for symbol distribution
  target_compile_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:Release>:/Zi>")
  target_link_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:Release>:/DEBUG:FULL>")
  set_target_properties(${TARGET_NAME} PROPERTIES
    PDB_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/Release"
    COMPILE_PDB_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/Release")
endif()

# ---- Windows VERSIONINFO resource for DLL file properties ----
if(WIN32)
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

# ---- Direct3D 11 ----
target_link_libraries(${TARGET_NAME} PRIVATE d3d11 dxgi dxguid)

# ---- Post-build copy/rename to GW2 ----
if(GW2_PATH AND GW2_SUBPATH)
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -f "${GW2_PATH}${GW2_SUBPATH}/d3d9_arcdps_sct.dll")
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${TARGET_NAME}>" "${GW2_PATH}${GW2_SUBPATH}/")
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rename "${GW2_PATH}${GW2_SUBPATH}/$<TARGET_FILE_NAME:${TARGET_NAME}>" "d3d9_arcdps_sct.dll")
endif()
