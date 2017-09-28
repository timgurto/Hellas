bitsadmin.exe /transfer "Downloading 7zip" http://hellas.timgurto.com/launcher/7z.exe "%CD%\7z.exe"
bitsadmin.exe /transfer "Downloading Hellas client" http://hellas.timgurto.com/client.zip "%CD%\client.zip"

7z.exe x client.zip -aoa

%SystemRoot%\explorer.exe "%CD%\Hellas"
