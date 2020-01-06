#include <Arduino.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#include <AutoPID.h>

#include <EEPROM.h>
int tempStoreAddr = 0;

#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(128, 64, &Wire, -1); // -1 = no reset pin
//#if (SSD1306_LCDHEIGHT != 64)
//#error("Height incorrect, please fix Adafruit_SSD1306.h!");
//#endif

// PID
#define KP .12
#define KI .0003
#define KD 0

#define RELAYS 8

double initTemp = 26.0;
double setPoint = initTemp;
double temperature;
double pulseWidth = 10000.00;
bool relayState = false;
AutoPIDRelay myPID(&temperature, &setPoint, &relayState, pulseWidth, KP, KI, KD);
unsigned long lastTempUpdate; //tracks clock time of last temp update

unsigned long time;
unsigned long displayUpdateDelta = 500;

//reset: plus and minus at same time
uint8_t BPlusPin = 4;
uint8_t BMinusPin = 5;
uint8_t BSavePin = 6;
int bPlusState;
int bMinusState;
int bSaveState;
bool resetPressed;

double increment = 0.1;

void setup(void)
{
  time = millis();

  pinMode(8, OUTPUT);
  pinMode(BPlusPin, INPUT_PULLUP);
  pinMode(BMinusPin, INPUT_PULLUP);
  pinMode(BSavePin, INPUT_PULLUP);

  // start serial port
  Serial.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");
  // Start up the library
  sensors.begin();

  double storedTemp;
  EEPROM.get(tempStoreAddr, storedTemp);
  setPoint = (storedTemp > 0) ? storedTemp : initTemp;

  //display
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(27, 30);
  display.print("Hello, world!");
  display.display();

  myPID.setTimeStep(1000);
}

void renderDisplay(void)
{
  display.clearDisplay();
  display.setCursor(10, 0);
  display.print("Temp: ");
  display.print(temperature);

  display.setCursor(10, 20);
  display.print("Desired: ");
  display.print(setPoint);

  display.setCursor(10, 40);
  display.print("Heat?: ");
  display.print(relayState);
  display.display();
}

void evaluateButtons()
{
  int tmpBPlusState = digitalRead(BPlusPin);
  int tmpBMinusState = digitalRead(BMinusPin);
  int tmpBSaveState = digitalRead(BSavePin);

  // reset combo was pressed and is now released
  if (resetPressed && tmpBPlusState == LOW && tmpBMinusState == LOW)
  {
    resetPressed = false;
    setPoint = initTemp;
  }
  else if (tmpBPlusState == HIGH && tmpBMinusState == HIGH)
  {
    resetPressed = true;
  }
  else if (bMinusState == HIGH && tmpBMinusState == LOW)
  {
    setPoint -= increment;
  }
  else if (bPlusState == HIGH && tmpBPlusState == LOW)
  {
    setPoint += increment;
  }
  else if (bSaveState == HIGH && tmpBSaveState == LOW)
  {
    EEPROM.put(tempStoreAddr, setPoint);
  }

  bPlusState = tmpBPlusState;
  bMinusState = tmpBMinusState;
  bSaveState = tmpBSaveState;
}

void loop(void)
{

  evaluateButtons();

  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  /********************************************************************/
  //Serial.print(" Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperature readings
  //Serial.print("DONE");
  //Serial.println(sensors.getResolution());
  /********************************************************************/
  //Serial.print("Temperature is: ");
  temperature = sensors.getTempCByIndex(0);

  myPID.run();
  if (!relayState)
  {
    digitalWrite(RELAYS, HIGH);
  }
  else
  {
    digitalWrite(RELAYS, LOW);
  }

  if (millis() >= time) {
    time = millis() + displayUpdateDelta;
    renderDisplay();
  }

}
