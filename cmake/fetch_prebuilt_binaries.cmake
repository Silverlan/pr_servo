include("${CMAKE_SOURCE_DIR}/cmake/install_helper.cmake")

set(version "2026-10-30")
set(base_url "https://github.com/Silverlan/libservo_capi/releases/download")
set(libservo_capi_toolset)

if(WIN32)
    set(libservo_capi_toolset "msvc")
endif()

if(NOT config.no_build_networking)
    pr_fetch_prebuilt_binaries("${PRAGMA_DEPS_DIR}/servo" "${base_url}" "${version}" TOOLSET ${libservo_capi_toolset})
endif()
