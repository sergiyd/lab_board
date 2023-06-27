#!/bin/sh

cd dist/client

curl -X DELETE http://$LAB_BOARD_URL/files

find . -type f -print0 | while read -d $'\0' file; do
    curl -X POST --verbose -F "file=@$file;filename=${file#./}" $LAB_BOARD_URL/upload
done
