TASKKILL /F /IM "mmo.exe"
xcopy /y /d Debug\mmo.exe .
start mmo.exe -server
timeout 1
start mmo.exe
timeout 1
start mmo.exe