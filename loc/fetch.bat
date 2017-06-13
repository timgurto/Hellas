git log --reverse --format=format:%%H > commits.txt

for /F "tokens=*" %%A in (commits.txt) do (
    git checkout %%A
    
    rem total
    cloc-1.64.exe --include-lang=cpp,h ../ | findstr SUM
    

    
    rem shared new
    cloc-1.64.exe --include-lang=cpp,h --not-match-f='Server' --not-match-f='Client' ../mmo/ ../src/*.* | findstr SUM
    
    echo %%A
)
