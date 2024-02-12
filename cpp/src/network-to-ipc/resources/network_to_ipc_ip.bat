@echo off

cd /D %~dp0

set /p Input=Digite o IP: 

network-to-ipc.exe %Input%
