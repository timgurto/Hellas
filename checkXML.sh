#!C:/Program\ Files/Git/usr/bin/sh.exe

case "${1}" in
    --about )
        echo "Checkint XML xmllint"
        ;;
    * )
        for FILE in $(git diff --cached --diff-filter=ACMTR --name-only HEAD | grep -E '\.xml|\.usr'); do
            echo xmllint --noout $FILE;
            third-party/xmllint/xmllint $FILE;
        done
        ;;
esac