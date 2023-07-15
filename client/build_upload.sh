#!/bin/bash

cd /client

npm i
npm run build

./upload_dist.sh
