pr_install_targets(pr_servo INSTALL_DIR "modules/servo/")
pr_install_files("${libservo_capi_RUNTIME}" INSTALL_DIR "modules/servo/")
pr_install_directory("${libservo_capi_RESOURCE_DIR}/" INSTALL_DIR "modules/servo/")
