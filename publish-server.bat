@echo off

rem Copy files
copy server.exe "C:\Hellas server\"
copy SDL2.dll "C:\Hellas server\"
xcopy Data "C:\Hellas server\Data\" /E /Y
xcopy logging "C:\Hellas server\logging\" /E /Y
xcopy Images\Items "C:\Hellas server\Images\Items\" /E /Y

rem Start server
C:
cd "C:\Hellas server"
server.exe
