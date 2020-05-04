#include <Wire.h>

//#define DEBUG

#define SERIAL_BAUD_RATE 115200

#define LOOP_DELAY 20
#define RPM_PERIOD 60000
#define RPM_MINIMUM_PERIOD 200
#define RPM_COUNT_THRESHOLD 3000

#define I2C_ADDRESS 111

#define RW_BUFFER_LENGTH 8

#define DEVICE_DATA_LENGTH 4

#define BOARD_STATE_INTERRUPTS 4

#define DEVICE_BUS_UNKNOWN 0
#define DEVICE_BUS_ANALOG 5
#define DEVICE_BUS_DIGITAL 6

#define DEVICE_TYPE_UNKNOWN 0
#define DEVICE_TYPE_PIN 1
#define DEVICE_TYPE_INTERRUPT_CHANGE 5
#define DEVICE_TYPE_INTERRUPT_RISING 6
#define DEVICE_TYPE_INTERRUPT_FALLING 7

#define DEVICE_FLAG_OUTPUT 2
#define DEVICE_FLAG_ENABLED 4

#define MSG_CMD_MANAGEMENT_REGISTERED_DEVICES_COUNT 1
#define MSG_CMD_MANAGEMENT_REGISTERED_DEVICE 2
#define MSG_CMD_MANAGEMENT_ALL_DEVICES_COUNT 7
#define MSG_CMD_MANAGEMENT_DEVICE 8
#define MSG_CMD_MANAGEMENT_DEVICE_VALUE 9

#define MSG_CMD_MONITORING_SOURCE_DATA 3

#define isInterruptDevice(t) ((t) == DEVICE_TYPE_INTERRUPT_CHANGE || (t) == DEVICE_TYPE_INTERRUPT_RISING || (t) == DEVICE_TYPE_INTERRUPT_FALLING ? 1 : 0)
#define isAnalogOrDigitalPin(t, b) ((t) == DEVICE_TYPE_PIN && ((b) == DEVICE_BUS_ANALOG || (b) == DEVICE_BUS_DIGITAL) ? 1 : 0)

typedef void (*InterruptFunction)();

typedef struct
{
	volatile uint16_t counter;
	uint16_t period;
	InterruptFunction function;
} interrupt_t;

typedef struct
{
	uint8_t address;
	uint8_t number;
} pin_device_t;

typedef uint8_t device_d_t[DEVICE_DATA_LENGTH];

typedef struct
{
	uint8_t flags;
	uint8_t bus;
	uint8_t type;
	uint8_t address;
	uint8_t *extra;
	device_d_t data;
} device_t;

const PROGMEM pin_device_t AnalogPins[] =
		{
				{0, A0},
				{1, A1},
				{2, A2},
				{3, A3},
				//{4, A4},
				//{5, A5},
				{6, A6},
				{7, A7}};
const PROGMEM pin_device_t DigitalPins[] =
		{
				{2, 2},
				{3, 3},
				{4, 4},
				{5, 5},
				{6, 6},
				{7, 7},
				{8, 8},
				{9, 9},
				{10, 10},
				{11, 11},
				{12, 12},
				{13, 13}};

#define DEVICE_MAX_COUNT (sizeof(AnalogPins) + sizeof(DigitalPins)) / sizeof(pin_device_t)
device_t devices[DEVICE_MAX_COUNT];

uint8_t readBuffer[RW_BUFFER_LENGTH];
uint8_t writeBuffer[RW_BUFFER_LENGTH];

void fInterruptCounter0();
void fInterruptCounter1();

interrupt_t interrupts[2] =
		{
				{0, 0, fInterruptCounter0},
				{0, 0, fInterruptCounter1}};

uint8_t boardState = 0;

void setup()
{
	memset(&devices, 0, sizeof(devices));

	Wire.begin(I2C_ADDRESS);
	Wire.onReceive(onReceiveI2C);
	Wire.onRequest(onRequestI2C);
#ifdef DEBUG
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.println(F("Setup"));
#endif
}

void loop()
{
	if (boardState & BOARD_STATE_INTERRUPTS)
	{
		sei();
	}

	delay(LOOP_DELAY);

	if (boardState & BOARD_STATE_INTERRUPTS)
	{
		cli();
	}

	readInputs();
}

void readInputs()
{
	device_t *device;
	int pinValue;
	device_d_t value;

	for (uint8_t deviceIndex = 0; deviceIndex < sizeof(devices); deviceIndex++)
	{
		device = &devices[deviceIndex];

		if (!(device->flags & DEVICE_FLAG_ENABLED))
		{
			continue;
		}

		if (device->type == DEVICE_TYPE_PIN &&
				!(device->flags & DEVICE_FLAG_OUTPUT))
		{
			if (device->bus == DEVICE_BUS_ANALOG)
			{
				pinValue = analogRead(getPinHardwareNumber(device->address, DEVICE_BUS_ANALOG));
			}
			else
			{
				pinValue = digitalRead(getPinHardwareNumber(device->address, DEVICE_BUS_DIGITAL));
			}

#ifdef DEBUG
			Serial.print(F("Read input, device: #"));
			Serial.print(deviceIndex);
			Serial.print(F(", value: "));
			Serial.println(pinValue);
#endif

			memcpy(&device->data, &pinValue, sizeof(pinValue));
		}
		else if (isInterruptDevice(device->type))
		{
			uint8_t interruptPin = digitalPinToInterrupt(device->address);
			interrupt_t *interrupt = &interrupts[interruptPin];

			interrupt->period += LOOP_DELAY;

			if (interrupt->counter > RPM_COUNT_THRESHOLD || interrupt->period > RPM_MINIMUM_PERIOD)
			{
				uint16_t rpmValue = (RPM_PERIOD / interrupt->period) * interrupt->counter;
				memcpy(&device->data, &rpmValue, sizeof(rpmValue));

				interrupt->counter = 0;
				interrupt->period = 0;
			}
		}
	}
}

uint8_t getPinHardwareNumber(uint8_t address, uint8_t bus)
{
	pin_device_t pin;
	pin_device_t *pinAddress;
	if (bus == DEVICE_BUS_ANALOG)
	{
		for (uint8_t pinIndex = 0; pinIndex < (sizeof(AnalogPins) / sizeof(pin_device_t)); pinIndex++)
		{
			pinAddress = (pin_device_t *)&AnalogPins[pinIndex];
			memcpy_P(&pin, (const pin_device_t *)pinAddress, sizeof(pin));

			if (pin.address == address)
			{
				return pin.number;
			}
		}
	}
	else
	{
		return address;
	}

	return 0;
}

void fInterruptCounter0()
{
	interrupts[0].counter++;
}
void fInterruptCounter1()
{
	interrupts[1].counter++;
}

void onRequestI2C()
{
#ifdef DEBUG
	Serial.println(F("Write buffer"));
#endif
	Wire.write((uint8_t *)&writeBuffer, sizeof(writeBuffer));
}

void onReceiveI2C(int bytes)
{
#ifdef DEBUG
	Serial.print(F("Receive "));
	Serial.println(bytes);
#endif
	memset(&writeBuffer, 0, sizeof(writeBuffer));

	if (!Wire.readBytes((uint8_t *)&readBuffer, sizeof(readBuffer)) ||
			!readBuffer[0])
	{
		return;
	}

#ifdef DEBUG
	Serial.print("Command ");
	Serial.println(readBuffer[0]);
#endif

	switch (readBuffer[0])
	{
	case MSG_CMD_MANAGEMENT_ALL_DEVICES_COUNT:
		handleCommandAllDevicesCount();
		break;
	case MSG_CMD_MANAGEMENT_DEVICE:
		handleCommandDevice();
		break;
	case MSG_CMD_MANAGEMENT_REGISTERED_DEVICE:
		handleCommandRegisteredDevice();
		break;
	case MSG_CMD_MANAGEMENT_DEVICE_VALUE:
		handleCommandDeviceValue();
		break;
	case MSG_CMD_MONITORING_SOURCE_DATA:
		handleCommandSourceData();
		break;
	default:
		return;
	}
}

void handleCommandAllDevicesCount()
{
#ifdef DEBUG
	Serial.println(F("Request all devices count"));
#endif
	switch (readBuffer[1])
	{
	case DEVICE_BUS_ANALOG:
		writeBuffer[0] = sizeof(AnalogPins) / sizeof(pin_device_t);
		break;
	case DEVICE_BUS_DIGITAL:
		writeBuffer[0] = sizeof(DigitalPins) / sizeof(pin_device_t);
		break;
	}
}

void handleCommandDevice()
{
#ifdef DEBUG
	Serial.println(F("Request device"));
#endif

	pin_device_t device;
	pin_device_t *deviceArrayAddress;
	if (readBuffer[1] == DEVICE_BUS_ANALOG)
	{
		deviceArrayAddress = (pin_device_t *)&AnalogPins[readBuffer[2]];
	}
	else
	{
		deviceArrayAddress = (pin_device_t *)&DigitalPins[readBuffer[2]];
	}

	memcpy_P(&device, (const pin_device_t *)deviceArrayAddress, sizeof(device));

	writeBuffer[0] = DEVICE_TYPE_PIN;
	writeBuffer[1] = sizeof(device.address);
	memcpy(&writeBuffer[2], &device.address, writeBuffer[1]);
}

void handleCommandRegisteredDevice()
{
#ifdef DEBUG
	char debugMessage[255];
	sprintf_P((char *)&debugMessage, PSTR("Register device #%d, f:%d, b:%d, t:%d, @:%d"),
						readBuffer[1],
						readBuffer[2],
						readBuffer[3],
						readBuffer[4],
						readBuffer[5]);
	Serial.println(debugMessage);
#endif

	device_t *device;

	// No matter order
	device = &devices[readBuffer[1]];
	device->flags = readBuffer[2];
	device->bus = readBuffer[3];
	device->type = readBuffer[4];
	device->address = readBuffer[5];

	if (isAnalogOrDigitalPin(device->type, device->bus))
	{
		pinMode(getPinHardwareNumber(device->address, device->bus), device->flags & DEVICE_FLAG_OUTPUT ? OUTPUT : INPUT);
	}
	else if (isInterruptDevice(device->type))
	{
		pinMode(getPinHardwareNumber(device->address, device->bus), INPUT_PULLUP);
		uint8_t interruptPin = digitalPinToInterrupt(device->address);

		attachInterrupt(interruptPin, interrupts[interruptPin].function, device->type - 4);
		boardState |= BOARD_STATE_INTERRUPTS;
	}
}

void handleCommandDeviceValue()
{
	device_t *device;

	device = &devices[readBuffer[1]];

	// Sanity check
	if (!(device->flags & DEVICE_FLAG_ENABLED) ||
			!(device->flags & DEVICE_FLAG_OUTPUT))
	{
		return;
	}

	device->data[0] = readBuffer[2];

	if (device->bus == DEVICE_BUS_ANALOG)
	{
		analogWrite(getPinHardwareNumber(device->address, DEVICE_BUS_ANALOG), device->data[0]);
	}
	else if (device->bus == DEVICE_BUS_DIGITAL)
	{
		digitalWrite(getPinHardwareNumber(device->address, DEVICE_BUS_DIGITAL), device->data[0]);
	}
}

void handleCommandSourceData()
{
#ifdef DEBUG
	char debugMessage[255];
	sprintf_P((char *)&debugMessage, PSTR("Register source data #%d, {%d:%d:%d:%d}"),
						readBuffer[1],
						devices[readBuffer[1]].data[0],
						devices[readBuffer[1]].data[1],
						devices[readBuffer[1]].data[2],
						devices[readBuffer[1]].data[3]);
	Serial.println(debugMessage);
#endif
	memcpy(&writeBuffer, &devices[readBuffer[1]].data, sizeof(device_d_t));
}
