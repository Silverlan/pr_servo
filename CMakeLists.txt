include(${CMAKE_SOURCE_DIR}/cmake/pr_common.cmake)

set(PROJ_NAME pr_servo)

pr_add_module(${PROJ_NAME} "modules/servo/")

pr_add_dependency(${PROJ_NAME} shared TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} client TARGET PUBLIC)

pr_init_module(${PROJ_NAME})

pr_finalize(${PROJ_NAME})
