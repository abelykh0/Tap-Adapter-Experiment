@echo off
set PATH=%CD%\..\win32\;%PATH%

echo %CD%
echo %1

echo Checking arguments...
IF %1=="" exit /b

echo Setting IP Address, mask, and gateway.
echo netsh interface ip set address name=%1 source=static 10.0.0.1 255.255.255.0 10.0.0.2 1
netsh interface ip set address name=%1 source=static 10.0.0.1 255.255.255.0 10.0.0.2 1
echo Setting DNS server.
echo netsh interface ip add dns name=%1 8.8.8.8 index=1
netsh interface ip add dns name=%1 8.8.8.8 index=1
echo netsh interface ip add dns name=%1 8.8.4.4 index=2
netsh interface ip add dns name=%1 8.8.4.4 index=2

echo Adding Windows Firewall Exception
echo netsh firewall set allowedprogram program="%CD%\win32\node.exe" name=Tether
netsh firewall set allowedprogram program="%CD%\win32\node.exe" name=Tether

echo Starting Tether...
cd node-tuntap
echo %CD%
..\win32\adb.exe start-server
..\win32\node.exe tether.js

exit /b
