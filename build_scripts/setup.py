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

build_servo = build_all or args["build_servo"]
if build_servo:
	# Required so we can import relative packages
	sys.path.insert(0, os.path.dirname(__file__))

	os.chdir(deps_dir)

	from third_party import servo
	servo_info = servo.main()
else:
	print_msg("Downloading prebuilt servo binaries...")
	run_cmake_script(str(Path(moduleDir) / "cmake/fetch_prebuilt_binaries.cmake"))
