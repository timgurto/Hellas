TASKKILL /F /IM "mmo.exe"
xcopy /y /d Debug\mmo.exe .
start mmo.exe -server -left 20 -top 20
timeout 1
start mmo.exe -left 825 -top 20
timeout 1
start mmo.exe -left 825 -top 525