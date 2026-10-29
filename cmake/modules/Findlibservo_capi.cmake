set(PCK "libservo_capi")

if (${PCK}_FOUND)
  return()
endif()

find_library(${PCK}_RUNTIME
  NAMES servo_capi
  HINTS ${PRAGMA_DEPS_DIR}/servo/lib
)

find_path(${PCK}_RESOURCE_DIR
        NAMES servo.ico
        HINTS
        ${PRAGMA_DEPS_DIR}/servo/resources
)

set(REQ_VARS ${PCK}_RUNTIME ${PCK}_RESOURCE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(${PCK}
  REQUIRED_VARS ${REQ_VARS}
)
