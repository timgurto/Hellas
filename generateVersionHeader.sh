#!C:/Program\ Files/Git/usr/bin/sh.exe

revisioncount=`git log --oneline | wc -l`
projectversion=`git describe --tags --long`

echo "$projectversion" > version.txt

echo "#define VERSION \"$projectversion\""
echo "#define VERSION \"$projectversion\"" > src/version.h
git add src/version.h