#include <DallasTemperature.h>
#include <RtcDS3231.h>
#include <Wire.h>
#include <OneWire.h>
#include <SD.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

#define DEBUG

#define INITIAL_DELAY 1000
#define INTERLOOP_DELAY_IDLE 200
#define INTERLOOP_DELAY_CAPTURING 100
#define RESTART_DELAY 500
#define EEPROM_WIFI_OFFSET 0
#define EEPROM_DEVICES_OFFSET 96
#define EEPROM_WIFI_BUFFER EEPROM_WIFI_OFFSET + WIFI_SSID_MAX_LEN + WIFI_PWD_MAX_LEN
#define EEPROM_DEVICES_BUFFER EEPROM_DEVICES_OFFSET + DEVICE_MAX_COUNT + ((DEVICE_LENGTH + DEVICE_EXTRA_LENGTH) * DEVICE_MAX_COUNT)
#define EEPROM_DEVICES_EXTRA_OFFSET EEPROM_DEVICES_OFFSET + DEVICE_MAX_COUNT + (DEVICE_LENGTH * DEVICE_MAX_COUNT)

#define SERIAL_BAUD_RATE 115200

#define SPI_CLK_PIN D5
#define SPI_CLK_PIN_ADDRESS 0x5
#define SPI_MISO_PIN D6
#define SPI_MISO_PIN_ADDRESS 0x6
#define SPI_MOSI_PIN D7
#define SPI_MOSI_PIN_ADDRESS 0x7
#define SPI_CS_PIN D8
#define SPI_CS_PIN_ADDRESS 0x8

#define EXTENSION_BOARD_RESET_PIN D0
#define EXTENSION_BOARD_RESET_PIN_ADDRESS 0x0

#define ACS712_PIN 0
#define ACS712_PIN_ADDRESS 0x0

#define OW_PIN D4
#define OW_PIN_ADDRESS 0x4

#define I2C_D_PIN D2
#define I2C_D_PIN_ADDRESS 0x2
#define I2C_C_PIN D3
#define I2C_C_PIN_ADDRESS 0x3

#define WEB_PAGE_NOT_FOUND 0
#define WEB_PAGE_UPLOAD_GET 1
#define WEB_PAGE_UPLOAD_POST 2
#define WEB_PAGE_PATH 16

#define WEB_SERVER_PORT 80
#define WEBSOCKET_SERVER_PORT 81
#define WEB_WRITE_BUFFER_LEN 32
#define HTTP_OK 200
#define HTTP_CREATED 201
#define HTTP_NOT_FOUND 404
#define HTTP_SERVER_ERROR 500

#define WIFI_RECONNECT_DELAY 2000
#define WIFI_RECONNECT_ATTEMPTS 5
#define WIFI_SSID_MAX_LEN 32
#define WIFI_PWD_MAX_LEN 32

#define SYSTEM_OPTION_SERIAL 1
#define SYSTEM_OPTION_SD 2
#define SYSTEM_OPTION_WIFI 4
#define SYSTEM_OPTION_WEB 8
#define SYSTEM_OPTION_EXTENSION_DEVICE 16

#define SYSTEM_STATE_UNKNOWN 0
#define SYSTEM_STATE_IDLE 1
#define SYSTEM_STATE_RESTARTING 2
#define SYSTEM_STATE_CAPTURING 3

#define MSG_SBJ_BOARD 0
#define MSG_SBJ_MANAGEMENT 1
#define MSG_SBJ_MONITORING 2
#define MSG_SBJ_HISTORY 3

#define MSG_CMD_BOARD_STATE 1
#define MSG_CMD_BOARD_CAPTURE 2
#define MSG_CMD_BOARD_SUBSCRIBE 3

#define MSG_CMD_MANAGEMENT_REGISTERED_DEVICES_COUNT 1
#define MSG_CMD_MANAGEMENT_REGISTERED_DEVICE 2
#define MSG_CMD_MANAGEMENT_SWITCH_DEVICE 3
#define MSG_CMD_MANAGEMENT_REGISTER_DEVICE 4
#define MSG_CMD_MANAGEMENT_RENAME_DEVICE 5
#define MSG_CMD_MANAGEMENT_UNREGISTER_DEVICE 6
#define MSG_CMD_MANAGEMENT_ALL_DEVICES_COUNT 7
#define MSG_CMD_MANAGEMENT_DEVICE 8
#define MSG_CMD_MANAGEMENT_DEVICE_VALUE 9

#define MSG_CMD_MONITORING_SOURCES_COUNT 1
#define MSG_CMD_MONITORING_SOURCE 2
#define MSG_CMD_MONITORING_SOURCE_DATA 3
#define MSG_CMD_MONITORING_PUT_EXTRA 4
#define MSG_CMD_MONITORING_MUTE 5

#define MSG_CMD_HISTORY_FILES_COUNT 1
#define MSG_CMD_HISTORY_FILE 2
#define MSG_CMD_HISTORY_REMOVE_FILE 3

#define WEB_SOCKET_NUMBER_BROADCAST 255
#define WEB_SOCKET_NUMBER_BROADCAST_BOARD WEB_SOCKET_NUMBER_BROADCAST - MSG_SBJ_BOARD
#define WEB_SOCKET_NUMBER_BROADCAST_MANAGEMENT WEB_SOCKET_NUMBER_BROADCAST - MSG_SBJ_MANAGEMENT
#define WEB_SOCKET_NUMBER_BROADCAST_MONITORING WEB_SOCKET_NUMBER_BROADCAST - MSG_SBJ_MONITORING
#define WEB_SOCKET_NUMBER_BROADCAST_HISTORY WEB_SOCKET_NUMBER_BROADCAST - MSG_SBJ_HISTORY

#define MSG_DATA_MAX_LEN 80
#define MSG_EXTENSION_BOARD_MAX_LEN 8

#define DEVICE_MAX_COUNT 32
#define DEVICE_NAME_LENGTH 32
#define DEVICE_NAME_CONTENT_LENGTH DEVICE_NAME_LENGTH - 1
#define DEVICE_ADDRESS_LENGTH 16
#define DEVICE_EXTRA_LENGTH 8
#define DEVICE_LENGTH sizeof(stored_device_t)

#define DEVICE_FLAG_MANAGEMENT_MASK 0x07
#define DEVICE_FLAG_SYSTEM 1
#define DEVICE_FLAG_OUTPUT 2
#define DEVICE_FLAG_ENABLED 4
#define DEVICE_FLAG_MUTED 8
#define DEVICE_FLAG_NO_DATA 16
#define DEVICE_FLAG_SYNC 32
#define DEVICE_FLAG_ACTIVE 64
#define DEVICE_FLAG_MONIRORING_MASK DEVICE_FLAG_ACTIVE
#define DEVICE_FLAG_SYNC_MASK_CLEAR 0xFF ^ DEVICE_FLAG_SYNC

#define DEVICE_BUS_UNKNOWN 0
#define DEVICE_BUS_ANALOG 1
#define DEVICE_BUS_DIGITAL 2
#define DEVICE_BUS_I2C 3
#define DEVICE_BUS_OW 4
#define DEVICE_BUS_EXTENSION_DEVICE_ANALOG 5
#define DEVICE_BUS_EXTENSION_DEVICE_DIGITAL 6

#define DEVICE_TYPE_UNKNOWN 0
#define DEVICE_TYPE_PIN 1
#define DEVICE_TYPE_DS3231_TIME 2
#define DEVICE_TYPE_DS3231_TEMPERATURE 3
#define DEVICE_TYPE_DS18B20 4
#define DEVICE_TYPE_EXTENSION_BOARD 8

#define DEVICE_SYSTEM_COUNT sizeof(SystemDevices) / sizeof(device_t *)

#define DS18B20_RESOLUTION 10

#define SOURCE_DATA_INDEX_IGNORE 0xFF
#define EXTENSION_BOARD_ADDRESS 111

#define EXTENSION_BOARD_RESET_DELAY 10
#define EXTENSION_BOARD_WARMUP_DELAY 3000

#define CAPTURING_FILE_NAME_MAX_LEN 64
#define CAPTURING_DEVICE_ADDRESS_MAX_LEN (DEVICE_ADDRESS_LENGTH * 3) + DEVICE_ADDRESS_LENGTH
#define CAPTURING_DEVICE_EXTRA_MAX_LEN (DEVICE_EXTRA_LENGTH * 3) + DEVICE_ADDRESS_LENGTH
#define CAPTURING_DEVICE_DATA_MAX_LEN (sizeof(device_d_t) * 3) + sizeof(device_d_t)

#define SYSTEM_VAR_DS18B20 1
#define SYSTEM_VAR_FIRST_CAPTURE_READ 2
#define SYSTEM_VAR_FIRST_CAPTURE_DEVICE 4

#define INTERLOOP_DELAY (systemState == SYSTEM_STATE_CAPTURING ? INTERLOOP_DELAY_CAPTURING : INTERLOOP_DELAY_IDLE)
#define isExtensionDeviceBus(b) ((b) == DEVICE_BUS_EXTENSION_DEVICE_ANALOG || (b) == DEVICE_BUS_EXTENSION_DEVICE_DIGITAL ? 1 : 0)
#define isAnalogOrDigitalPin(t, b) ((t) == DEVICE_TYPE_PIN && ((b) == DEVICE_BUS_ANALOG || (b) == DEVICE_BUS_DIGITAL) ? 1 : 0)
#define deviceExtraOffset(i) (EEPROM_DEVICES_EXTRA_OFFSET + (DEVICE_EXTRA_LENGTH * (i - 1)))

typedef struct
{
  uint8_t address;
  uint8_t number;
} pin_device_t;

typedef uint8_t device_d_t[4];

typedef struct
{
  uint8_t flags;
  uint8_t bus;
  uint8_t type;
  char *name;
  uint8_t *address;
  uint8_t *extra;
  device_d_t data;
  uint8_t extraBlock;
} device_t;

const PROGMEM uint8_t DeviceOwBusAddress[] = {OW_PIN_ADDRESS};
const PROGMEM device_t DeviceOwBus = {
    DEVICE_FLAG_ENABLED | DEVICE_FLAG_SYSTEM | DEVICE_FLAG_NO_DATA,
    DEVICE_BUS_DIGITAL,
    DEVICE_TYPE_PIN,
    "OneWire pin",
    (uint8_t *)&DeviceOwBusAddress};

const PROGMEM uint8_t DeviceI2CDataPinAddress[] = {I2C_D_PIN_ADDRESS};
const PROGMEM device_t DeviceI2CDataPin = {
    DEVICE_FLAG_ENABLED | DEVICE_FLAG_SYSTEM | DEVICE_FLAG_NO_DATA,
    DEVICE_BUS_DIGITAL,
    DEVICE_TYPE_PIN,
    "I2C data pin",
    (uint8_t *)&DeviceI2CDataPinAddress};

const PROGMEM uint8_t DeviceI2CClockPinAddress[] = {I2C_C_PIN_ADDRESS};
const PROGMEM device_t DeviceI2CClockPin = {
    DEVICE_FLAG_ENABLED | DEVICE_FLAG_SYSTEM | DEVICE_FLAG_NO_DATA,
    DEVICE_BUS_DIGITAL,
    DEVICE_TYPE_PIN,
    "I2C clock pin",
    (uint8_t *)&DeviceI2CClockPinAddress};

const PROGMEM uint8_t DeviceSpiClkPinAddress[] = {SPI_CLK_PIN_ADDRESS};
const PROGMEM device_t DeviceSpiClkPin = {
    DEVICE_FLAG_ENABLED | DEVICE_FLAG_SYSTEM | DEVICE_FLAG_NO_DATA,
    DEVICE_BUS_DIGITAL,
    DEVICE_TYPE_PIN,
    "SPI CLK pin",
    (uint8_t *)&DeviceSpiClkPinAddress};

const PROGMEM uint8_t DeviceSpiMisoPinAddress[] = {SPI_MISO_PIN_ADDRESS};
const PROGMEM device_t DeviceSpiMisoPin = {
    DEVICE_FLAG_ENABLED | DEVICE_FLAG_SYSTEM | DEVICE_FLAG_NO_DATA,
    DEVICE_BUS_DIGITAL,
    DEVICE_TYPE_PIN,
    "SPI MISO pin",
    (uint8_t *)&DeviceSpiMisoPinAddress};

const PROGMEM uint8_t DeviceSpiMosiPinAddress[] = {SPI_MOSI_PIN_ADDRESS};
const PROGMEM device_t DeviceSpiMosiPin = {
    DEVICE_FLAG_ENABLED | DEVICE_FLAG_SYSTEM | DEVICE_FLAG_NO_DATA,
    DEVICE_BUS_DIGITAL,
    DEVICE_TYPE_PIN,
    "SPI MOSI pin",
    (uint8_t *)&DeviceSpiMosiPinAddress};

const PROGMEM uint8_t DeviceSpiCsPinAddress[] = {SPI_CS_PIN_ADDRESS};
const PROGMEM device_t DeviceSpiCsPin = {
    DEVICE_FLAG_ENABLED | DEVICE_FLAG_SYSTEM | DEVICE_FLAG_NO_DATA,
    DEVICE_BUS_DIGITAL,
    DEVICE_TYPE_PIN,
    "SPI CS pin",
    (uint8_t *)&DeviceSpiCsPinAddress};

const PROGMEM device_t DeviceSystemTime = {
    DEVICE_FLAG_ENABLED | DEVICE_FLAG_SYSTEM | DEVICE_FLAG_NO_DATA,
    DEVICE_BUS_I2C,
    DEVICE_TYPE_DS3231_TIME,
    "System time"};
const PROGMEM device_t DeviceSystemTemperature = {
    DEVICE_FLAG_ENABLED | DEVICE_FLAG_SYSTEM,
    DEVICE_BUS_I2C,
    DEVICE_TYPE_DS3231_TEMPERATURE,
    "System temperature",
    0,
    0,
    {0, 0, 0, 0},
    0};
const PROGMEM device_t DeviceExtensionBoard = {
    DEVICE_FLAG_ENABLED | DEVICE_FLAG_SYSTEM | DEVICE_FLAG_NO_DATA,
    DEVICE_BUS_I2C,
    DEVICE_TYPE_EXTENSION_BOARD,
    "Extension board NANO"};

const PROGMEM uint8_t DeviceExtensionBoardResetPinAddress[] = {EXTENSION_BOARD_RESET_PIN_ADDRESS};
const PROGMEM device_t DeviceExtensionBoardResetPin = {
    DEVICE_FLAG_ENABLED | DEVICE_FLAG_SYSTEM | DEVICE_FLAG_OUTPUT | DEVICE_FLAG_NO_DATA,
    DEVICE_BUS_DIGITAL,
    DEVICE_TYPE_PIN,
    "Extension board RST pin",
    (uint8_t *)&DeviceExtensionBoardResetPinAddress};

const PROGMEM uint8_t DeviceAcs712Address[] = {ACS712_PIN_ADDRESS};
const PROGMEM device_t DeviceAcs712 = {
    DEVICE_FLAG_ENABLED | DEVICE_FLAG_SYSTEM,
    DEVICE_BUS_EXTENSION_DEVICE_ANALOG,
    DEVICE_TYPE_PIN,
    "ACS712 5A",
    (uint8_t *)&DeviceAcs712Address,
    0,
    {0, 0, 0, 0},
    1};

const device_t *const SystemDevices[] PROGMEM = {
    &DeviceOwBus,
    &DeviceI2CDataPin,
    &DeviceI2CClockPin,
    &DeviceSpiClkPin,
    &DeviceSpiMisoPin,
    &DeviceSpiMosiPin,
    &DeviceSpiCsPin,
    &DeviceSystemTime,
    &DeviceSystemTemperature,
    &DeviceExtensionBoard,
    &DeviceExtensionBoardResetPin,
    &DeviceAcs712};

const PROGMEM pin_device_t AnalogPins[] = {{0, A0}};
const PROGMEM pin_device_t DigitalPins[] = {
    {0, D0},
    {1, D1},
    {2, D2},
    {3, D3},
    {4, D4},
    {5, D5},
    {6, D6},
    {7, D7},
    {8, D8}};

const PROGMEM char FileUploadFailedText[] = "Could NOT Upload file!";
const PROGMEM char ContentTypeTextHtml[] = "text/html";
const PROGMEM char ContentTypeTextPlain[] = "text/plain";
const PROGMEM char UploadHtml[] = "<!doctype html>"
                                  "<html lang=\"en\">"
                                  "<head>"
                                  "<title>LAB Board File Upload</title>"
                                  "</head>"
                                  "<body>"
                                  "<center>"
                                  "<form method=\"POST\" enctype=\"multipart/form-data\">"
                                  "<input type=\"file\" name=\"file\">"
                                  "<input type=\"submit\" value=\"Upload\">"
                                  "</form>"
                                  "</center>"
                                  "</body>"
                                  "</html>";
const PROGMEM char UploadSuccessHtml[] = "<!doctype html>"
                                         "<html lang=\"en\">"
                                         "<head>"
                                         "<title>LAB Board File Upload</title>"
                                         "</head>"
                                         "<body>"
                                         "<center>"
                                         "<h3>"
                                         "Upload Success! <a href=\"/upload\">Upload page</a>"
                                         "</h3>"
                                         "</center>"
                                         "</body>"
                                         "</html>";
const PROGMEM char NotFoundHtml[] = "<!doctype html>"
                                    "<html lang=\"en\">"
                                    "<head>"
                                    "<title>LAB Board File | NOT FOUND</title>"
                                    "</head>"
                                    "<body>"
                                    "<center>"
                                    "<h3>"
                                    "Not found!"
                                    "</h3>"
                                    "</center>"
                                    "</body>"
                                    "</html>";

RtcDS3231<TwoWire> rtcObject(Wire);

OneWire *oneWire;
DallasTemperature *ds18b20;

ESP8266WebServer webServer(WEB_SERVER_PORT);
WebSocketsServer webSocketServer(WEBSOCKET_SERVER_PORT);
uint8_t systemOptions = 0;

uint8_t systemState;
uint32_t lastStateTimestamp;
uint32_t millisInitOffset;

typedef uint8_t subscribers_t[WEBSOCKETS_SERVER_CLIENT_MAX];

subscribers_t subscribers[4];

File fsUploadFile;
File fsDataFile;

typedef struct __attribute__((packed))
{
  uint8_t flags;
  uint8_t bus;
  uint8_t type;
  char name[DEVICE_NAME_LENGTH];
  uint8_t address[DEVICE_ADDRESS_LENGTH];
  uint8_t extraBlock;
} stored_device_t;

typedef struct message_in
{
  const uint8_t number;
  const uint8_t subject;
  const uint16_t command;
  const uint8_t length;
  const uint8_t *data;

  message_in(const uint8_t num, const uint8_t *payload, size_t len)
      : number(num),
        subject(payload[0]),
        command(payload[1] + (payload[2] << 8)),
        length(len - (sizeof(number) + sizeof(command))),
        data(&payload[3])
  {
  }
} message_in_t;

typedef struct __attribute__((packed))
{
  uint8_t subject;
  uint16_t command;
  uint8_t data[MSG_DATA_MAX_LEN];
} message_out_t;

uint32_t ds18b20Requested = 0;

uint8_t deviceCount;
device_t **devices;
uint8_t systemVars = 0;
char dataStr[CAPTURING_DEVICE_ADDRESS_MAX_LEN];

message_out_t *messageOut = new message_out_t;
uint8_t extensionBoardMessageIn[MSG_EXTENSION_BOARD_MAX_LEN];
uint8_t extensionBoardMessageOut[MSG_EXTENSION_BOARD_MAX_LEN];
uint8_t *extensionDevicesIndexes;
uint8_t extensionDevicesCount = 0;

void setup()
{
  pinMode(EXTENSION_BOARD_RESET_PIN, OUTPUT);

  Serial.begin(SERIAL_BAUD_RATE);
  // Move down from abraquadabraz
  Serial.println();
  Serial.println();

  if (!SPIFFS.begin())
  {
    Serial.println(F("Could not initialize SPIFFS!"));
  }

  readDevices();

  // Initialize extension device bus

  oneWire = new OneWire(OW_PIN);
  ds18b20 = new DallasTemperature(oneWire);
  ds18b20->begin();
  ds18b20->setResolution(DS18B20_RESOLUTION); // Should be taken from user configurable source
  ds18b20->setWaitForConversion(false);

  Wire.pins(I2C_D_PIN, I2C_C_PIN);
  rtcObject.Begin();

  if (SD.begin(SPI_CLK_PIN))
  {
    systemOptions |= SYSTEM_OPTION_SD;
    Serial.println(F("SD card was initialized!"));
  }
  else
  {
    Serial.println(F("SD card was NOT initialized!"));
  }

  if (systemOptions & SYSTEM_OPTION_SD)
  {
    wifiBootstrap();
  }

  wifiStart();

  setupWebServer();
  setupWebSockets();

  resetExtensionBoard();

  millisInitOffset = millis();
  setSystemState(SYSTEM_STATE_IDLE);
}

uint32_t loopStart;
uint32_t loopEnd;

void loop()
{
  loopStart = millis();

  readInputs();

  if (systemState != SYSTEM_STATE_CAPTURING && systemState != SYSTEM_STATE_RESTARTING && Serial.available())
  {
    handleSerial();
  }

  if (wifiConnected())
  {
    if (anySubscriber(MSG_SBJ_MONITORING))
    {
      sendSourceData(WEB_SOCKET_NUMBER_BROADCAST_MONITORING, loopStart - millisInitOffset, SOURCE_DATA_INDEX_IGNORE);
    }

    webSocketServer.loop();
    if (systemState != SYSTEM_STATE_RESTARTING)
    {
      webServer.handleClient();
    }
  }

  // Restart after web socket loop processed messages
  if (systemState == SYSTEM_STATE_RESTARTING && (millis() - lastStateTimestamp >= RESTART_DELAY))
  {
    webSocketServer.disconnect();
    webSocketServer.close();
    ESP.reset();
  }

  loopEnd = millis();

  if (loopEnd - loopStart <= INTERLOOP_DELAY)
  {
    delay(INTERLOOP_DELAY - (loopEnd - loopStart));
  }
}

void readDevices()
{
  deviceCount = DEVICE_SYSTEM_COUNT;

  EEPROM.begin(EEPROM_DEVICES_BUFFER);
  uint8_t blockIndexes[DEVICE_MAX_COUNT];
  device_t *device;
  stored_device_t storedDevice;
  uint8_t nameLength;
  uint8_t addressLength;
  uint8_t extra[DEVICE_EXTRA_LENGTH];

  for (uint8_t index = 0; index < DEVICE_MAX_COUNT; index++)
  {
    blockIndexes[index] = EEPROM.read(EEPROM_DEVICES_OFFSET + index);
    if (blockIndexes[index])
    {
      deviceCount++;
    }
  }
  devices = new device_t *[deviceCount];

  // Initialize system devices
  for (uint8_t systemDeviceIndex = 0; systemDeviceIndex < DEVICE_SYSTEM_COUNT; systemDeviceIndex++)
  {
    device_t deviceP;
    memset(&deviceP, 0, sizeof(deviceP));
    device = new device_t;

    memcpy_P(&deviceP, SystemDevices[systemDeviceIndex], sizeof(deviceP));
    device->flags = deviceP.flags;
    device->bus = deviceP.bus;
    device->type = deviceP.type;
    device->extraBlock = deviceP.extraBlock;

    device->name = new char[strlen_P(deviceP.name) + 1];
    strcpy_P(device->name, deviceP.name);

    uint8_t addressLength = getDeviceAddressLength(deviceP.bus, deviceP.type);
    if (addressLength)
    {
      device->address = new uint8_t[addressLength];
      memcpy_P(device->address, deviceP.address, addressLength);
    }

    devices[systemDeviceIndex] = device;

    if (device->flags & DEVICE_FLAG_NO_DATA)
    {
      continue;
    }

    EEPROM.get(deviceExtraOffset(device->extraBlock), extra);
    device->extra = new uint8_t[DEVICE_EXTRA_LENGTH];
    memcpy(device->extra, &extra, sizeof(extra));

    memset(&device->data, 0, sizeof(device_d_t));

    if (isExtensionDeviceBus(device->bus))
    {
      extensionDevicesCount++;
    }
  }

  // Initialize user-defined devices
  uint8_t deviceIndex = DEVICE_SYSTEM_COUNT;
  for (uint8_t blockIndex = 0; blockIndex < DEVICE_MAX_COUNT; blockIndex++)
  {
    if (!blockIndexes[blockIndex])
    {
      continue;
    }

    EEPROM.get(EEPROM_DEVICES_OFFSET + DEVICE_MAX_COUNT + DEVICE_LENGTH * (blockIndexes[blockIndex] - 1), storedDevice);

    nameLength = strlen(storedDevice.name);
    device = new device_t;
    device->bus = storedDevice.bus;
    device->type = storedDevice.type;
    device->name = new char[nameLength + 1];
    device->flags = storedDevice.flags | DEVICE_FLAG_ACTIVE;
    strcpy(device->name, (const char *)&storedDevice.name);

    device->extraBlock = storedDevice.extraBlock;
    EEPROM.get(deviceExtraOffset(device->extraBlock), extra);

    device->extra = new uint8_t[DEVICE_EXTRA_LENGTH];
    memcpy(device->extra, &extra, sizeof(extra));

    addressLength = getDeviceAddressLength(storedDevice.bus, storedDevice.type);
    if (addressLength)
    {
      device->address = new uint8_t[addressLength];
      memcpy(device->address, &storedDevice.address, addressLength);
    }

    memset(&device->data, 0, sizeof(device_d_t));

    devices[deviceIndex] = device;
    deviceIndex++;

    if (isExtensionDeviceBus(device->bus))
    {
      extensionDevicesCount++;
    }
    else if (isAnalogOrDigitalPin(device->type, device->bus))
    {
      pinMode(getPinHardwareNumber(device->address[0], device->bus), device->flags & DEVICE_FLAG_OUTPUT ? OUTPUT : INPUT);
    }
  }

  EEPROM.end();

  // Put extension devices indexes
  if (extensionDevicesCount > 0)
  {
    extensionDevicesIndexes = new uint8_t[extensionDevicesCount];
    extensionDevicesCount = 0;
    for (deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++)
    {
      if (isExtensionDeviceBus(devices[deviceIndex]->bus))
      {
        extensionDevicesIndexes[extensionDevicesCount++] = deviceIndex;
      }
    }
  }
}

void wifiBootstrap()
{
  String fileName = "/wifi.txt";
  if (!SD.exists(fileName))
  {
    return;
  }

  File file = SD.open(fileName, 0x01);

  char ssid[WIFI_SSID_MAX_LEN];
  memset(&ssid, 0, sizeof(ssid));
  char pwd[WIFI_PWD_MAX_LEN];
  memset(&pwd, 0, sizeof(pwd));
  char *buffer = (char *)&ssid;
  char readChar;
  uint8_t bufferPosition;
  while (file.available())
  {
    readChar = file.read();
    switch (readChar)
    {
    case '\r':
    case '\n':
      buffer = (char *)&pwd;
      break;
    default:
      buffer[0] = readChar;
      buffer++;
      break;
    }
  }

  file.close();

  wifiWriteSsid((const char *)&ssid);
  wifiWritePwd((const char *)&pwd);

  SD.remove(fileName);
}

void resetExtensionBoard()
{
  digitalWrite(EXTENSION_BOARD_RESET_PIN, LOW);
  delay(EXTENSION_BOARD_RESET_DELAY);
  digitalWrite(EXTENSION_BOARD_RESET_PIN, HIGH);
  delay(EXTENSION_BOARD_WARMUP_DELAY);

  device_t *device;

  for (uint8_t deviceIndex = 0; deviceIndex < extensionDevicesCount; deviceIndex++)
  {
    device = devices[extensionDevicesIndexes[deviceIndex]];

    extensionBoardMessageOut[0] = MSG_CMD_MANAGEMENT_REGISTERED_DEVICE;
    extensionBoardMessageOut[1] = deviceIndex;
    extensionBoardMessageOut[2] = device->flags;
    extensionBoardMessageOut[3] = device->bus;
    extensionBoardMessageOut[4] = device->type;
    extensionBoardMessageOut[5] = device->address[0];

    sendExtensionBoardMessage();
  }
}

uint8_t getExtensionDeviceIndex(uint8_t localDeviceIndex)
{
  uint8_t deviceIndex = 0;
  while (extensionDevicesIndexes[deviceIndex] != localDeviceIndex)
  {
    deviceIndex++;
  }
  return deviceIndex;
}

uint8_t getDeviceAddressLength(uint8_t bus, uint8_t type)
{
  switch (bus)
  {
  case DEVICE_BUS_ANALOG:
  case DEVICE_BUS_DIGITAL:
  case DEVICE_BUS_EXTENSION_DEVICE_ANALOG:
  case DEVICE_BUS_EXTENSION_DEVICE_DIGITAL:
    return 1;
  case DEVICE_BUS_OW:
    return sizeof(DeviceAddress);
  default:
    return 0;
  }
}

void handleSerial()
{
  uint8_t wifiRestart = 0;
  String command = Serial.readStringUntil(':');
  String value = Serial.readStringUntil('\n');
  if (command == "restart" && value == "YES")
  {
    restart();
  }
  else if (command == "devices_clear" && value == "YES")
  {
    EEPROM.begin(EEPROM_DEVICES_BUFFER);
    for (uint8_t index = 0; index < DEVICE_MAX_COUNT; index++)
    {
      EEPROM.write(EEPROM_DEVICES_OFFSET + index, 0);
    }
    EEPROM.end();
  }
  else if (command == "wifi_ssid")
  {
    wifiRestart = 1;
    wifiWriteSsid(value.c_str());
  }
  else if (command == "wifi_pwd")
  {
    wifiRestart = 1;
    wifiWritePwd(value.c_str());
  }
  else if (command == "wifi_clear" && value == "YES")
  {
    wifiRestart = 1;
    wifiWriteSsid("");
    wifiWritePwd("");
  }
  else if (command == "wifi_rst" && value == "YES")
  {
    wifiRestart = 1;
  }
  else if (command == "spiffs_frmt" && value == "YES")
  {
    Serial.println(F("Formatting SPIFFS..."));
    if (SPIFFS.format())
    {
      Serial.println(F("SPIFFS formatting complete."));
    }
    else
    {
      Serial.println(F("SPIFFS formatting failed!"));
    }
  }
  else if (command == "spiffs_lst")
  {
    Dir dir = SPIFFS.openDir(value);
    while (dir.next())
    {
      Serial.print(dir.fileName());
      Serial.print("\t");
      Serial.println(dir.fileSize());
    }
  }
  else if (command == "spiffs_rmv")
  {
    if (SPIFFS.exists(value))
    {
      SPIFFS.remove(value);
    }
  }
  else if (command == "spiffs_mv")
  {
    String originalName = value.substring(0, value.indexOf(" "));
    String newName = value.substring(value.indexOf(" ") + 1);
    if (SPIFFS.exists(originalName))
    {
      SPIFFS.rename(originalName, newName);
    }
  }
  else if (command == "date_get")
  {
    RtcDateTime currentTime = rtcObject.GetDateTime();
    uint32_t seconds = currentTime.TotalSeconds();

    Serial.printf_P(PSTR("UTC date: %d/%d/%d %d:%d:%d (%d seconds from 2k)\r\n"),
                    currentTime.Year(),
                    currentTime.Month(),
                    currentTime.Day(),
                    currentTime.Hour(),
                    currentTime.Minute(),
                    currentTime.Second(),
                    seconds);
  }
  else if (command == "date_set")
  {
    RtcDateTime newDate((uint16_t)value.substring(0, 4).toInt(),
                        (uint8_t)value.substring(5, 7).toInt(),
                        (uint8_t)value.substring(8, 10).toInt(),
                        (uint8_t)value.substring(11, 13).toInt(),
                        (uint8_t)value.substring(14, 16).toInt(),
                        (uint8_t)value.substring(17, 19).toInt());
    rtcObject.SetDateTime(newDate);
  }
  else if (command == "temp_get")
  {
    RtcTemperature environmentTemperature = rtcObject.GetTemperature();
    int16_t temp = environmentTemperature.AsCentiDegC();
    Serial.printf_P(PSTR("Environment temp.: %4.2fC\r\n"), temp / 100.0f);
  }
#ifdef DEBUG
  else if (command == "subscribers")
  {
    Serial.println(F("Subscribers:"));    
    for (uint8_t subject = 0; subject < sizeof(subscribers) / sizeof(subscribers_t); subject++)
    {
      Serial.printf_P(PSTR("#%d:"), subject);    
      for (uint8_t clientIndex = 0; clientIndex < sizeof(subscribers_t); clientIndex++)
      {       
        if (subscribers[subject][clientIndex])
        {
          Serial.printf_P(PSTR(" %d"), subject);
        }
      }
      Serial.println();
    }
  }
  else if (command == "set_system_state")
  {
    uint8_t newState;
    if (value == "capturing")
    {
      newState = SYSTEM_STATE_CAPTURING;
    }
    else if (value == "idle")
    {
      newState = SYSTEM_STATE_IDLE;
    }
    else
    {
      return;
    }

    setSystemState(newState);
    Serial.printf_P(PSTR("State changed to %d\r\n"), systemState);
  }
#endif

  if (wifiRestart)
  {
    Serial.println(F("WiFi credentials were changed, rest arting..."));
    wifiStart();
  }
}

void restart()
{
  setSystemState(SYSTEM_STATE_RESTARTING);
  sendSystemState(WEB_SOCKET_NUMBER_BROADCAST);
}

void sendSystemState(uint8_t number)
{
  uint32_t timestamp = lastStateTimestamp - millisInitOffset;
  resetMessageOut(MSG_SBJ_BOARD, MSG_CMD_BOARD_STATE);
  messageOut->data[0] = systemState;
  messageOut->data[1] = INTERLOOP_DELAY;
  memcpy(&messageOut->data[2], &timestamp, sizeof(timestamp));
  sendBin(number, sizeof(messageOut->subject) + sizeof(messageOut->command) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(timestamp));
}

void resetMessageOut(uint8_t subject, uint16_t command)
{
  messageOut->subject = subject;
  messageOut->command = command;
}

void setSystemState(uint8_t state)
{
  device_t *device;
  uint8_t turnOffCapturing = 0;
  if (systemState == SYSTEM_STATE_CAPTURING)
  {
    turnOffCapturing = 1;
  }
  systemState = state;
  lastStateTimestamp = millis();

  if (state == SYSTEM_STATE_CAPTURING)
  {
    systemVars |= SYSTEM_VAR_FIRST_CAPTURE_READ;

    RtcDateTime currentTime = rtcObject.GetDateTime();
    uint32_t seconds = currentTime.TotalSeconds();
    uint8_t first = 1;

    char *fileName = new char[CAPTURING_FILE_NAME_MAX_LEN];
    sprintf_P(fileName,
              PSTR("/lab_board/%d_%d_%d_%d_%d_%d.json"),
              currentTime.Year(),
              currentTime.Month(),
              currentTime.Day(),
              currentTime.Hour(),
              currentTime.Minute(),
              currentTime.Second());
    fsDataFile = SD.open(fileName, 0x10 | 0x02);
    delete fileName;

    fsDataFile.printf_P(PSTR("{\"started\":%d,\"period\":%d,"), lastStateTimestamp, INTERLOOP_DELAY_CAPTURING);
    fsDataFile.print(F("\"devices\":["));

    uint8_t addressLength;
    char addressStr[CAPTURING_DEVICE_ADDRESS_MAX_LEN];
    char extraStr[CAPTURING_DEVICE_EXTRA_MAX_LEN];
    for (uint8_t deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++)
    {
      device = devices[deviceIndex];

      if (!(device->flags & DEVICE_FLAG_ENABLED) || (device->flags & DEVICE_FLAG_NO_DATA))
      {
        continue;
      }

      if (!(device->flags & DEVICE_FLAG_SYSTEM) && (device->flags & DEVICE_FLAG_OUTPUT) && device->type == DEVICE_TYPE_PIN)
      {
        writeDevice(deviceIndex, 1);
        continue;
      }

      addressLength = getDeviceAddressLength(device->bus, device->type);
      memset(&addressStr, 0, sizeof(addressStr));
      if (addressLength)
      {
        fillFromArray((char *)&addressStr, device->address, addressLength);
      }
      memset(&dataStr, 0, sizeof(dataStr));
      fillFromArray((char *)&dataStr, (const uint8_t *)&device->data, sizeof(device_d_t));

      memset(&extraStr, 0, sizeof(extraStr));
      fillFromArray((char *)&extraStr, (const uint8_t *)device->extra, DEVICE_EXTRA_LENGTH);

      fsDataFile.printf_P(PSTR("%s{\"index\":%d,\"flags\":%d,\"bus\":%d,\"type\":%d,\"name\":\"%s\",\"address\":[%s],\"extra\":[%s],\"data\":[%s]}\r\n"),
                          first ? "" : ",",
                          deviceIndex,
                          device->flags,
                          device->bus,
                          device->type,
                          device->name,
                          addressStr,
                          extraStr,
                          dataStr);
      first = 0;
    }
    fsDataFile.print(F("],\"data\":["));
    fsDataFile.flush();
  }
  else if (turnOffCapturing)
  {
    for (uint8_t deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++)
    {
      device = devices[deviceIndex];

      if (!(device->flags & DEVICE_FLAG_SYSTEM) && (device->flags & DEVICE_FLAG_ENABLED) && (device->flags & DEVICE_FLAG_OUTPUT) && device->type == DEVICE_TYPE_PIN)
      {
        writeDevice(deviceIndex, 0);
      }
    }

    fsDataFile.printf_P(PSTR("],\"finished\":%d}"), lastStateTimestamp);
    fsDataFile.flush();
    fsDataFile.close();
  }
}

void writeDevice(const uint8_t deviceIndex, const uint8_t value)
{
  device_t *device = devices[deviceIndex];

  if (isExtensionDeviceBus(device->bus))
  {
    extensionBoardMessageOut[0] = MSG_CMD_MANAGEMENT_DEVICE_VALUE;
    extensionBoardMessageOut[1] = getExtensionDeviceIndex(deviceIndex);
    extensionBoardMessageOut[2] = value;
    sendExtensionBoardMessage();
    return;
  }

  if (device->bus == DEVICE_BUS_ANALOG)
  {
    analogWrite(getPinHardwareNumber(device->address[0], DEVICE_BUS_ANALOG), value);
  }
  else if (device->bus == DEVICE_BUS_DIGITAL)
  {
    digitalWrite(getPinHardwareNumber(device->address[0], DEVICE_BUS_DIGITAL), value);
  }
  device->data[0] = value;
}

void fillFromArray(char *dest, const uint8_t *source, size_t length)
{
  for (uint8_t byteIndex = 0; byteIndex < length; byteIndex++)
  {
    dest += sprintf(dest, "%d%s", source[byteIndex], byteIndex == length - 1 ? "" : ",");
  }
}

void writeFlash(const char *value, uint8_t offset, uint8_t size)
{
  EEPROM.begin(offset + size);
  for (uint8_t index = 0; index < size; index++)
  {
    EEPROM.write(offset + index, value[index]);
  }
  EEPROM.end();
}

void wifiWriteSsid(const char *ssid)
{
  writeFlash(ssid, EEPROM_WIFI_OFFSET, WIFI_SSID_MAX_LEN);
}

void wifiWritePwd(const char *pwd)
{
  writeFlash(pwd, EEPROM_WIFI_OFFSET + WIFI_SSID_MAX_LEN, WIFI_SSID_MAX_LEN + WIFI_PWD_MAX_LEN);
}

void sendSourceData(uint8_t number, uint32_t timestamp, uint8_t index)
{
  resetMessageOut(MSG_SBJ_MONITORING, MSG_CMD_MONITORING_SOURCE_DATA);

  uint8_t inputsUpdated = 0;
  uint8_t dataOffset;
  device_t *device;
  for (uint8_t deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++)
  {
    device = devices[deviceIndex];
    if (!(device->flags & DEVICE_FLAG_ENABLED) || (device->flags & (DEVICE_FLAG_NO_DATA | DEVICE_FLAG_MUTED)) || !((device->flags & DEVICE_FLAG_SYNC) || deviceIndex == index))
    {
      continue;
    }
    dataOffset = inputsUpdated * (sizeof(device_d_t) + 1) + sizeof(timestamp);
    messageOut->data[dataOffset + 1] = deviceIndex | (device->flags & DEVICE_FLAG_MONIRORING_MASK);
    memcpy(&messageOut->data[dataOffset + 2], &device->data, sizeof(device_d_t));
    inputsUpdated++;

    if (index == SOURCE_DATA_INDEX_IGNORE)
    {
      device->flags &= DEVICE_FLAG_SYNC_MASK_CLEAR;
    }
  }

  memcpy(&messageOut->data, &timestamp, sizeof(timestamp));
  messageOut->data[sizeof(timestamp)] = inputsUpdated;
  size_t length = sizeof(timestamp) + sizeof(uint8_t) + (sizeof(device_d_t) + 1) * inputsUpdated;
  sendBin(number, length + 3);
}

void wifiStart()
{
  WiFi.disconnect();

  char wifiSsid[WIFI_SSID_MAX_LEN + 1];
  char wifiPwd[WIFI_PWD_MAX_LEN + 1];
  EEPROM.begin(EEPROM_WIFI_BUFFER);
  for (uint8_t index = 0; index < WIFI_SSID_MAX_LEN && (wifiSsid[index] = EEPROM.read(EEPROM_WIFI_OFFSET + index)) != 0; index++)
    ;
  for (uint8_t index = 0; index < WIFI_PWD_MAX_LEN && (wifiPwd[index] = EEPROM.read(EEPROM_WIFI_OFFSET + WIFI_SSID_MAX_LEN + index)) != 0; index++)
    ;
  EEPROM.end();

  if (strlen(wifiSsid) && strlen(wifiPwd))
  {
#ifdef DEBUG
    Serial.print(F("WiFi SSID "));
    Serial.println(wifiSsid);
#endif
    systemOptions |= SYSTEM_OPTION_WIFI;
    WiFi.begin(wifiSsid, wifiPwd);
  }
  else
  {
    systemOptions &= (0xFF ^ SYSTEM_OPTION_WIFI);
#ifdef DEBUG
    Serial.println(F("WiFi credentials are not complete!"));
#endif
  }
}

uint8_t wifiConnected()
{
  if (!(systemOptions & SYSTEM_OPTION_WIFI))
  {
    return 0;
  }

  uint8_t wifiConnected = WiFi.status() == WL_CONNECTED ? 1 : 0;

  if (!wifiConnected)
  {
    systemOptions &= 0xFF ^ SYSTEM_OPTION_WEB;
    webServer.close();
  }

  uint8_t attempt = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    if (attempt++ == WIFI_RECONNECT_ATTEMPTS)
    {
      systemOptions &= (0xFF ^ SYSTEM_OPTION_WIFI);
      return 0;
    }
#ifdef DEBUG
    Serial.print(F("WiFi connection attempt "));
    Serial.print(attempt);
    Serial.println(F("..."));
#endif
    delay(WIFI_RECONNECT_DELAY);
  }

  if (!wifiConnected)
  {
#ifdef DEBUG
    Serial.println();
    Serial.println(F("WiFi connected."));
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());
#endif
    webServer.begin();
  }

  return 1;
}

void setupWebServer()
{
  webServer.on("/upload", HTTP_GET, []()
               { webServer.send_P(200, (const char *)&ContentTypeTextHtml, (const char *)&UploadHtml); });

  webServer.on(
      "/files", HTTP_DELETE,
      []()
      {
        Dir dir = SPIFFS.openDir("");
        while (dir.next())
        {
          SPIFFS.remove(dir.fileName());
        }
        webServer.send(200);
      });

  webServer.on(
      "/upload", HTTP_POST, []()
      { webServer.send(200); },
      handleWebServerUpload);

  webServer.on("/data-file", HTTP_GET, []()
               {
    if (systemState != SYSTEM_STATE_IDLE || !webServer.hasArg("name") || !strlen(webServer.arg("name").c_str())) {
      webServer.send(400);
    }

    char fileName[CAPTURING_FILE_NAME_MAX_LEN];

    sprintf_P((char *)&fileName, PSTR("/lab_board/%s"), webServer.arg("name").c_str());

    if (!SD.exists((const char *)&fileName)) {
      webServer.send(404);
    }

#ifdef DEBUG
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
#endif

    fsDataFile = SD.open((const char *)&fileName, 0x01);
    webServer.streamFile(fsDataFile, "application/json");
    fsDataFile.close(); });

  webServer.onNotFound([]()
                       {
    if (!handleWebServerGetFile(webServer.uri())) {
      webServer.send_P(HTTP_NOT_FOUND, (const char *)&ContentTypeTextHtml, (const char *)&NotFoundHtml);
    } });
}

void handleWebServerUpload()
{
  HTTPUpload &upload = webServer.upload();
  if (upload.status == UPLOAD_FILE_START)
  {
    upload.filename.toLowerCase();
    fsUploadFile = SPIFFS.open("/" + upload.filename, "w");
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    fsUploadFile.write(upload.buf, upload.currentSize);
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    fsUploadFile.flush();
    fsUploadFile.close();
    webServer.send_P(HTTP_CREATED, (const char *)&ContentTypeTextHtml, (const char *)&UploadSuccessHtml);
  }
  else if (upload.status == UPLOAD_FILE_ABORTED)
  {
    fsUploadFile.close();
    webServer.send_P(HTTP_SERVER_ERROR, (const char *)&ContentTypeTextPlain, (const char *)&FileUploadFailedText);
  }
}

String getContentType(const String filename)
{
  if (filename.endsWith(".html"))
  {
    return "text/html";
  }
  else if (filename.endsWith(".css"))
  {
    return "text/css";
  }
  else if (filename.endsWith(".js"))
  {
    return "application/javascript";
  }
  else if (filename.endsWith(".ico"))
  {
    return "image/x-icon";
  }

  return "text/plain";
}

uint8_t handleWebServerGetFile(String path)
{
  if (path.endsWith("/"))
  {
    path += "index.html";
  }
  else
  {
    path.toLowerCase();
  }

  if (!SPIFFS.exists(path))
  {
    return 0;
  }

  File file = SPIFFS.open(path, "r");
  size_t sent = webServer.streamFile(file, getContentType(path));
  file.close();

  return 1;
}

void setupWebSockets()
{
  webSocketServer.begin();
  webSocketServer.onEvent(webSocketEvent);
}

void clearSubscriberSubjects(uint8_t number)
{
  for (uint8_t subject = 0; subject < sizeof(subscribers) / sizeof(subscribers_t); subject++)
  {
    subscribers[subject][number] = 0;
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  IPAddress ip;
  switch (type)
  {
  case WStype_DISCONNECTED:
    clearSubscriberSubjects(num);
#ifdef DEBUG
    Serial.printf_P(PSTR("[%u] Disconnected!\r\n"), num);
#endif
    break;
  case WStype_CONNECTED:
    clearSubscriberSubjects(num);
#ifdef DEBUG
    ip = webSocketServer.remoteIP(num);
    Serial.printf_P(PSTR("[%u] Connected from %d.%d.%d.%d url: %s\r\n"), num, ip[0], ip[1], ip[2], ip[3], payload);
#endif
    break;
  case WStype_TEXT:
#ifdef DEBUG
    Serial.printf_P(PSTR("[%u] get Text: %s\r\n"), num, payload);
#endif
    break;
  case WStype_BIN:
#ifdef DEBUG
    Serial.printf_P(PSTR("[%u] get Bin [%u]\r\n"), num, length);
#endif
    message_in_t message(num, payload, length);
    handleWebSocketMessage(&message);
    break;
  }
}

void handleWebSocketMessage(const message_in_t *message)
{
#ifdef DEBUG
  Serial.printf_P(PSTR("Subject: %u, command: %u\r\n"), message->subject, message->command);
#endif
  switch (message->subject)
  {
  case MSG_SBJ_BOARD:
    handleBoardSubject(message);
    break;
  case MSG_SBJ_MANAGEMENT:
    handleManagementSubject(message);
    break;
  case MSG_SBJ_MONITORING:
    handleMonitoringSubject(message);
    break;
  case MSG_SBJ_HISTORY:
    handleHistorySubject(message);
    break;
  default:
#ifdef DEBUG
    Serial.print(F("Not implemented subject: "));
    Serial.println(message->subject);
#endif
    break;
  }
}

void handleBoardSubject(const message_in_t *message)
{
  switch (message->command)
  {
  case MSG_CMD_BOARD_STATE:
    sendSystemState(message->number);
    break;
  case MSG_CMD_BOARD_CAPTURE:
    setSystemStateCapturing(message->data);
    break;
  case MSG_CMD_BOARD_SUBSCRIBE:
    subscribe(message->data[0], message->number, message->data[1]);
    break;
  }
}

void setSystemStateCapturing(const uint8_t *data)
{
  setSystemState(data[0] ? SYSTEM_STATE_CAPTURING : SYSTEM_STATE_IDLE);
  sendSystemState(WEB_SOCKET_NUMBER_BROADCAST);
}

void handleManagementSubject(const message_in_t *message)
{
  resetMessageOut(message->subject, message->command);
  size_t subjectCommandLength = sizeof(messageOut->subject) + sizeof(messageOut->command);

  device_t *device;
  uint8_t deviceIndex = 0;
  uint8_t flag;
  uint8_t addressLength;

  switch (message->command)
  {
  case MSG_CMD_MANAGEMENT_REGISTERED_DEVICES_COUNT:
    messageOut->data[0] = deviceCount;

    subscribe(message->subject, message->number, 1);

    sendBin(message->number, subjectCommandLength + sizeof(uint8_t));
    break;
  case MSG_CMD_MANAGEMENT_REGISTERED_DEVICE:
    deviceIndex = message->data[0];

    if (deviceIndex < deviceCount)
    {
      device = devices[deviceIndex];
    }
    else
    {
      break;
    }

    messageOut->data[0] = deviceIndex;
    messageOut->data[1] = device->flags & DEVICE_FLAG_MANAGEMENT_MASK;
    messageOut->data[2] = device->bus;
    messageOut->data[3] = device->type;
    messageOut->data[4] = strlen(device->name);
    memcpy(&messageOut->data[5], device->name, messageOut->data[4]);
    addressLength = getDeviceAddressLength(device->bus, device->type);
    messageOut->data[5 + messageOut->data[4]] = addressLength;
    if (addressLength)
    {
      memcpy(&messageOut->data[5 + messageOut->data[4] + 1], device->address, addressLength);
    }

    sendBin(message->number,
            subjectCommandLength + (sizeof(uint8_t) * 6) + messageOut->data[4] + messageOut->data[5 + messageOut->data[4]]);
    break;
  case MSG_CMD_MANAGEMENT_SWITCH_DEVICE:
    if (systemState == SYSTEM_STATE_CAPTURING)
    {
      break;
    }

    deviceIndex = message->data[0];

    if (!isDeviceEditable(deviceIndex))
    {
      break;
    }
    device = devices[deviceIndex];

    if (message->data[1])
    {
      device->flags |= DEVICE_FLAG_ENABLED;
    }
    else
    {
      device->flags &= 0xFF ^ DEVICE_FLAG_ENABLED;
    }

    messageOut->data[0] = deviceIndex;
    messageOut->data[1] = message->data[1];
    sendBin(WEB_SOCKET_NUMBER_BROADCAST_MANAGEMENT, subjectCommandLength + (sizeof(uint8_t) * 2));
    notifySourcesCountChanged();
    break;
  case MSG_CMD_MANAGEMENT_ALL_DEVICES_COUNT:
    messageOut->data[0] = message->data[0];
    switch (message->data[0])
    {
    case DEVICE_BUS_ANALOG:
      messageOut->data[1] = sizeof(AnalogPins) / sizeof(pin_device_t);
      break;
    case DEVICE_BUS_DIGITAL:
      messageOut->data[1] = sizeof(DigitalPins) / sizeof(pin_device_t);
      break;
    case DEVICE_BUS_OW:
      messageOut->data[1] = getOwDevicesCount();
      break;
    case DEVICE_BUS_EXTENSION_DEVICE_ANALOG:
    case DEVICE_BUS_EXTENSION_DEVICE_DIGITAL:
      extensionBoardMessageOut[0] = MSG_CMD_MANAGEMENT_ALL_DEVICES_COUNT;
      extensionBoardMessageOut[1] = message->data[0];
      sendExtensionBoardMessage();
      if (readFromExtensionBoard())
      {
        messageOut->data[1] = extensionBoardMessageIn[0];
      }
      break;
    }

    sendBin(message->number, subjectCommandLength + (sizeof(uint8_t) * 2));
    break;
  case MSG_CMD_MANAGEMENT_DEVICE:
    messageOut->data[0] = message->data[0];
    messageOut->data[1] = message->data[1];

    switch (message->data[0])
    {
    case DEVICE_BUS_ANALOG:
    case DEVICE_BUS_DIGITAL:
      populatePinDevice(message->data[0], message->data[1], (uint8_t *)&messageOut->data[3]);
      break;
    case DEVICE_BUS_OW:
      populateOwDevice(message->data[1], (uint8_t *)&messageOut->data[3]);
      break;
    case DEVICE_BUS_EXTENSION_DEVICE_ANALOG:
    case DEVICE_BUS_EXTENSION_DEVICE_DIGITAL:
      extensionBoardMessageOut[0] = MSG_CMD_MANAGEMENT_DEVICE;
      extensionBoardMessageOut[1] = message->data[0];
      extensionBoardMessageOut[2] = message->data[1];
      sendExtensionBoardMessage();
      if (readFromExtensionBoard())
      {
        messageOut->data[3] = extensionBoardMessageIn[0];
        messageOut->data[4] = extensionBoardMessageIn[1];
        memcpy(&messageOut->data[5], &extensionBoardMessageIn[2], extensionBoardMessageIn[1]);
      }
      break;
    default:
      break;
    }

    messageOut->data[2] = isDeviceRegistered(message->data[0], &messageOut->data[5], messageOut->data[4]);

    sendBin(message->number, subjectCommandLength + (sizeof(uint8_t) * 5) + messageOut->data[4]);
    break;
  case MSG_CMD_MANAGEMENT_REGISTER_DEVICE:
    if (systemState == SYSTEM_STATE_CAPTURING)
    {
      break;
    }  
    registerDevice(message->data, message->length);
    break;
  case MSG_CMD_MANAGEMENT_UNREGISTER_DEVICE:
    if (systemState == SYSTEM_STATE_CAPTURING)
    {
      break;
    }
    unregisterDevice(message->data[0]);
    break;
  case MSG_CMD_MANAGEMENT_RENAME_DEVICE:
    if (systemState == SYSTEM_STATE_CAPTURING)
    {
      break;
    }
    break;
  }
}

void notifySourcesCountChanged()
{
  if (!anySubscriber(MSG_SBJ_MONITORING))
  {
    return;
  }

  uint8_t payloadIn[] = {MSG_SBJ_MONITORING, MSG_CMD_MONITORING_SOURCES_COUNT, 0};
  message_in_t messageIn(WEB_SOCKET_NUMBER_BROADCAST_MONITORING,
                         (const uint8_t *)&payloadIn,
                         sizeof(payloadIn));

  handleMonitoringSubject(&messageIn);
}

uint8_t anySubscriber(uint8_t subject)
{
  for (uint8_t clientIndex = 0; clientIndex < sizeof(subscribers_t); clientIndex++)
  {
    if (subscribers[subject][clientIndex])
    {
      return 1;
    }
  }
  return 0;
}

void subscribe(uint8_t subject, uint8_t number, uint8_t flag)
{
  if (subject >= sizeof(subscribers) / sizeof(subscribers_t) || number >= sizeof(subscribers_t))
  {
    return;
  }

#ifdef DEBUG
  Serial.printf_P(PSTR("%subscribe(%d) %d=%d\r\n"), flag ? "S" : "Uns", subject, number);
#endif

  subscribers[subject][number] = flag;
}

uint8_t isDeviceRegistered(uint8_t bus, const uint8_t *address, uint8_t addressLength)
{
  if (!addressLength)
  {
    return 0;
  }

  uint8_t match;

  for (uint8_t deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++)
  {
    if (devices[deviceIndex]->bus != bus)
    {
      continue;
    }

    match = 1;
    for (uint8_t byteIndex = 0; byteIndex < addressLength; byteIndex++)
    {
      if (address[byteIndex] != devices[deviceIndex]->address[byteIndex])
      {
        match = 0;
        break;
      }
    }
    if (match)
    {
      return 1;
    }
  }

  return 0;
}

void handleMonitoringSubject(const message_in_t *message)
{
  uint8_t deviceIndex = 0;
  device_t *device;

  if (message->command == MSG_CMD_MONITORING_PUT_EXTRA ||
    message->command == MSG_CMD_MONITORING_MUTE)
  {
    if (systemState == SYSTEM_STATE_CAPTURING)
    {
      return;
    }

    deviceIndex = message->data[0];
    if (deviceIndex >= deviceCount)
    {
      return;
    }
    device = devices[deviceIndex];
    if (message->command == MSG_CMD_MONITORING_PUT_EXTRA)
    {
      uint8_t extra[DEVICE_EXTRA_LENGTH];
      memcpy(device->extra, &message->data[2], message->data[1]);
      memcpy(&extra, device->extra, sizeof(extra));

      EEPROM.begin(EEPROM_DEVICES_BUFFER);
      EEPROM.put(deviceExtraOffset(device->extraBlock), extra);
      EEPROM.end();
    }
    else
    {
      if (message->data[1])
      {
        device->flags |= DEVICE_FLAG_MUTED;
      }
      else
      {
        device->flags &= 0xFF ^ DEVICE_FLAG_MUTED;
      }
    }

    // Send update for subscribed ones
    uint8_t payloadIn[] = {MSG_SBJ_MONITORING, MSG_CMD_MONITORING_SOURCE, 0, deviceIndex};
    message_in_t messageIn(WEB_SOCKET_NUMBER_BROADCAST_MONITORING,
                           (const uint8_t *)&payloadIn,
                           sizeof(payloadIn));

    handleMonitoringSubject(&messageIn);
    return;
  }

  resetMessageOut(message->subject, message->command);
  size_t subjectCommandLength = sizeof(messageOut->subject) + sizeof(messageOut->command);

  switch (message->command)
  {
  case MSG_CMD_MONITORING_SOURCES_COUNT:
    messageOut->data[0] = 0;
    for (deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++)
    {
      if (devices[deviceIndex]->flags & DEVICE_FLAG_ENABLED && !(devices[deviceIndex]->flags & DEVICE_FLAG_NO_DATA))
      {
        messageOut->data[++messageOut->data[0]] = deviceIndex;
      }
    }

    // Auto subscribe after requesting monitoring initial data
    subscribe(message->subject, message->number, 1);

    sendBin(message->number, subjectCommandLength + sizeof(uint8_t) + messageOut->data[0]);
    break;
  case MSG_CMD_MONITORING_SOURCE:
    deviceIndex = message->data[0];
    if ((devices[deviceIndex]->flags & DEVICE_FLAG_NO_DATA) || deviceIndex >= deviceCount)
    {
      break;
    }
    device = devices[deviceIndex];
    messageOut->data[0] = deviceIndex;
    messageOut->data[1] = device->flags & (DEVICE_FLAG_OUTPUT | DEVICE_FLAG_MUTED);
    messageOut->data[2] = DEVICE_EXTRA_LENGTH;
    memcpy(&messageOut->data[3], device->extra, DEVICE_EXTRA_LENGTH);
    messageOut->data[3 + messageOut->data[2]] = strlen(device->name);
    memcpy(&messageOut->data[4 + messageOut->data[2]], device->name, messageOut->data[3 + messageOut->data[2]]);

    sendBin(message->number, subjectCommandLength + (sizeof(uint8_t) * 4) + messageOut->data[2] + messageOut->data[3 + messageOut->data[2]]);
    break;
  case MSG_CMD_MONITORING_SOURCE_DATA:
    // Send source data to requestor
    sendSourceData(message->number, 0, message->data[0]);
    break;
  }
}

void handleHistorySubject(const message_in_t *message)
{
  if (systemState != SYSTEM_STATE_IDLE)
  {
    return;
  }

  resetMessageOut(message->subject, message->command);
  size_t subjectCommandLength = sizeof(messageOut->subject) + sizeof(messageOut->command);
  uint32_t fileSize;
  File file;
  uint8_t fileIndex = 0;
  char *fileName;
  char *name;

  switch (message->command)
  {
  case MSG_CMD_HISTORY_FILES_COUNT:
    messageOut->data[0] = 0;

    file = SD.open("/lab_board");
    while (fsDataFile = file.openNextFile())
    {
      messageOut->data[0]++;
      fsDataFile.close();
    }
    file.close();

    subscribe(message->subject, message->number, 1);

    sendBin(message->number, subjectCommandLength + sizeof(uint8_t));
    break;
  case MSG_CMD_HISTORY_FILE:
    messageOut->data[0] = message->data[0];
    file = SD.open("/lab_board");
    while ((fsDataFile = file.openNextFile()) && fileIndex++ < message->data[0])
      ;
    fileSize = fsDataFile.size();
    memcpy(&messageOut->data[1], &fileSize, sizeof(fileSize));
    messageOut->data[1 + sizeof(fileSize)] = strlen(fsDataFile.name());
    memcpy(&messageOut->data[2 + sizeof(fileSize)], fsDataFile.name(), messageOut->data[1 + sizeof(fileSize)]);
    fsDataFile.close();
    file.close();

    sendBin(message->number, subjectCommandLength + sizeof(uint8_t) + sizeof(fileSize) + sizeof(uint8_t) + messageOut->data[1 + sizeof(fileSize)]);
    break;
  case MSG_CMD_HISTORY_REMOVE_FILE:
    fileName = new char[CAPTURING_FILE_NAME_MAX_LEN];
    name = new char[message->data[0] + 1];
    memcpy(name, &message->data[1], message->data[0]);
    // Make c_str
    name[message->data[0]] = 0;
    sprintf_P(fileName, PSTR("/lab_board/%s"), name);
    SD.remove(fileName);

    delete fileName;
    delete name;

    messageOut->data[0] = message->data[0];
    memcpy(&messageOut->data[1], &message->data[1], messageOut->data[0]);
    sendBin(WEB_SOCKET_NUMBER_BROADCAST_HISTORY, subjectCommandLength + sizeof(uint8_t) + messageOut->data[0]);
    break;
  }
}

void sendBin(uint8_t number, size_t length)
{
  switch (number)
  {
  case WEB_SOCKET_NUMBER_BROADCAST:
    webSocketServer.broadcastBIN((uint8_t *)messageOut, length, false);
    return;
  case WEB_SOCKET_NUMBER_BROADCAST_MANAGEMENT:
  case WEB_SOCKET_NUMBER_BROADCAST_MONITORING:
  case WEB_SOCKET_NUMBER_BROADCAST_HISTORY:
    for (uint8_t clientIndex = 0; clientIndex < sizeof(subscribers_t); clientIndex++)
    {
      if (subscribers[WEB_SOCKET_NUMBER_BROADCAST - number][clientIndex])
      {
        sendBin(clientIndex, length);
      }
    }
    return;
  default:
    webSocketServer.sendBIN(number, (uint8_t *)messageOut, length, false);
    return;
  }
}

uint8_t getOwDevicesCount()
{
  uint8_t count = 0;
  DeviceAddress deviceAddress;
  oneWire->reset_search();
  while (oneWire->search(deviceAddress))
  {
    count++;
  }
  return count;
}

void populatePinDevice(uint8_t bus, uint8_t index, uint8_t *data)
{
  pin_device_t device;
  pin_device_t *deviceArrayAddress;
  if (bus == DEVICE_BUS_ANALOG)
  {
    deviceArrayAddress = (pin_device_t *)&AnalogPins[index];
  }
  else
  {
    deviceArrayAddress = (pin_device_t *)&DigitalPins[index];
  }

  memcpy_P(&device, (const pin_device_t *)deviceArrayAddress, sizeof(device));

  data[0] = DEVICE_TYPE_PIN;
  data[1] = sizeof(device.address);
  data[2] = device.address;
}

void populateOwDevice(uint8_t index, uint8_t *data)
{
  DeviceAddress deviceAddress;
  uint8_t deviceIndex = 0;
  oneWire->reset_search();
  while (oneWire->search(deviceAddress) && deviceIndex++ < index)
    ;

  data[0] = ds18b20->validAddress(deviceAddress) && ds18b20->validFamily(deviceAddress) ? DEVICE_TYPE_DS18B20 : DEVICE_TYPE_UNKNOWN;
  data[1] = sizeof(DeviceAddress);
  memcpy(&data[2], &deviceAddress, sizeof(DeviceAddress));
}

void registerDevice(const uint8_t *data, uint8_t length)
{
  stored_device_t device;
  memset(&device, 0, sizeof(device));
  device.flags = data[0];
  device.bus = data[1];
  device.type = data[2];

  uint8_t nameLength = min((int)data[3], DEVICE_NAME_CONTENT_LENGTH);
  memcpy(&device.name, &data[4], nameLength);

  int addressLength = getDeviceAddressLength(device.bus, device.type);
  addressLength = min(addressLength, DEVICE_ADDRESS_LENGTH);

  if (addressLength)
  {
    if (isDeviceRegistered(data[1], &data[4 + data[3]], addressLength))
    {
      return;
    }

    memcpy(&device.address, &data[4 + data[3]], addressLength);
  }

  // Get the lowest free extra block
  uint8_t extraBlock;
  uint8_t extraBlockInUse;
  for (extraBlock = 0; extraBlock < deviceCount; extraBlock++)
  {
    extraBlockInUse = 0;
    for (uint8_t deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++)
    {
      if (!(devices[deviceIndex]->flags & DEVICE_FLAG_NO_DATA) && devices[deviceIndex]->extraBlock == extraBlock)
      {
        extraBlockInUse = 1;
        break;
      }
    }

    if (!extraBlockInUse)
    {
      break;
    }
  }

  device.extraBlock = extraBlock;

  EEPROM.begin(EEPROM_DEVICES_BUFFER);

  uint8_t freeBlockIndex = 0;
  uint8_t block;
  uint8_t freeBlocks[DEVICE_MAX_COUNT];
  memset(&freeBlocks, 1, sizeof(freeBlocks));
  for (uint8_t blockIndex = 1; blockIndex <= DEVICE_MAX_COUNT; blockIndex++)
  {
    block = EEPROM.read(EEPROM_DEVICES_OFFSET + blockIndex - 1);
    if (!block && !freeBlockIndex)
    {
      freeBlockIndex = blockIndex;
    }
    else if (block)
    {
      freeBlocks[block - 1] = 0;
    }
  }

  uint8_t freeBlock = 0;
  for (uint8_t blockIndex = 0; blockIndex < DEVICE_MAX_COUNT; blockIndex++)
  {
    if (freeBlocks[blockIndex])
    {
      freeBlock = blockIndex + 1;
      break;
    }
  }

  if (!freeBlock)
  {
    return;
  }

  EEPROM.write(EEPROM_DEVICES_OFFSET + freeBlockIndex - 1, freeBlock);
  EEPROM.commit();

  EEPROM.put(EEPROM_DEVICES_OFFSET + DEVICE_MAX_COUNT + DEVICE_LENGTH * (freeBlock - 1), device);

  // Clear previous extra
  uint8_t extra[DEVICE_EXTRA_LENGTH];
  memset(&extra, 0, sizeof(extra));
  EEPROM.put(deviceExtraOffset(device.extraBlock), extra);

  EEPROM.end();

  restart();
}

void unregisterDevice(const uint8_t deviceIndex)
{
  if (!isDeviceEditable(deviceIndex))
  {
    return;
  }

  EEPROM.begin(EEPROM_DEVICES_BUFFER);
  uint8_t storedDeviceIndex = deviceIndex - DEVICE_SYSTEM_COUNT;
  uint8_t storedDevicesCount = deviceCount - DEVICE_SYSTEM_COUNT;

  for (uint8_t blockIndex = storedDeviceIndex; blockIndex < storedDevicesCount - 1; blockIndex++)
  {
    EEPROM.write(EEPROM_DEVICES_OFFSET + blockIndex, EEPROM.read(EEPROM_DEVICES_OFFSET + blockIndex + 1));
  }

  EEPROM.write(EEPROM_DEVICES_OFFSET + storedDevicesCount - 1, 0);
  EEPROM.end();

  restart();
}

bool isDeviceEditable(uint8_t deviceIndex)
{
  return deviceIndex >= DEVICE_SYSTEM_COUNT && deviceIndex < deviceCount;
}

uint8_t getPinHardwareNumber(uint8_t address, uint8_t bus)
{
  pin_device_t pin;
  const pin_device_t *pins;
  uint8_t pinsCount;

  if (bus == DEVICE_BUS_ANALOG)
  {
    pins = (const pin_device_t *)&AnalogPins;
    pinsCount = sizeof(AnalogPins) / sizeof(pin_device_t);
  }
  else
  {
    pins = (const pin_device_t *)&DigitalPins;
    pinsCount = sizeof(DigitalPins) / sizeof(pin_device_t);
  }

  for (uint8_t pinIndex = 0; pinIndex < pinsCount; pinIndex++)
  {
    memcpy_P(&pin, (const pin_device_t *)&pins[pinIndex], sizeof(pin));

    if (pin.address == address)
    {
      return pin.number;
    }
  }

  return 0;
}

void readInputs()
{
  uint32_t now = millis();
  device_d_t value;
  device_t *device;
  uint8_t first = 1;

  if (systemState == SYSTEM_STATE_CAPTURING)
  {
    fsDataFile.printf_P(PSTR("%s{\"t\":%d,\"d\":["),
                        systemVars & SYSTEM_VAR_FIRST_CAPTURE_READ ? "" : ",",
                        now);

    systemVars &= (0xFF ^ SYSTEM_VAR_FIRST_CAPTURE_READ);
  }

  systemVars &= (0xFF ^ SYSTEM_VAR_DS18B20);
  int16_t temperature;
  int pinValue;
  for (uint8_t deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++)
  {
    device = devices[deviceIndex];
    if (!(device->flags & DEVICE_FLAG_ENABLED) || (device->flags & (DEVICE_FLAG_NO_DATA | DEVICE_FLAG_MUTED)))
    {
      continue;
    }

    memset(&value, 0, sizeof(value));

    if ((device->type == DEVICE_TYPE_DS18B20) && (now - ds18b20Requested > ds18b20->millisToWaitForConversion(DS18B20_RESOLUTION)))
    {
      temperature = ds18b20->getTemp((const uint8_t *)device->address);
      if (temperature == DEVICE_DISCONNECTED_RAW)
      {
        setActivityFlag(device, 0);
      }
      else
      {
        setActivityFlag(device, 1);
      }

      memcpy(&value, &temperature, sizeof(temperature));
      setInput(device, &value);
      systemVars |= SYSTEM_VAR_DS18B20;
    }
    else if (isAnalogOrDigitalPin(device->type, device->bus))
    {
      if (device->flags & DEVICE_FLAG_OUTPUT)
      {
        pinValue = device->data[0];
      }
      else
      {
        if (device->bus == DEVICE_BUS_ANALOG)
        {
          pinValue = analogRead(getPinHardwareNumber(device->address[0], DEVICE_BUS_ANALOG));
        }
        else
        {
          pinValue = digitalRead(getPinHardwareNumber(device->address[0], DEVICE_BUS_DIGITAL));
        }
      }

      setActivityFlag(device, 1);
      memcpy(&value, &pinValue, sizeof(pinValue));
      setInput(device, &value);
    }
    else if (device->type == DEVICE_TYPE_DS3231_TEMPERATURE)
    {
      temperature = rtcObject.GetTemperature().AsCentiDegC();

      setActivityFlag(device, 1);
      memcpy(&value, &temperature, sizeof(temperature));
      setInput(device, &value);
    }
    else if (isExtensionDeviceBus(device->bus))
    {
      extensionBoardMessageOut[0] = MSG_CMD_MONITORING_SOURCE_DATA;
      extensionBoardMessageOut[1] = getExtensionDeviceIndex(deviceIndex);
      sendExtensionBoardMessage();
      if (readFromExtensionBoard())
      {
        setActivityFlag(device, 1);
        setInput(device, (const device_d_t *)&extensionBoardMessageIn);
      }
    }

    if (systemState == SYSTEM_STATE_CAPTURING && !(device->flags & DEVICE_FLAG_OUTPUT))
    {
      writeDeviceValue(deviceIndex, device, first);
      first = 0;
    }
  }

  if (systemState == SYSTEM_STATE_CAPTURING)
  {
    fsDataFile.print(F("]}\r\n"));
    fsDataFile.flush();
  }

  if (systemVars & SYSTEM_VAR_DS18B20)
  {
    ds18b20->requestTemperatures();
    ds18b20Requested = millis();
  }
}

void writeDeviceValue(const uint8_t index, const device_t *device, const uint8_t first)
{
  memset(&dataStr, 0, sizeof(dataStr));
  fillFromArray((char *)&dataStr, (const uint8_t *)&device->data, sizeof(device_d_t));

  fsDataFile.printf_P(PSTR("%s{\"i\":%d,\"f\":%d,\"v\":[%s]}"), first ? "" : ",", index, device->flags, dataStr);
}

uint8_t readFromExtensionBoard()
{
  uint8_t bytesRead = Wire.requestFrom(EXTENSION_BOARD_ADDRESS, sizeof(extensionBoardMessageIn));

  for (uint8_t byteIndex = 0; byteIndex < bytesRead; byteIndex++)
  {
    extensionBoardMessageIn[byteIndex] = Wire.read();
  }

  return bytesRead;
}

void setActivityFlag(device_t *device, uint8_t flag)
{
  if ((device->flags & DEVICE_FLAG_ACTIVE) != (flag * DEVICE_FLAG_ACTIVE))
  {
    device->flags ^= DEVICE_FLAG_ACTIVE;
    device->flags |= DEVICE_FLAG_SYNC;
  }
}

void setInput(device_t *input, const device_d_t *value)
{
  const uint8_t *valueBuff = (const uint8_t *)value;
  uint8_t changeDirectionFlag = 0;
  for (uint8_t byteIndex = 0; byteIndex < sizeof(device_d_t); byteIndex++)
  {
    if (input->data[byteIndex] != valueBuff[byteIndex])
    {
      input->flags |= DEVICE_FLAG_SYNC;
      memcpy(&input->data, value, sizeof(device_d_t));
      return;
    }
  }
}

void sendExtensionBoardMessage()
{
  Wire.beginTransmission(EXTENSION_BOARD_ADDRESS);
  Wire.write((const uint8_t *)&extensionBoardMessageOut, sizeof(extensionBoardMessageOut));
  Wire.endTransmission();
}
