include(${CMAKE_SOURCE_DIR}/cmake/pr_common.cmake)

include("cmake/fetch_prebuilt_binaries.cmake")

set(PROJ_NAME pr_servo)
pr_add_module(${PROJ_NAME} "modules/servo/")

pr_add_dependency(${PROJ_NAME} shared TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} client TARGET PUBLIC)

# Some servo C API functions are currently just pull requests and do not exist in
# libservo yet. We'll use stub placeholder functions until they do.
pr_add_compile_definitions(${PROJ_NAME} -DSERVO_USE_STUB_PLACEHOLDERS)
# Once the pull requests have been merged and libservo has been rebuilt, the stubs can be removed.
# See
# https://github.com/servo/servo/pull/49334
# and
# https://github.com/servo/servo/pull/49326

pr_init_module(${PROJ_NAME})

find_package(libservo_capi REQUIRED)

set_target_properties(pr_servo PROPERTIES FOLDER modules/servo)

pr_finalize(${PROJ_NAME} FOLDER modules/servo)
