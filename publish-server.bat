@echo off

rem Copy files
copy server.exe "C:\Hellas production server\"
copy SDL2.dll "C:\Hellas production server\"
copy libcurl.dll "C:\Hellas production server\"
xcopy Data "C:\Hellas production server\Data\" /E /Y
xcopy logging "C:\Hellas production server\logging\" /E /Y /EXCLUDE:*.csv
xcopy Images\Items "C:\Hellas production server\Images\Items\" /E /Y

rem Start server
cd "C:\Hellas production server"
launch.bat
