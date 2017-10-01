TreeGen.exe
dot.exe -Tcmapx -o../web/tree.map -Tsvg:cairo -o../web/tree.svg tree.gv
dot.exe -Tpng < tree.gv > ../web/tree.png