#include <Arduino.h>
#include <string.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <Mcp320x.h>
#include <IRremote.h>

#define PIN_IR_RECV 2
#define PIN_EXT_FAN A1

#define PIN_LIGHTS A5
#define PIN_FAN_1 1
#define PIN_FAN_2 A4
#define PIN_FAN_3 3
#define PIN_FAN_4 4
#define PIN_RELAY_UNKNOWN 0

#define PIN_MCP_CS 10
#define PIN_MCP_VREF 5000
#define MCP_READ_INTERVAL_MS 250
#define MCP_CHANNEL_LIGHTS 0
#define MCP_CHANNEL_FAN_1 3
#define MCP_CHANNEL_FAN_2 2
#define MCP_CHANNEL_FAN_3 4
#define MCP_CHANNEL_FAN_4 6
#define MCP_CHANNEL_UNKNOWN 1

#define HEX_FAN_OFF     0x055303A3
#define HEX_FAN_1       0xE3C01BE2
#define HEX_FAN_2       0xD051C301
#define HEX_FAN_3       0xC22FFFD7
#define HEX_FAN_4       0xB9121B29
#define HEX_LIGHT_ON    0xE208293C
#define HEX_LIGHT_OFF   0x24ACF947

SoftwareSerial debugSerial(5, 6); // PD5 RX, PD6 TX

MCP3208 adc(PIN_MCP_VREF, PIN_MCP_CS);
uint16_t analogButtonThresholds[6] = {3000, 3000, 3000, 3000, 3000, 3000}; // Values in mV
uint16_t analogButtonValues[6];
uint8_t manualFanSpeed = 0;
bool manualLightState = false;
unsigned long lastMCPRead = 0;

IRrecv irrecv(2); // create instance of 'irrecv'
decode_results results;

// Set fan speed, a speed of 0 = fan off
void setFanSpeed(uint8_t speed)
{
  debugSerial.print("Fan speed: "); debugSerial.println(speed);
  if (speed == 0)
  {
    digitalWrite(PIN_FAN_1, LOW);
    digitalWrite(PIN_FAN_2, LOW);
    digitalWrite(PIN_FAN_3, LOW);
    digitalWrite(PIN_FAN_4, LOW);
    digitalWrite(PIN_EXT_FAN, LOW);
  }
  else if (speed == 1)
  {
    digitalWrite(PIN_FAN_2, LOW);
    digitalWrite(PIN_FAN_3, LOW);
    digitalWrite(PIN_FAN_4, LOW);
    delay(100);
    digitalWrite(PIN_FAN_1, HIGH);
    digitalWrite(PIN_EXT_FAN, HIGH);
  }
  else if (speed == 2)
  {
    digitalWrite(PIN_FAN_1, LOW);
    digitalWrite(PIN_FAN_3, LOW);
    digitalWrite(PIN_FAN_4, LOW);
    delay(100);
    digitalWrite(PIN_FAN_2, HIGH);
    digitalWrite(PIN_EXT_FAN, HIGH);
  }
  else if (speed == 3)
  {
    digitalWrite(PIN_FAN_1, LOW);
    digitalWrite(PIN_FAN_2, LOW);
    digitalWrite(PIN_FAN_4, LOW);
    delay(100);
    digitalWrite(PIN_FAN_3, HIGH);
    digitalWrite(PIN_EXT_FAN, HIGH);
  }
  else if (speed == 4)
  {
    digitalWrite(PIN_FAN_1, LOW);
    digitalWrite(PIN_FAN_2, LOW);
    digitalWrite(PIN_FAN_3, LOW);
    delay(100);
    digitalWrite(PIN_FAN_4, HIGH);
    digitalWrite(PIN_EXT_FAN, HIGH);
  }
}

// Set the light state
void setLightState(bool state)
{
  debugSerial.print("Lights: "); debugSerial.println(state ? "ON" : "OFF");
  digitalWrite(PIN_LIGHTS, state ? HIGH : LOW);
}

void readButtonStates()
{
  analogButtonValues[0] = adc.toAnalog(adc.read(MCP3208::Channel::SINGLE_0));
  analogButtonValues[1] = adc.toAnalog(adc.read(MCP3208::Channel::SINGLE_1));
  analogButtonValues[2] = adc.toAnalog(adc.read(MCP3208::Channel::SINGLE_2));
  analogButtonValues[3] = adc.toAnalog(adc.read(MCP3208::Channel::SINGLE_3));
  analogButtonValues[4] = adc.toAnalog(adc.read(MCP3208::Channel::SINGLE_4));
  analogButtonValues[5] = adc.toAnalog(adc.read(MCP3208::Channel::SINGLE_5));
  analogButtonValues[6] = adc.toAnalog(adc.read(MCP3208::Channel::SINGLE_6));

  if (analogButtonValues[MCP_CHANNEL_LIGHTS] >= analogButtonThresholds[MCP_CHANNEL_LIGHTS])
  {
    manualLightState = true;
  }
  else
  {
    manualLightState = false;
  }

  if (analogButtonValues[MCP_CHANNEL_FAN_1] >= analogButtonThresholds[MCP_CHANNEL_FAN_1])
  {
    manualFanSpeed = 1;
  }
  else if (analogButtonValues[MCP_CHANNEL_FAN_2] >= analogButtonThresholds[MCP_CHANNEL_FAN_2])
  {
    manualFanSpeed = 2;
  }
  else if (analogButtonValues[MCP_CHANNEL_FAN_3] >= analogButtonThresholds[MCP_CHANNEL_FAN_3])
  {
    manualFanSpeed = 3;
  }
  else if (analogButtonValues[MCP_CHANNEL_FAN_4] >= analogButtonThresholds[MCP_CHANNEL_FAN_2])
  {
    manualFanSpeed = 4;
  }
  else
  {
    manualFanSpeed = 0;
  }
}

// Receive and decode IR commands and control hood upon received command
void receiveIRCommand()
{
  // have we received an IR signal?
  // if (irrecv.decode(&results))
  if (irrecv.decode())
  {
    // debugSerial.println("Received IR command: ");
    // debugSerial.println(results.value, HEX); // display it on serial monitor in hexadecimal
    switch (irrecv.decodedIRData.decodedRawData)
    {
      case HEX_LIGHT_ON:
        setLightState(true);
        break;
      case HEX_LIGHT_OFF:
        setLightState(false);
        break;
      case HEX_FAN_OFF:
        setFanSpeed(0);
        break;
      case HEX_FAN_1:
        setFanSpeed(1);
        break;
      case HEX_FAN_2:
        setFanSpeed(2);
        break;
      case HEX_FAN_3:
        setFanSpeed(3);
        break;
      case HEX_FAN_4:
        setFanSpeed(4);
        break;
    
      default:
        debugSerial.print("Unknown hex: "); Serial.println(irrecv.decodedIRData.decodedRawData, HEX);
        break;
    }

    irrecv.resume(); // receive the next value
  }
}

void loop()
{
  receiveIRCommand();

  if (millis() - lastMCPRead >= MCP_READ_INTERVAL_MS)
  {
    readButtonStates();
  }
}

void setup()
{
  // Setup outputs to fan and set default OFF state
  pinMode(PIN_LIGHTS, OUTPUT);
  pinMode(PIN_FAN_1, OUTPUT);
  pinMode(PIN_FAN_2, OUTPUT);
  pinMode(PIN_FAN_3, OUTPUT);
  pinMode(PIN_FAN_4, OUTPUT);
  setFanSpeed(0);
  setLightState(false);

  // Setup input from buttons
  pinMode(PIN_MCP_CS, OUTPUT);
  digitalWrite(PIN_MCP_CS, HIGH);

  // Setup SPI
  SPISettings settings(1600000, MSBFIRST, SPI_MODE0);
  SPI.begin();
  SPI.beginTransaction(settings);

  // Serial.begin(115200); // for serial monitor output
  debugSerial.begin(115200); // for serial monitor output
  debugSerial.println("Hob2Hood Starting ...");

  // Serial.println(" ... Setup IR receiver");
  irrecv.enableIRIn(); // Start the IR receiver
  // Serial.println("Hob2Hood ready ...");
}