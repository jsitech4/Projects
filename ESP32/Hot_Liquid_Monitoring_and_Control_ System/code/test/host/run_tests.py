"""Build and run firmware fault regression tests without a board or PlatformIO."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    root = Path(__file__).resolve().parents[2]
    compiler = os.environ.get("CXX") or shutil.which("g++")
    if not compiler:
        raise SystemExit("Install g++ or set CXX to its executable path.")

    sources = [
        "test/host/test_faults.cpp",
        "lib/Control/device_manager/device_manager.cpp",
        "lib/Display/led_indicator/led_indicator.cpp",
        "lib/Actuators/buzzer/buzzer.cpp",
        "lib/Actuators/load_relay/load_relay.cpp",
        "lib/Pins/Pins.cpp",
    ]
    includes = [
        "test/host", "lib/Control", "lib/Display", "lib/Actuators",
        "lib/Sensors", "lib/Pins",
    ]
    environment = os.environ.copy()
    # MinGW runtime DLLs must be discoverable when the temporary binary runs.
    environment["PATH"] = str(Path(compiler).resolve().parent) + os.pathsep + environment.get("PATH", "")
    with tempfile.TemporaryDirectory(prefix="liquid-monitor-tests-") as temporary:
        executable = Path(temporary) / ("fault_tests.exe" if os.name == "nt" else "fault_tests")
        command = [compiler, "-std=c++17", "-Wall", "-Wextra", "-pedantic"]
        command += ["-I" + str(root / path) for path in includes]
        command += [str(root / path) for path in sources]
        command += ["-o", str(executable)]
        subprocess.run(command, cwd=root, env=environment, check=True)
        subprocess.run([str(executable)], cwd=root, env=environment, check=True)


if __name__ == "__main__":
    main()
