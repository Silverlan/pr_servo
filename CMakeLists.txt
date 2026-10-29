include(${CMAKE_SOURCE_DIR}/cmake/pr_common.cmake)

include("cmake/fetch_prebuilt_binaries.cmake")

set(PROJ_NAME pr_servo)
pr_add_module(${PROJ_NAME} "modules/servo/")

pr_add_dependency(${PROJ_NAME} shared TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} client TARGET PUBLIC)

pr_init_module(${PROJ_NAME})

find_package(libservo_capi REQUIRED)

set_target_properties(pr_servo PROPERTIES FOLDER modules/servo)

pr_finalize(${PROJ_NAME} FOLDER modules/servo)
