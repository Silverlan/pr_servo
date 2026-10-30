import os, sys, subprocess, shutil
from pathlib import Path

sys.path.insert(0, str(Path(os.path.abspath(os.path.dirname(__file__))).parent.parent.parent.parent / "build_scripts"))
from scripts.shared import *

def run(cmd, shell=False, cwd=None):
	print_msg("Running: " +(cmd if isinstance(cmd, str) else " ".join(cmd)))
	subprocess.run(cmd, shell=shell, cwd=cwd, check=True)

def main():
	deps_dir = config.deps_dir

	chdir_mkdir(deps_dir)
	os.chdir(deps_dir)

	# See https://book.servo.org/building/linux.html#dependencies-for-debian for build instructions
	if platform == "linux":
		if prefer_apt():
			packages = ["curl", "build-essential", "ca-certificates", "ccache", "clang", "cmake", "curl", "g++", "git", "gperf", "gstreamer1.0-libav", "gstreamer1.0-plugins-bad", "gstreamer1.0-plugins-base", "gstreamer1.0-plugins-good", "gstreamer1.0-plugins-ugly", "gstreamer1.0-tools", "libdbus-1-dev", "libegl1-mesa-dev", "libfreetype6-dev", "libges-1.0-dev", "libgl1-mesa-dri", "libgles2-mesa-dev", "libglib2.0-dev", "libgstreamer-plugins-bad1.0-dev", "libgstreamer-plugins-base1.0-dev", "libgstrtspserver-1.0-dev", "libharfbuzz-dev", "liblzma-dev", "libudev-dev", "libunwind-dev", "libvulkan1", "libx11-dev", "libxcb-render0-dev", "libxcb-shape0-dev", "libxcb-xfixes0-dev", "libxkbcommon-x11-0", "libxkbcommon0", "libxmu-dev", "libxmu6", "llvm-dev", "m4", "xorg-dev"]
			commands = []
			commands.append("apt update")
			commands.append("apt install " +" ".join(packages))
			install_system_packages(commands, config.no_confirm)
	
	# Note: This should match the prebuilt version in cmake/fetch_prebuilt_binaries.cmake
	commit_sha = "5daca8588b16781b28fde02bd8212b310c8b65c9"
	servo_root = normalize_path(os.getcwd() +"/servo")

	if not Path(servo_root).is_dir():
		print_msg("Servo not found. Downloading...")
		git_clone("https://github.com/servo/servo.git", directory="servo")

	os.chdir("servo")
	if commit_sha is not None:
		reset_to_commit(commit_sha)

	# Install uv
	if shutil.which("uv") is None:
		print_msg("Installing uv...")
		if platform == "win32":
			run(["powershell", "-ExecutionPolicy", "ByPass", "-c", "irm https://astral.sh/uv/install.ps1 | iex"])
		else:
			run("curl -LsSf https://astral.sh/uv/install.sh | sh", shell=True)

	# Install Rust
	if shutil.which("rustup") is None:
		print_msg("Installing Rust...")
		if platform == "win32":
			print_msg("Please install Rust manually from https://rustup.rs and re-run this script.")
			sys.exit(1)
		run("curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y", shell=True)

	# Add new tools to PATH
	home = str(Path.home())
	os.environ["PATH"] = os.pathsep.join([
		os.environ["PATH"],
		os.path.join(home, ".local", "bin"),
		os.path.join(home, ".cargo", "bin")
	])

	mach = [sys.executable, "mach"] if platform == "win32" else ["./mach"]

	print_msg("Bootstrapping servo...")
	run(mach +["bootstrap", "--skip-lints"], cwd=servo_root)

	print_msg("Building servo-capi...")
	run(["cargo", "build", "-p", "servo-capi", "--release", "--jobs", "2"], cwd=servo_root)

	copy_prebuilt_binaries(servo_root +"/target/release/", "servo", recursive=False)
	src = Path(servo_root) / "resources"
	dst = Path(get_library_root_dir("servo")) / "resources"
	shutil.rmtree(dst, ignore_errors=True)
	dst.mkdir(parents=True, exist_ok=True)
	shutil.copytree(src, dst, dirs_exist_ok=True)

	return {
		"buildDir": servo_root
	}

if __name__ == "__main__":
	main()
