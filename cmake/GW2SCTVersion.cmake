# Centralized helpers for deriving GW2-SCT version strings/build numbers

function(gw2sct_seed_version var_name)
  if(NOT var_name)
    message(FATAL_ERROR "gw2sct_seed_version requires a variable name")
  endif()

  if(DEFINED ${var_name} AND NOT "${${var_name}}" STREQUAL "")
    set(${var_name} "${${var_name}}" PARENT_SCOPE)
    return()
  endif()

  get_property(_gw2sct_cache_has CACHE "${var_name}" PROPERTY TYPE SET)
  if(_gw2sct_cache_has)
    get_property(_gw2sct_cache_val CACHE "${var_name}" PROPERTY VALUE)
    if(NOT "${_gw2sct_cache_val}" STREQUAL "")
      set(${var_name} "${_gw2sct_cache_val}" PARENT_SCOPE)
      return()
    endif()
  endif()

  if(DEFINED ENV{GW2SCT_VERSION_STRING} AND NOT "$ENV{GW2SCT_VERSION_STRING}" STREQUAL "")
    set(${var_name} "$ENV{GW2SCT_VERSION_STRING}" PARENT_SCOPE)
    return()
  endif()

  string(TIMESTAMP _date "%Y.%m.%d")

  set(_build_seed "")
  foreach(_candidate "${GW2SCT_BUILD_NUMBER}" "$ENV{GW2SCT_BUILD_NUMBER}" "$ENV{GITHUB_RUN_NUMBER}" "$ENV{BUILD_BUILDNUMBER}" "$ENV{BUILD_NUMBER}")
    if(NOT "${_candidate}" STREQUAL "")
      set(_build_seed "${_candidate}")
      break()
    endif()
  endforeach()

  if(_build_seed STREQUAL "")
    string(TIMESTAMP _hour "%H")
    string(TIMESTAMP _minute "%M")
    math(EXPR _build_seed "${_hour} * 60 + ${_minute}")
  endif()

  set(${var_name} "v${_date}.${_build_seed}" PARENT_SCOPE)
endfunction()

function(gw2sct_log_version var_name)
  if(var_name AND DEFINED ${var_name})
    message(STATUS "GW2SCT version resolved to ${${var_name}}")
  endif()
endfunction()
