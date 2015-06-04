rem xcopy /y /d Debug\mmo.exe .
rem for /L %%x in (805, 645, 3195) do for /L %%y in (25, 510, 1650) do (
rem     start mmo.exe -left %%x -top %%y -server-ip 192.168.1.5
rem     timeout 1
rem )

TASKKILL /F /IM "mmo.exe"
xcopy /y /d Debug\mmo.exe .
start mmo.exe -server-ip 192.168.1.5 -username archon
mmo.exe -left 5 -top 25 -server
