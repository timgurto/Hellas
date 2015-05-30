TASKKILL /F /IM "mmo.exe"
xcopy /y /d Debug\mmo.exe .
start mmo.exe -server -left 10 -top 25
timeout 1
start mmo.exe -left 815 -top 25 -server-ip 127.0.0.1
timeout 1
start mmo.exe -left 815 -top 535 -server-ip 192.168.1.5
timeout 1
start mmo.exe -left 1460 -top 25 -server-ip 121.44.199.135 