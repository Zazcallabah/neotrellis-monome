/***********************************************************
 *  DIY monome compatible grid w/ Adafruit NeoTrellis
 *  for RP2040 Pi Pico
 *
 *  This code makes the Adafruit Neotrellis boards into a Monome compatible grid via monome's mext protocol
 *  ----> https://www.adafruit.com/product/3954
 *
 *  Code here is for a 16x8 grid, but can be modified for 4x8, 8x8, or 16x16 (untested on larger grid arrays)
 *
 *  Many thanks to:
 *  scanner_darkly <https://github.com/scanner-darkly>,
 *  TheKitty <https://github.com/TheKitty>,
 *  Szymon Kaliski <https://github.com/szymonkaliski>,
 *  John Park, Todbot, Juanma, Gerald Stevens, and others
 *
*/

/*
	Edits made by Zaz, 2023-04-23.
	set addr const to 16 for second grid
*/
//#define DEVICE_SERIAL_NUMBER "m332960073452"
//#define DEVICE_ADDR_CONST 0
#define DEVICE_SERIAL_NUMBER "m52500681"
#define DEVICE_ADDR_CONST 16

// set to 1 to run neotrellis validation script
#define VALIDATE_AT_START 0

// SET TOOLS USB STACK TO TinyUSB
#include "MonomeSerialDevice.h"
#include <Adafruit_NeoTrellis.h>

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <elapsedMillis.h>
#include "palette.h"

#define NUM_ROWS 8 // DIM_Y number of rows of keys down
#define NUM_COLS 16 // DIM_X number of columns of keys across
#define NUM_LEDS NUM_ROWS*NUM_COLS

// I2C pin defs for RP2040
const byte I2C_SDA = 20;
const byte I2C_SCL = 21;

#define INT_PIN 9
#define LED_PIN 25 // teensy LED used to show boot info
void blinkLed(uint32_t on, uint32_t off){
	digitalWrite(LED_PIN, HIGH);
	delay(on);
	digitalWrite(LED_PIN, LOW);
	delay(off);
}
// This assumes you are using a USB breakout board to route power to the board
// If you are plugging directly into the controller, you will need to adjust this brightness to a much lower value
#define BRIGHTNESS 32 // overall grid brightness - use gamma table below to adjust levels

// gamma table for 16 levels of brightness
uint8_t gammaTable[16] = { 0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

uint8_t selected_palette = 0;
bool globalDirty = false;
bool isInited = false;
elapsedMillis monomeRefresh;

// set your monome device name here
String deviceID = "neo-monome";
String serialNum = DEVICE_SERIAL_NUMBER;

// DEVICE INFO FOR TinyUSB
char mfgstr[32] = "monome";
char prodstr[32] = "monome";
char serialstr[32] = DEVICE_SERIAL_NUMBER;

// Monome class setup
MonomeSerialDevice mdp;

int prevLedBuffer[mdp.MAXLEDCOUNT];

// NeoTrellis setup
Adafruit_NeoTrellis trellis_array[NUM_ROWS / 4][NUM_COLS / 4] = {
	{ Adafruit_NeoTrellis(0x2e + 4 + DEVICE_ADDR_CONST), Adafruit_NeoTrellis(0x2e + 2 + DEVICE_ADDR_CONST), Adafruit_NeoTrellis(0x2e + 1 + DEVICE_ADDR_CONST), Adafruit_NeoTrellis(0x2e + 0 + DEVICE_ADDR_CONST) }, // top row
	{ Adafruit_NeoTrellis(0x2e + 9 + DEVICE_ADDR_CONST), Adafruit_NeoTrellis(0x2e + 5 + DEVICE_ADDR_CONST), Adafruit_NeoTrellis(0x2e + 3 + DEVICE_ADDR_CONST), Adafruit_NeoTrellis(0x2e + 8 + DEVICE_ADDR_CONST) } // bottom row
};

void validateAtStart(){
	uint8_t start = 0x2e;
	for (uint8_t x = 0; x < 32 ; x++) {
		uint8_t addr = start+x;
		Adafruit_NeoTrellis t_test(addr);
		if (t_test.begin(addr))
		{
			t_test.pixels.setPixelColor(1,0xFFFFFF);
			t_test.pixels.show();

		} else {
			blinkLed(100,200);
			blinkLed(100,200);
			blinkLed(100,300);
		}
	}
	while(1){
		blinkLed(400,200);
	}
}

Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)trellis_array, NUM_ROWS / 4, NUM_COLS / 4);

//define a callback for key presses
TrellisCallback keyCallback(keyEvent evt){
	uint8_t x  = evt.bit.NUM % NUM_COLS;
	uint8_t y = evt.bit.NUM / NUM_COLS;

	if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING){
		mdp.sendGridKey(x, y, 1);
	}else if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING){
		mdp.sendGridKey(x, y, 0);
	}
	return 0;
}

void setup(){
	USBDevice.setManufacturerDescriptor(mfgstr);
	USBDevice.setProductDescriptor(prodstr);
	USBDevice.setSerialDescriptor(serialstr);
	pinMode(LED_PIN, OUTPUT);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	Serial.begin(115200);

	Wire.setSDA(I2C_SDA);
	Wire.setSCL(I2C_SCL);

	mdp.isMonome = true;
	mdp.deviceID = deviceID;
	mdp.setupAsGrid(NUM_ROWS, NUM_COLS);
		monomeRefresh = 0;
		isInited = true;

	int var = 0;
	while (var < 8) {
		mdp.poll();
		var++;
		delay(100);
	}
	if(VALIDATE_AT_START){
		validateAtStart();
	}

	if (!trellis.begin()) {
		Serial.println("trellis.begin() failed!");
		Serial.println("check your addresses.");
		Serial.println("reset to try again.");
		while(1)
		{
			blinkLed(50,450);
		}
	}

	// key callback
	for (uint8_t x = 0; x < NUM_COLS; x++) {
		for (uint8_t y = 0; y < NUM_ROWS; y++) {
			trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING, true);
			trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
			trellis.registerCallback(x, y, keyCallback);
		}
	}

	setBrightnessForAllPixels();
	setGammaTable(2);

	// clear grid leds
	mdp.setAllLEDs(0);
	sendLeds();

	// animate board on start
	for (uint8_t x = 0; x < NUM_COLS; x++) {
		for (uint8_t y = 0; y < NUM_ROWS; y++) {
			uint32_t r = ((0xFF / NUM_COLS) * x) << 16;
			uint32_t g = ((0xFF / NUM_ROWS) * y) << 8;
			uint32_t b = DEVICE_ADDR_CONST ? 0 : 0x99;
			trellis.setPixelColor((y*NUM_COLS+x),  r | g | b);
			trellis.show();
			delay(1);
		}
	}
	for (uint8_t x = 0; x < NUM_COLS; x++) {
		for (uint8_t y = 0; y < NUM_ROWS; y++) {
			trellis.setPixelColor((y*NUM_COLS+x), 0x000000);
		}
	}
	trellis.show();
}

void sendLeds(){
	uint8_t value, prevValue = 0;
	bool isDirty = false;
	for(uint8_t i=0; i< NUM_ROWS * NUM_COLS; i++){
		value = mdp.leds[i];
		prevValue = prevLedBuffer[i];
		if (value != prevValue) {
			uint8_t gValue = gammaTable[value];

			uint8_t r = allpalettes[selected_palette][0][value]*gValue/255;
			uint8_t g = allpalettes[selected_palette][1][value]*gValue/255;
			uint8_t b = allpalettes[selected_palette][2][value]*gValue/255;
			trellis.setPixelColor(i, (r << 16) + (g << 8) + (b << 0));

			prevLedBuffer[i] = value;
			isDirty = true;
		}
	}
	if (isDirty || globalDirty) {
		trellis.show();
	}
}

void setBrightnessForAllPixels() {
	for (uint8_t x = 0; x < NUM_COLS / 4; x++) {
		for (uint8_t y = 0; y < NUM_ROWS / 4; y++) {
			trellis_array[y][x].pixels.setBrightness(BRIGHTNESS);
		}
	}
}

// value {0,7}
void setGammaTable(float value){
	float k = ((8.0-value)/8.0)*(255.0) / 15.0;
	for (uint8_t x = 0; x < 16; x++) {
		gammaTable[x] =  k*x;
	}
}

void loop() {
	mdp.poll(); // process incoming serial from Monomes

	// refresh every 16ms or so
	if (isInited && monomeRefresh > 16) {
		trellis.read();
		sendLeds();
		monomeRefresh = 0;
	}
}
