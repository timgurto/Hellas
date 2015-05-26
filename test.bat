rem @echo off

xcopy Debug\mmo.exe .
start mmo.exe -server
timeout 1
start mmo.exe
start mmo.exe