### Prepare Arduino CLI environment
```sh
arduino-cli lib update-index
arduino-cli lib install DallasTemperature
arduino-cli lib install OneWire
arduino-cli lib install "Rtc by Makuna"
arduino-cli lib install WebSockets
```
### Upload client to board
Linux
```sh
cd client
LAB_BOARD_URL=http://<your board URL> ./upload_dist.sh
```
or via Docker
```sh
docker build -t lab-board-admin-uploader ./client
docker run --rm -e LAB_BOARD_URL=<your board URL> -v ./client:/client lab-board-admin-uploader /client/build_upload.sh
```
