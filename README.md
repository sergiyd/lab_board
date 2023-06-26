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
