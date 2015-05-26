rem @echo off

xcopy -y Debug\mmo.exe .
start mmo.exe -server
timeout 1
start mmo.exe
start mmo.exe