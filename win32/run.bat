@echo off
set PATH=%CD%\..\win32\;%PATH%

echo Starting Tether...
cd node-tuntap
echo %CD%
..\win32\adb.exe start-server
node.exe tether.js

cd ..
exit /b
