@echo off

rem Delete existing files
set folder="client-publish"
IF EXIST "%folder%" (
    cd /d %folder%
    for /F "delims=" %%i in ('dir /b') do (rmdir "%%i" /s/q || del "%%i" /s/q)
    cd ..
)

mkdir client-publish\Hellas

rem Copy files
copy client.exe client-publish\Hellas\
copy 04B_03__.TTF client-publish\Hellas\
copy AdvoCut.ttf client-publish\Hellas\
copy client-config.xml client-publish\Hellas\
copy *.dll client-publish\Hellas\
xcopy Data client-publish\Hellas\Data\ /E
xcopy Images client-publish\Hellas\Images\ /E
xcopy Sounds client-publish\Hellas\Sounds\ /E

rem Assemble zip
cd client-publish
"C:\Program Files\7-Zip\7z" a client.zip Hellas\

rem Copy to local play directory
copy client.zip "F:\Permanent\Hellas production client\"
pushd "F:\Permanent\Hellas production client"
7z.exe x client.zip -aoa
popd

rem Publish zip
"C:\Program Files (x86)\WinSCP\WinSCP.com" ^
  /log="WinSCP.log" /ini=nul ^
  /command ^
    "open ftp://{username}:{password}@ftp.ipage.com/" ^
    "lcd ""{local directory}\client-publish""" ^
    "cd /hellas" ^
    "put client.zip" ^
    "exit"

set WINSCP_RESULT=%ERRORLEVEL%
if %WINSCP_RESULT% equ 0 (
  echo Success
) else (
  echo Error
)

exit /b %WINSCP_RESULT%
