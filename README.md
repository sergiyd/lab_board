### Prepare Arduino CLI environment
```sh
arduino-cli lib update-index
arduino-cli lib install DallasTemperature
arduino-cli lib install OneWire
arduino-cli lib install "Rtc by Makuna"
arduino-cli lib install WebSockets
```
### Upload client to board
```sh
cd client
LAB_BOARD_URL=http://<your board IP> ./upload_dist.sh
```
or via Docker
```sh
docker remove lab-board-admin-uploader
docker build --build-arg LAB_BOARD_URL=<your board IP> -t lab-board-admin-uploader ./client
docker run --name lab-board-admin-uploader lab-board-admin-uploader
```
