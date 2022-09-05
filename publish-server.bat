@echo off

rem Copy files
copy server.exe "F:\Permanent\Hellas production server\"
copy SDL2.dll "F:\Permanent\Hellas production server\"
copy libcurl.dll "F:\Permanent\Hellas production server\"
xcopy Data "F:\Permanent\Hellas production server\Data\" /E /Y
xcopy logging "F:\Permanent\Hellas production server\logging\" /E /Y /EXCLUDE:*.csv
xcopy Images\Items "F:\Permanent\Hellas production server\Images\Items\" /E /Y

rem Start server
F:
cd "F:\Permanent\Hellas production server"
launch.bat
