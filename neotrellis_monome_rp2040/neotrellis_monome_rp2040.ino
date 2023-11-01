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
//#define DEVICE_SERIAL_NUMBER "m332960073452" // under construction
//#define DEVICE_ADDR_CONST 0
 #define DEVICE_SERIAL_NUMBER "m52500681" // in case
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

#define NORMAL_MODE 0
#define PICKER_MODE 2
uint8_t current_mode = NORMAL_MODE;

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

uint32_t colorForN(uint16_t led_n,uint16_t val){
	if(led_n > 15){
		return 0x808000;
	}
	if(led_n == 15){
		return val > 0x7fff ? 0x800000 : 0x404040;
	}

	uint8_t islit = (val >> led_n) & 0x1;

	if(islit){
		uint8_t r = allpalettes[8][0][val]*32/255;
		uint8_t g = allpalettes[8][1][val]*32/255;
		uint8_t b = allpalettes[8][2][val]*32/255;
		return (r << 16) + (g << 8) + (b << 0);
	} else {
		return 0x000000;
	}
}

bool validateAtStart(){
	bool isValid = true;
	for (uint8_t x = 0; x < NUM_COLS / 4; x++) {
		for (uint8_t y = 0; y < NUM_ROWS / 4; y++) {
			Adafruit_NeoTrellis t_test = trellis_array[y][x];
			if (t_test.begin())
			{
				for(uint8_t ledn=0;ledn<16;ledn++){
					t_test.pixels.setPixelColor(ledn,colorForN(ledn, x));
				}
				t_test.pixels.show();
			} else {
				isValid=false;
			}
		}
	}
	return isValid;
}

Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)trellis_array, NUM_ROWS / 4, NUM_COLS / 4);

uint8_t chordcount = 0;

bool detectChordPress(uint8_t x, uint8_t y) {
	if ((x==13 && y==7) || (x==14 && y==7) || (x==15 && y==7)) {
		chordcount += 1;
		if(chordcount == 3){
			chordcount = 0;
			return 1;
		}
	} else {
		chordcount = 0;
	}
	return 0;
}
void blankLeds(){
	for (uint8_t x = 0; x < NUM_COLS; x++) {
		for (uint8_t y = 0; y < NUM_ROWS; y++) {
			trellis.setPixelColor((y*NUM_COLS+x), 0x000000);
		}
	}
	trellis.show();
}
uint8_t pickerstep = 0;
uint8_t pickershift = 0;
void drawPickerMode(){
	trellis.setPixelColor(0,0,0x900000);

	float f = 1.0*255.0/NUM_ROWS;
	for (uint8_t y = 1; y < NUM_ROWS; y++) {
		uint8_t c = y*f;
		trellis.setPixelColor(0, y, (c << 16) + (c << 8) + (c << 0));
	}


	for (uint8_t x = 1; x < NUM_COLS; x++) {
		for (uint8_t y = 0; y < NUM_ROWS; y++) {
			uint8_t palettedemo = (x+pickershift) % 25;
			uint8_t palettestep = (pickerstep+y)%16;
			uint8_t gValue = gammaTable[palettestep];
			uint8_t r = allpalettes[palettedemo][0][palettestep]*gValue/255;
			uint8_t g = allpalettes[palettedemo][1][palettestep]*gValue/255;
			uint8_t b = allpalettes[palettedemo][2][palettestep]*gValue/255;
			trellis.setPixelColor(x, y, (r << 16) + (g << 8) + (b << 0));
		}
	}

	// page buttons
	trellis.setPixelColor(NUM_COLS-1, NUM_ROWS-1, 0);
	trellis.setPixelColor(NUM_COLS-2, NUM_ROWS-1, 0);

	trellis.show();
	pickerstep = pickerstep + 1;
	if(pickerstep == 16){
		pickerstep = 0;
	}
}

//define a callback for key presses
TrellisCallback keyCallback(keyEvent evt){
	uint8_t x  = evt.bit.NUM % NUM_COLS;
	uint8_t y = evt.bit.NUM / NUM_COLS;

	if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING){
		keyDown(x,y);
	}else if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING){
		keyUp(x,y);
	}
	return 0;
}
void keyDown(uint8_t x, uint8_t y){
	if(current_mode == NORMAL_MODE){
		if(detectChordPress(x,y)){
			current_mode = PICKER_MODE;
			pickershift = 0;
		}else{
			mdp.sendGridKey(x, y, 1);
		}
	} else if( current_mode == PICKER_MODE){
		if(x==NUM_COLS-1 && y==NUM_ROWS-1){
			if(pickershift==24) {
				pickershift = 0;
			}else{
				pickershift += 1;
			}
			return;
		}
		if(x==NUM_COLS-2 && y==NUM_ROWS-1){
			if(pickershift==0) {
				pickershift = 24;
			}else{
				pickershift -= 1;
			}
			return;
		}
		current_mode = NORMAL_MODE;
		blankLeds();
		if(x==0&&y==0){
			return;
		}
		if(x==0){
			Serial.println("Selected gamma:");
			Serial.println(y);
			setGammaTable(y);
			animateBoard(true);
			return;
		}
		selected_palette = (x+pickershift) % 25;
		Serial.println("Selected palette:");
		Serial.println(selected_palette);
		animateBoard(true);
	}
}
void keyUp(uint8_t x, uint8_t y){
	chordcount = 0;
	if(current_mode == NORMAL_MODE){
		mdp.sendGridKey(x, y, 0);
	}
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
		if(!validateAtStart()){
			while(1)
			{
				blinkLed(200,450);
			}
		}
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
	float initGamma = 1.0;
	setGammaTable(initGamma);

	// clear grid leds
	mdp.setAllLEDs(0);
	sendLeds();

	// animate board on start
	animateBoard(false);

	Serial.println("Init gamma:");
	Serial.println(initGamma);
	Serial.println("Init palette:");
	Serial.println(selected_palette);
}

uint32_t getColor(bool palette, uint8_t x, uint8_t y){
	uint32_t r = 0;
	uint32_t g = 0;
	uint32_t b = 0;

	if(palette) {
		uint8_t value = (x+y)%16;
		uint8_t gValue = gammaTable[value];
		r = allpalettes[selected_palette][0][value]*gValue/255;
		g = allpalettes[selected_palette][1][value]*gValue/255;
		b = allpalettes[selected_palette][2][value]*gValue/255;
	} else {
		r = ((0xFF / NUM_COLS) * x) ;
		g = ((0xFF / NUM_ROWS) * y) ;
		b = DEVICE_ADDR_CONST ? 0 : 0x99;
	}
	return (r<<16) | (g<<8) | b;
}

void animateBoard(bool palette){
	for (uint8_t x = 0; x < NUM_COLS; x++) {
		for (uint8_t y = 0; y < NUM_ROWS; y++) {
			uint32_t color = getColor(palette,x,y);
			trellis.setPixelColor((y*NUM_COLS+x),  color);
			trellis.show();
			delay(1);
		}
	}
	if(palette){
		delay(1000);
	}
	blankLeds();
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
		if(current_mode == NORMAL_MODE){
			sendLeds();
		} else if( current_mode == PICKER_MODE){
			drawPickerMode();
		}
		monomeRefresh = 0;
	}
}
