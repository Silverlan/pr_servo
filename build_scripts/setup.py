import os
import sys
import argparse
from pathlib import Path
from urllib.parse import quote
import shutil
import subprocess
import config

parser = argparse.ArgumentParser(description='pr_servo build script', allow_abbrev=False, formatter_class=argparse.ArgumentDefaultsHelpFormatter, epilog="")
parser.add_argument("--build-servo", type=str2bool, nargs='?', const=True, default=False, help="Build the libservo C-API library (otherwise uses pre-built binaries).")
args,unknown = parser.parse_known_args()
args = vars(args)

# Building via script is currently not yet supported
# TODO: Implement based on workflow script:
# https://github.com/Silverlan/libservo_capi/blob/main/.github/workflows/build-libservo.yml
build_servo = False # build_all or args["build_servo"]
if build_servo:
	os.chdir(deps_dir)

	# TODO
else:
	print_msg("Downloading prebuilt servo binaries...")
	run_cmake_script(str(Path(moduleDir) / "cmake/fetch_prebuilt_binaries.cmake"))
