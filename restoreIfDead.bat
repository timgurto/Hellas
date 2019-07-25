TITLE Server recovery
:loop

cls

tasklist /FI "IMAGENAME eq server.exe" 2>NUL | find /I /N "server.exe">NUL
if NOT "%ERRORLEVEL%"=="0" (
    start launch.bat
)

timeout 10

goto loop