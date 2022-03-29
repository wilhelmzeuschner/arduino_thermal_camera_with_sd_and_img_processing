// SD Image Save
#define SAVE70x70
//#define SAVE8X8
// OTA
#define USE_OTA // https://docs.platformio.org/en/latest/platforms/espressif32.html#over-the-air-ota-update

/*

  This program is for upsizing an 8 x 8 array of thermal camera readings
  it will size up by 10x and display to a 240 x 320
  interpolation is linear and "good enough" given the display is a 5-6-5 color palette
  Total final array is an array of 70 x 70 of internal points only

  Revisions
  1.0		Kasprzak      Initial code

  Branches
  1.0		Deiss         Code modified for Wemos D1 Mini, vertical display, temperature measurement at center, battery measurement
  1.1		Deiss         Exchanged TFT driver for better performance regarding framerate

  2.0		Wilhelm Zeuschner: Added SD-card support and OTA-Updating (hold the capture image button down during boot)
			https://github.com/wilhelmzeuschner/arduino_thermal_camera_with_sd_and_img_processing

  MCU                       Wemos D1 Mini clone
  Display                   https://www.amazon.com/Wrisky-240x320-Serial-Module-ILI9341/dp/B01KX26JJU/ref=sr_1_10?ie=UTF8&qid=1510373771&sr=8-10&keywords=240+x+320+tft
  Thermal sensor            https://learn.adafruit.com/adafruit-amg8833-8x8-thermal-camera-sensor/overview
  sensor library            https://github.com/adafruit/Adafruit_AMG88xx
  equations generated from  http://web-tech.ga-usa.com/2012/05/creating-a-custom-hot-to-cold-temperature-color-gradient-for-use-with-rrdtool/index.html

  Pinouts
  MCU         Device
  D1          AMG SDA
  D2          AMG SCL
  Gnd         Dispaly GND, AMG Gnd
  3v3         Dispaly Vcc,Display LED,Display RST, AMG Vcc
  D0          Dispaly T_IRQ
  D8          Display D/C
  D3          Display CS
  D7          Display SDI
  D6          Dispaly SDO
  D5          Display SCK

  SD:
  D4          SD SS (CS)

*/

#include <Arduino.h>

#ifdef USE_OTA
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#endif

#include <TFT_eSPI.h>
#include <Adafruit_AMG88xx.h> //Thermal camera lib
#include <SPI.h>
#include "SdFat.h"
#include "sdios.h"
#include <Wire.h>
#include <user_interface.h>

#ifdef USE_OTA
#warning Set your WiFi Passwords here!
const char *ssid = "yourSSID";
const char *password = "yourPass";
#endif

#define PIN_INT -1 // D0 Interrupt from touch for autoscale/scale

// constants for the cute little keypad
#define KEYPAD_TOP 15
#define KEYPAD_LEFT 50
#define BUTTON_W 60
#define BUTTON_H 30
#define BUTTON_SPACING_X 10
#define BUTTON_SPACING_Y 10
#define BUTTON_TEXTSIZE 2

TFT_eSPI Display = TFT_eSPI();

// create some colors for the keypad buttons
#define C_BLUE Display.color565(0, 0, 255)
#define C_RED Display.color565(255, 0, 0)
#define C_GREEN Display.color565(0, 255, 0)
#define C_WHITE Display.color565(255, 255, 255)
#define C_BLACK Display.color565(0, 0, 0)
#define C_LTGREY Display.color565(200, 200, 200)
#define C_DKGREY Display.color565(80, 80, 80)
#define C_GREY Display.color565(127, 127, 127)

// Added for measure Temp
boolean measure = true;
float centerTemp;
unsigned long tempTime = millis();
unsigned long tempTime2 = 0;
unsigned long batteryTime = 1;

// create some text for the keypad butons
char KeyPadBtnText[12][5] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "Done", "0", "Clr"};

// define some colors for the keypad buttons
uint16_t KeyPadBtnColor[12] = {C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_GREEN, C_BLUE, C_RED};

// start with some initial colors
//#warning Changed uint16_t to float here!
float MinTemp = 25.0;
float MaxTemp = 35.0;

// variables for interpolated colors
byte red, green, blue;

// variables for row/column interpolation
byte i, j, k, row, col, incr;
float intPoint, val, a, b, c, d, ii;
byte aLow, aHigh;

// size of a display "pixel"
byte BoxWidth = 3;
byte BoxHeight = 3;

int x, y;
char buf[20];

// variable to toggle the display grid
int ShowGrid = -1;
int DefaultTemp = -1;

// array for the 8 x 8 measured pixels
float pixels[64];

// array for the interpolated array
float HDTemp[80][80];

// create the keypad buttons
// note the ILI9438_3t library makes use of the Adafruit_GFX library (which makes use of the Adafruit_button library)
// Adafruit_GFX_Button KeyPadBtn[12];

// create the Camera object
Adafruit_AMG88xx ThermalSensor;

// create the touch screen object
// UTouch  Touch( 2, 3, 4, 5, 6);

// SD Card Variables
// Sd2Card card;
// SdVolume volume;
// SdFile root;

SdFs card;
FsFile root;
#define SD_CS_PIN D4
FsFile image_file;

// Try to select the best SD card configuration.
/*
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(sd_ss, DEDICATED_SPI, SD_SCK_MHZ(16))
#else // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(16))
#endif // HAS_SDIO_CLASS
*/

//#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(16))
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(16))

// File image_file;
bool SD_present = 0;
unsigned long last_image_time = 0;

// Capture Image Button
const int capture_button = D0;
volatile bool get_image = 0;
volatile unsigned long last_image = 0;

// Prototypes
void InterpolateRows();
void InterpolateCols();
void capture_image();
void capture_image_isr();
void save_image_sd();
void print_sd_info();
void drawBattery();
int measureBattery();
void drawMeasurement();
void DrawLegend();
void Getabcd();
void SetTempScale();
uint16_t GetColor(float val);
void DisplayGradient();
void ota_start();
void activate_sd(int state);

void setup()
{
	pinMode(TFT_DC, OUTPUT);
	pinMode(SD_CS_PIN, OUTPUT);
	pinMode(capture_button, INPUT);

	// attachInterrupt(digitalPinToInterrupt(capture_button), capture_image_isr, FALLING); //D0 cant be used for interrupts :(

	// Set A0 to input for battery measurement
	pinMode(A0, INPUT);

	Serial.begin(115200);

	// Start the display and set the background to black
	SPI.begin();

	activate_sd(1);
	Serial.println("\nInitializing SD card...");
	print_sd_info();
	activate_sd(0);

	SPI.setFrequency(80000000L);
	Display.begin();

	Display.setTextFont(4);
	Display.setTextSize(2);
	Display.fillScreen(TFT_BLACK);
	Display.setRotation(0);

	Display.setTextColor(TFT_GREENYELLOW, C_BLACK);
	Display.setCursor(30, 60);
	Display.print("Thermal");

	Display.setCursor(30, 100);
	Display.setTextColor(C_RED, C_BLACK);
	Display.print("Camera");

	Display.setTextSize(1);
	Display.setTextFont(NULL);
	// let sensor boot up
	bool status = ThermalSensor.begin();
	Wire.setClock(400000);
	delay(100);

	// check status and display results
	if (!status)
	{
		while (1)
		{
			// Display.setFont(DroidSans_20);
			Display.setCursor(20, 180);
			Display.setTextColor(C_RED, C_BLACK);
			Display.print("Sensor: FAIL");
			delay(500);
			// Display.setFont(DroidSans_20);
			Display.setCursor(20, 180);
			Display.setTextColor(C_BLACK, C_BLACK);
			Display.print("Sensor: FAIL");
			delay(500);
		}
	}
	else
	{
		// Display.setFont(DroidSans_20);
		Display.setCursor(20, 180);
		Display.setTextColor(C_GREEN, C_BLACK);
		Display.print("Sensor: FOUND");
	}

	// read the camera for initial testing
	ThermalSensor.readPixels(pixels);

	// check status and display results
	if (pixels[0] < 0)
	{
		while (1)
		{
			// Display.setFont(DroidSans_20);
			Display.setCursor(20, 210);
			Display.setTextColor(C_RED, C_BLACK);
			Display.print("Readings: FAIL");
			delay(500);
			// Display.setFont(DroidSans_20);
			Display.setCursor(20, 210);
			Display.setTextColor(C_BLACK, C_BLACK);
			Display.print("Readings: FAIL");
			delay(500);
		}
	}
	else
	{
		// Display.setFont(DroidSans_20);
		Display.setCursor(20, 210);
		Display.setTextColor(C_GREEN, C_BLACK);
		Display.print("Readings: OK");
		Display.setCursor(0, 250);
		Display.setTextFont(4);
		Display.setTextColor(C_RED, C_BLACK);
		Display.setTextSize(1);

#ifdef USE_OTA
		Display.println("HOLD CAPTURE \nBUTTON FOR OTA!");
		delay(2000);
		// Check for capture button and start OTA if it is pressed for at least one second
		if (digitalRead(capture_button) == 0)
		{
			unsigned long ota_press_time = millis();
			while (digitalRead(capture_button) == 0 && (millis() - ota_press_time) < 1000)
			{
				;
			}
			// Check if button is still pressed
			if (digitalRead(capture_button) == 0)
			{
				system_update_cpu_freq(80000000L);
				// Start OTA
				WiFi.mode(WIFI_STA);
				WiFi.begin(ssid, password);
				while (WiFi.waitForConnectResult() != WL_CONNECTED)
				{
					Serial.println("Connection Failed! Running normal program.");
					Display.fillScreen(C_BLACK);
					Display.println("Connection Failed! Running normal program.");
				}
				ArduinoOTA.setPort(8266);
				ArduinoOTA.setHostname("ThermalCamera_ESP8266");
				ota_start();
			}
		}
#endif
	}

	SPI.setFrequency(80000000L);
	Display.fillScreen(C_BLACK);

	// get the cutoff points for the color interpolation routines
	// note this function called when the temp scale is changed
	Getabcd();

	// draw a cute legend with the scale that matches the sensors max and min
	DrawLegend();

	// draw a large white border for the temperature area
	Display.fillRect(10, 10, 220, 220, C_WHITE);
}

void loop()
{

	// if someone touched the screen do something with it
	//  if (Touch.dataAvailable()) {
	//   ProcessTouch();
	// }

	if (get_image)
	{ // Capture Image
		Serial.println("image");

		capture_image();

		last_image = millis();
		get_image = 0;
	}

	if (digitalRead(PIN_INT) == false)
	{
		// Serial.println("low");
		SetTempScale();

		if (millis() - tempTime > 2000)
		{
			measure = !measure;
			tempTime = millis();
			Display.fillRect(0, 300, 100, 16, TFT_BLACK);
		}
	}
	else
	{
		tempTime = millis();
	}

	// read the sensor
	ThermalSensor.readPixels(pixels);

	// now that we have an 8 x 8 sensor array
	// interpolate to get a bigger screen
	InterpolateRows();

	// now that we have row data with 70 columns
	// interpolate each of the 70 columns
	// forget Arduino..no where near fast enough..Teensy at > 72 mhz is the starting point
	InterpolateCols();
	drawMeasurement();
	// display the 70 x 70 array
	DisplayGradient();

	// Update battery everx 30s
	if (batteryTime < millis())
	{
		drawBattery();
		batteryTime = millis() + 500;
	}

	// Frametime: 260ms before and 360ms after image capture
	/*Serial.println("Frametime: ");
	Serial.println(millis() - tempTime2);
	tempTime2 = millis();*/

	// Use this if you can't use intterupts for the image capture button
	if (!digitalRead(capture_button))
	{
		capture_image();
	}
}

void activate_sd(int state) {
	digitalWrite(SD_CS_PIN, state);
}

// interplation function to create 70 columns for 8 rows
void InterpolateRows()
{

	// interpolate the 8 rows (interpolate the 70 column points between the 8 sensor pixels first)
	for (row = 0; row < 8; row++)
	{
		for (col = 0; col < 70; col++)
		{
			// get the first array point, then the next
			// also need to bump by 8 for the subsequent rows
			aLow = col / 10 + (row * 8);
			aHigh = (col / 10) + 1 + (row * 8);
			// get the amount to interpolate for each of the 10 columns
			// here were doing simple linear interpolation mainly to keep performace high and
			// display is 5-6-5 color palet so fancy interpolation will get lost in low color depth
			intPoint = ((pixels[aHigh] - pixels[aLow]) / 10.0);
			// determine how much to bump each column (basically 0-9)
			incr = col % 10;
			// find the interpolated value
			val = (intPoint * incr) + pixels[aLow];
			// store in the 70 x 70 array
			// since display is pointing away, reverse row to transpose row data
			HDTemp[(7 - row) * 10][col] = val;
		}
	}
}

// interplation function to interpolate 70 columns from the interpolated rows
void InterpolateCols()
{

	// then interpolate the 70 rows between the 8 sensor points
	for (col = 0; col < 70; col++)
	{
		for (row = 0; row < 70; row++)
		{
			// get the first array point, then the next
			// also need to bump by 8 for the subsequent cols
			aLow = (row / 10) * 10;
			aHigh = aLow + 10;
			// get the amount to interpolate for each of the 10 columns
			// here were doing simple linear interpolation mainly to keep performace high and
			// display is 5-6-5 color palet so fancy interpolation will get lost in low color depth
			intPoint = ((HDTemp[aHigh][col] - HDTemp[aLow][col]) / 10.0);
			// determine how much to bump each column (basically 0-9)
			incr = row % 10;
			// find the interpolated value
			val = (intPoint * incr) + HDTemp[aLow][col];
			// store in the 70 x 70 array
			HDTemp[row][col] = val;
		}
	}
}

// Not perfect yet, screen flickers (propably normal / to be expected)
// One reason might be that my 240x240 test-screen interprets the data sent to the SD card since it does not have a CS Pin
void capture_image()
{
	// print_sd_info();
	if (SD_present)
	{
		Serial.println("Starting Image Capture!");
		save_image_sd();
	}
	else
	{
		Serial.println("No SD Card!");
	}
	// Running this command ensures a high framerate after taking an image
	// If you have problems while saving to the SD you might want to change "SPI_FULL_SPEED" to "SPI_HALF_SPEED" in save_image() or decrease the SPI Clock rate
	SPI.setFrequency(80000000L);
}

// Sets variable after button was pressed
void capture_image_isr()
{
	if (millis() - last_image >= 400)
	{
		get_image = 1;
	}
}

// Saves the image to SD Card
void save_image_sd()
{

	Serial.println(F("Initializing SD card..."));

	// See if the card is present and can be initialized:
	if (!card.cardBegin(SD_CONFIG))
	{
		Serial.println(F("  SD card failed, or not present!"));
	}

	Serial.println(F("  SD card initialized."));

	// Pick a numbered filename, 00 to 99.
	char filename[15] = "image_##.txt";

	for (uint8_t i = 0; i < 100; i++)
	{
		filename[6] = '0' + i / 10;
		filename[7] = '0' + i % 10;
		if (!card.exists(filename))
		{
			// Use this one!
			break;
		}
	}

	// image_file = SD.open(filename, FILE_WRITE);
	image_file = card.open(filename, FILE_WRITE);
	if (!image_file)
	{
		Serial.print(F("Couldn't create "));
		Serial.println(filename);
	}
	else
	{
		Serial.print(F("Writing to "));
		Serial.println(filename);

		// Save 8x8 Image to SD Card
#ifdef SAVE8X8
		for (int i = 63; i >= 0; i--)
		{
			image_file.print(pixels[i]);

			if (i == 8 || i == 16 || i == 24 || i == 32 || i == 40 || i == 48 || i == 56)
			{
				image_file.println();
			}
			else
			{
				image_file.print(',');
			}
			image_file.println();
			image_file.close();
#endif

			// Save interpolated image
			//			  row  col
			// float HDTemp[80][80]; //BUT ONLY 70x70?

#ifdef SAVE70x70
			for (int row_counter = 0; row_counter < 70; row_counter++)
			{
				for (int column_counter = 69; column_counter >= 0; column_counter--)
				{
					image_file.print(GetColor(HDTemp[row_counter][column_counter]));
					if (column_counter != 0)
					{
						image_file.print(',');
					}
				}
				image_file.println();
			}

			image_file.println();
			image_file.close();
#endif
		}
	}

	// function to display the results
	void DisplayGradient()
	{

		// rip through 70 rows
		for (row = 0; row < 70; row++)
		{

			// fast way to draw a non-flicker grid--just make every 10 pixels 2x2 as opposed to 3x3
			// drawing lines after the grid will just flicker too much
			if (ShowGrid < 0)
			{
				BoxWidth = 3;
			}
			else
			{
				if ((row % 10 == 9))
				{
					BoxWidth = 2;
				}
				else
				{
					BoxWidth = 3;
				}
			}
			// then rip through each 70 cols
			for (col = 0; col < 70; col++)
			{

				// fast way to draw a non-flicker grid--just make every 10 pixels 2x2 as opposed to 3x3
				if (ShowGrid < 0)
				{
					BoxHeight = 3;
				}
				else
				{
					if ((col % 10 == 9))
					{
						BoxHeight = 2;
					}
					else
					{
						BoxHeight = 3;
					}
				}
				// finally we can draw each the 70 x 70 points, note the call to get interpolated color
				Display.fillRect((row * 3) + 15, (col * 3) + 15, BoxWidth, BoxHeight, GetColor(HDTemp[row][col]));

				if (measure == true && row == 36 && col == 36)
				{
					// drawMeasurement();  //Draw after center pixels to reduce flickering
				}
			}
		}
	}

	// my fast yet effective color interpolation routine
	uint16_t GetColor(float val)
	{

		/*
		  pass in value and figure out R G B
		  several published ways to do this I basically graphed R G B and developed simple linear equations
		  again a 5-6-5 color display will not need accurate temp to R G B color calculation

		  equations based on
		  http://web-tech.ga-usa.com/2012/05/creating-a-custom-hot-to-cold-temperature-color-gradient-for-use-with-rrdtool/index.html

		*/

		red = constrain(255.0 / (c - b) * val - ((b * 255.0) / (c - b)), 0, 255);

		if ((val > MinTemp) & (val < a))
		{
			green = constrain(255.0 / (a - MinTemp) * val - (255.0 * MinTemp) / (a - MinTemp), 0, 255);
		}
		else if ((val >= a) & (val <= c))
		{
			green = 255;
		}
		else if (val > c)
		{
			green = constrain(255.0 / (c - d) * val - (d * 255.0) / (c - d), 0, 255);
		}
		else if ((val > d) | (val < a))
		{
			green = 0;
		}

		if (val <= b)
		{
			blue = constrain(255.0 / (a - b) * val - (255.0 * b) / (a - b), 0, 255);
		}
		else if ((val > b) & (val <= d))
		{
			blue = 0;
		}
		else if (val > d)
		{
			blue = constrain(240.0 / (MaxTemp - d) * val - (d * 240.0) / (MaxTemp - d), 0, 240);
		}

		// use the displays color mapping function to get 5-6-5 color palet (R=5 bits, G=6 bits, B-5 bits)
		return Display.color565(red, green, blue);
	}

	// function to automatically set the max / min scale based on adding an offset to the average temp from the 8 x 8 array
	// you could also try setting max and min based on the actual max min
	void SetTempScale()
	{

		if (false)
		{ // DefaultTemp < 0) {
			MinTemp = 25;
			MaxTemp = 35;
			Getabcd();
			DrawLegend();
		}
		else
		{
			MinTemp = 255;
			MaxTemp = 0;

			for (i = 0; i < 64; i++)
			{

				MinTemp = min(MinTemp, pixels[i]);
				MaxTemp = max(MaxTemp, pixels[i]);
			}

			// MaxTemp = MaxTemp + 5.0;
			// MinTemp = MinTemp + 3.0;
			Getabcd();
			DrawLegend();
		}
	}

	// function to get the cutoff points in the temp vs RGB graph
	void Getabcd()
	{

		a = MinTemp + (MaxTemp - MinTemp) * 0.2121;
		b = MinTemp + (MaxTemp - MinTemp) * 0.3182;
		c = MinTemp + (MaxTemp - MinTemp) * 0.4242;
		d = MinTemp + (MaxTemp - MinTemp) * 0.8182;
	}

	// function to draw a cute little legend
	void DrawLegend()
	{

		// my cute little color legend with max and min text
		j = 0;

		float inc = (MaxTemp - MinTemp) / 220.0;

		for (ii = MinTemp; ii < MaxTemp; ii += inc)
		{
			//    Display.drawFastHLine(10, 255 - j++, 30, GetColor(ii));
			Display.drawFastVLine(10 + j++, 263, 30, GetColor(ii));
		}

		Display.setTextFont(4);
		Display.setTextSize(1);
		Display.setCursor(9, 235);
		Display.setTextColor(TFT_WHITE, TFT_BLACK);
		// sprintf(buf, "%d/%d", MinTemp, max_temp_upper);
		//   Display.fillRect(233, 15, 94, 22, C_BLACK);
		//   Display.setFont(Arial_14);
		Display.print(MinTemp);

		Display.setTextSize(1);
		// Display.setFont(Arial_24_Bold);
		Display.setCursor(168, 235);
		Display.setTextColor(TFT_WHITE, TFT_BLACK);
		// sprintf(buf, "%d/%d", max_temp, max_temp_upper);
		//   Display.fillRect(233, 215, 94, 55, C_BLACK);
		//   Display.setFont(Arial_14);
		Display.print(MaxTemp);

		Display.setTextFont(NULL);
	}

	// Draw a circle + measured value
	void drawMeasurement()
	{

		// Mark center measurement
		Display.drawCircle(120, 120, 3, TFT_WHITE);

		// Measure and print center temperature
		centerTemp = pixels[27];
		Display.setCursor(10, 300);
		Display.setTextColor(TFT_WHITE, TFT_BLACK);
		Display.setTextSize(2);
		sprintf(buf, "%s:%.2f SD:", "Temp", centerTemp);
		Display.print(buf);

		// Print info about SD state
		Display.setCursor(175, 300);
		if (SD_present)
		{
			Display.setTextColor(TFT_GREEN);
			Display.print("OK");
		}
		else
		{
			Display.setTextColor(TFT_RED, TFT_BLACK);
			Display.print("X");
		}
	}

	int measureBattery()
	{
		uint16_t adcValue = analogRead(A0);
		int volt = ((3.3 * adcValue) / 1023);
		volt = (volt * 61) / 10;
		/*Serial.println(adcValue);
		Serial.println(volt);*/
		/*Display.setCursor(128, 235);
		Display.setTextColor(TFT_BLUE, ILI9341_BLACK);
		Display.println(adcValue);*/

		return volt;
	}

	// Draw battery symbol (I have disabled it)
	void drawBattery()
	{
		// int volt = measureBattery(); // range from 3.2V - 4.2V
		///*volt = max(volt, 1);
		// volt = min(volt, 10);*/
		///*Serial.print("Battery: ");
		// Serial.println(volt);*/
		///*if (volt < 32) volt = 32;
		// else if (volt > 42) volt = 42;*/

		// int battery_meter = map(volt, 32, 42, 2, 28);

		//// draw battery
		// Display.drawRect(204, 301, 30, 10, C_WHITE);	//Frame
		// Display.fillRect(201, 303, 3, 6, C_WHITE);		//Contact

		//

		// Display.fillRect(205 + 28 - battery_meter, 302 , battery_meter, 8, TFT_GREEN);
		// Display.fillRect(205, 302, 28 - battery_meter, 8, TFT_BLACK);		//delete old

		// float display_voltage = float(volt) / float(10);

		// Display.setCursor(78, 235);
		// Display.setTextColor(TFT_CYAN, ILI9341_BLACK);
		// Display.println(display_voltage);
	}

	void print_sd_info()
	{
		if (!card.cardBegin(SD_CONFIG))
		{
			Serial.println("initialization failed. Things to check:");
			Serial.println("* is a card inserted?");
			Serial.println("* is your wiring correct?");
			Serial.println("* did you change the chipSelect pin to match your shield or module?");
			SD_present = 0;
			return;
		}
		else
		{
			SD_present = 1;
			Serial.println("Wiring is correct and a card is present.");
		}

		// print the type of card
		/*
		Serial.print("\nCard type: not implemented");
		switch (card.type())
		{
		case SD_CARD_TYPE_SD1:
			Serial.println("SD1");
			break;
		case SD_CARD_TYPE_SD2:
			Serial.println("SD2");
			break;
		case SD_CARD_TYPE_SDHC:
			Serial.println("SDHC");
			break;
		default:
			Serial.println("Unknown");
		}

		// Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
		if (!volume.init(card))
		{
			Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
			return;
		}

		// print the type and size of the first FAT-type volume
		uint32_t volumesize;
		Serial.print("\nVolume type is FAT");
		Serial.println(volume.fatType(), DEC);
		Serial.println();

		volumesize = volume.blocksPerCluster(); // clusters are collections of blocks
		volumesize *= volume.clusterCount();	// we'll have a lot of clusters
		volumesize *= 512;						// SD card blocks are always 512 bytes
		Serial.print("Volume size (bytes): ");
		Serial.println(volumesize);
		Serial.print("Volume size (Kbytes): ");
		volumesize /= 1024;
		Serial.println(volumesize);
		Serial.print("Volume size (Mbytes): ");
		volumesize /= 1024;
		Serial.println(volumesize);

		Serial.println("\nFiles found on the card (name, date and size in bytes): ");
		root.openRoot(volume);

		// list all files in the card with date and size
		root.ls(LS_R | LS_DATE | LS_SIZE);
		*/
	}
#ifdef USE_OTA
	void ota_start()
	{
		ArduinoOTA.onStart([]()
						   {
		String type;
		if (ArduinoOTA.getCommand() == U_FLASH) {
			type = "sketch";
		}
		else { // U_SPIFFS
			type = "filesystem";
		}

		// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
		Serial.println("Start updating " + type); });
		ArduinoOTA.onEnd([]()
						 { Serial.println("\nEnd"); });
		ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
							  {
		Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
		Display.setCursor(0, 140);
		Display.setTextColor(TFT_GREEN, TFT_BLACK);
		Display.printf("Progress: %u%%\r", (progress / (total / 100))); });
		ArduinoOTA.onError([](ota_error_t error)
						   {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) {
			Serial.println("Auth Failed");
		}
		else if (error == OTA_BEGIN_ERROR) {
			Serial.println("Begin Failed");
		}
		else if (error == OTA_CONNECT_ERROR) {
			Serial.println("Connect Failed");
		}
		else if (error == OTA_RECEIVE_ERROR) {
			Serial.println("Receive Failed");
		}
		else if (error == OTA_END_ERROR) {
			Serial.println("End Failed");
		} });
		ArduinoOTA.begin();

		Serial.println("Ready");
		Serial.print("IP address: ");
		Serial.println(WiFi.localIP());

		Display.fillScreen(C_BLACK);
		Display.setTextFont(4);
		Display.setTextSize(1);
		Display.setTextColor(TFT_RED);
		Display.setCursor(0, 0);
		Display.println("OTA-Update!");
		Display.println("\nIP:\n");
		Display.println(WiFi.localIP());

		while (true)
		{
			ArduinoOTA.handle();
			delay(10);
		}
	}
#endif