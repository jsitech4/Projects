"""Compile production persistence sources against native fault-injection adapters."""
from pathlib import Path
import os
import subprocess
ROOT=Path(__file__).resolve().parents[1]
output=ROOT/'test-results';output.mkdir(exist_ok=True)
binary=output/('persistence.exe' if os.name=='nt' else 'persistence')
includes=['test/host','include','lib/Core','lib/Configuration','lib/Storage','.pio/libdeps/native/ArduinoJson/src']
command=['g++','-std=c++17','-Wall','-Wextra']
for path in includes:command+=['-I',str(ROOT/path)]
command += [str(ROOT/p) for p in ['test/host/persistence_main.cpp','lib/Core/Core.cpp','lib/Configuration/Configuration.cpp','lib/Storage/Storage.cpp']]
command += ['-o',str(binary)]
subprocess.run(command,check=True,cwd=ROOT)
subprocess.run([str(binary)],check=True,cwd=ROOT)
portal=output/('portal.exe' if os.name=='nt' else 'portal')
command=['g++','-std=c++17','-Wall','-Wextra']
for path in ['test/host','include','lib/Core','lib/LocalWebServer','.pio/libdeps/native/ArduinoJson/src']:
    command+=['-I',str(ROOT/path)]
command += [str(ROOT/p) for p in ['test/host/portal_main.cpp','lib/Core/Core.cpp','lib/LocalWebServer/Portal.cpp']]
command += ['-o',str(portal)]
subprocess.run(command,check=True,cwd=ROOT)
subprocess.run([str(portal)],check=True,cwd=ROOT)
