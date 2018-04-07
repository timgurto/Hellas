@echo off

for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /format:list') do set datetime=%%I
set datetime=%datetime:~0,8%-%datetime:~8,6%
set dirName=backup\%datetime%

mkdir "%dirName%"
xcopy Users %dirName%\Users\ /E
xcopy World %dirName%\World\ /E