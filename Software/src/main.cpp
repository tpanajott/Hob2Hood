#include <Arduino.h>
#include <string.h>
#include <IRremote.h>
#include <Bounce2.h>

#define PIN_IR_RECV 2
#define PIN_CTRL_EXT_FAN A1

// Pins that control the relays
#define PIN_CTRL_LIGHTS 16 // 16/A2
#define PIN_CTRL_FAN_1 18  // 18/A4
#define PIN_CTRL_FAN_2 19  // 19/A5
#define PIN_CTRL_FAN_3 3
#define PIN_CTRL_FAN_4 4
#define PIN_CTRL_RELAY_UNKNOWN 0

// Pins that read the input from the manual controls
#define PIN_MAN_LIGHTS 5
#define PIN_MAN_FAN_1 8
#define PIN_MAN_FAN_2 7
#define PIN_MAN_FAN_3 9
#define PIN_MAN_FAN_4 10

// Test HEX values
// #define HEX_FAN_OFF 0x180C
// #define HEX_FAN_1 0x5878
// #define HEX_FAN_2 0x587B
// #define HEX_FAN_3 0x5879
// #define HEX_FAN_4 0x587A
// #define HEX_LIGHT_ON 0x5877
// #define HEX_LIGHT_OFF 0x184B

// Real HEX values:
#define HEX_FAN_OFF 0x055303A3
#define HEX_FAN_1 0xE3C01BE2
#define HEX_FAN_2 0xD051C301
#define HEX_FAN_3 0xC22FFFD7
#define HEX_FAN_4 0xB9121B29
#define HEX_LIGHT_ON 0xE208293C
#define HEX_LIGHT_OFF 0x24ACF947

uint8_t manualFanSpeed = 0;
uint8_t lastManualFanSpeed = 0;
bool manualLightState = false;
bool lastManualLightState = false;

// Button debounce instances
Bounce btnLightControl = Bounce();
Bounce btnFan1 = Bounce();
Bounce btnFan2 = Bounce();
Bounce btnFan3 = Bounce();
Bounce btnFan4 = Bounce();

IRrecv irrecv(2); // create instance of 'irrecv'
// decode_results results;

/**
 * @brief Set the Fan Speed. A speed of 0 indicates OFF
 *
 * @param speed
 */
void setFanSpeed(uint8_t speed)
{
  Serial.print("Fan speed: ");
  Serial.println(speed);
  if (speed == 0)
  {
    digitalWrite(PIN_CTRL_FAN_1, LOW);
    digitalWrite(PIN_CTRL_FAN_2, LOW);
    digitalWrite(PIN_CTRL_FAN_3, LOW);
    digitalWrite(PIN_CTRL_FAN_4, LOW);
    digitalWrite(PIN_CTRL_EXT_FAN, LOW);
  }
  else if (speed == 1)
  {
    digitalWrite(PIN_CTRL_FAN_2, LOW);
    digitalWrite(PIN_CTRL_FAN_3, LOW);
    digitalWrite(PIN_CTRL_FAN_4, LOW);
    delay(100);
    digitalWrite(PIN_CTRL_FAN_1, HIGH);
    digitalWrite(PIN_CTRL_EXT_FAN, HIGH);
  }
  else if (speed == 2)
  {
    digitalWrite(PIN_CTRL_FAN_1, LOW);
    digitalWrite(PIN_CTRL_FAN_3, LOW);
    digitalWrite(PIN_CTRL_FAN_4, LOW);
    delay(100);
    digitalWrite(PIN_CTRL_FAN_2, HIGH);
    digitalWrite(PIN_CTRL_EXT_FAN, HIGH);
  }
  else if (speed == 3)
  {
    digitalWrite(PIN_CTRL_FAN_1, LOW);
    digitalWrite(PIN_CTRL_FAN_2, LOW);
    digitalWrite(PIN_CTRL_FAN_4, LOW);
    delay(100);
    digitalWrite(PIN_CTRL_FAN_3, HIGH);
    digitalWrite(PIN_CTRL_EXT_FAN, HIGH);
  }
  else if (speed == 4)
  {
    digitalWrite(PIN_CTRL_FAN_1, LOW);
    digitalWrite(PIN_CTRL_FAN_2, LOW);
    digitalWrite(PIN_CTRL_FAN_3, LOW);
    delay(100);
    digitalWrite(PIN_CTRL_FAN_4, HIGH);
    digitalWrite(PIN_CTRL_EXT_FAN, HIGH);
  }
}

/**
 * @brief Set the Light State
 *
 * @param state
 */
void setLightState(bool state)
{
  Serial.print("Lights: ");
  Serial.println(state ? "ON" : "OFF");
  digitalWrite(PIN_CTRL_LIGHTS, state ? HIGH : LOW);
}

/**
 * @brief Read inputs from manual control button and make decisions based on the input.
 * If ANY input is high, manualControl will be set to true, else, set to false.
 */
void readButtonStates()
{
  btnLightControl.update();
  btnFan1.update();
  btnFan2.update();
  btnFan3.update();
  btnFan4.update();

  bool newManualLightState = lastManualLightState;
  uint8_t newManualFanSpeed = lastManualFanSpeed;

  // Control the state of the light
  if (btnLightControl.rose())
  {
    newManualLightState = true;
    setLightState(true);
  }
  else if (btnLightControl.fell())
  {
    newManualLightState = false;
    setLightState(false);
  }

  if (btnFan1.rose())
  {
    newManualFanSpeed = 1;
    setFanSpeed(1);
  }
  else if (btnFan2.rose())
  {
    newManualFanSpeed = 2;
    setFanSpeed(2);
  }
  else if (btnFan3.rose())
  {
    newManualFanSpeed = 3;
    setFanSpeed(3);
  }
  else if (btnFan4.rose())
  {
    newManualFanSpeed = 4;
    setFanSpeed(4);
  }
  else if (btnFan1.fell() && btnFan1.read() == LOW && btnFan2.read() == LOW && btnFan3.read() == LOW && btnFan4.read() == LOW)
  {
    // Button 1 fell (ie. is no longer pressed) and all the other fan buttons are depressed.
    // This means that no fan speed is currently selected manually.
    setFanSpeed(0);
    newManualFanSpeed = 0;
  }

  if (lastManualLightState != newManualLightState || lastManualFanSpeed != newManualFanSpeed)
  {
    // New values are going to be set.
    if (!newManualLightState && newManualFanSpeed == 0)
    {
      // No manual controls available. Start the IR timer.
      Serial.println("Manual control disabled! Starting IR reciving.");
      irrecv.start();
    }
    else
    {
      Serial.println("Manual control enabled! Stopping IR reciving.");
      irrecv.stop();
    }

    lastManualLightState = newManualLightState;
    lastManualFanSpeed = newManualFanSpeed;

    manualLightState = newManualLightState;
    manualFanSpeed = newManualFanSpeed;
  }
}

void loop()
{
  readButtonStates(); // Read and act according to manual button states.

  // Manual controls are not used AND we received a IR code
  if (manualFanSpeed == 0 && !manualLightState && irrecv.decode())
  {
    // have we received an IR signal?
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
      // Serial.print("Unknown hex: ");
      // Serial.println(irrecv.decodedIRData.decodedRawData, HEX);
      break;
    }

    irrecv.resume(); // receive the next value
  }
}

void setup()
{
  Serial.begin(115200); // for serial monitor output
  Serial.println("Hob2Hood Starting ...");

  // Setup outputs to fan and set default OFF state
  pinMode(PIN_CTRL_LIGHTS, OUTPUT);
  pinMode(PIN_CTRL_FAN_1, OUTPUT);
  pinMode(PIN_CTRL_FAN_2, OUTPUT);
  pinMode(PIN_CTRL_FAN_3, OUTPUT);
  pinMode(PIN_CTRL_FAN_4, OUTPUT);
  pinMode(PIN_CTRL_EXT_FAN, OUTPUT);
  setFanSpeed(0);
  setLightState(false);

  // Setup input from buttons
  btnLightControl.attach(PIN_MAN_LIGHTS, INPUT);
  btnFan1.attach(PIN_MAN_FAN_1, INPUT);
  btnFan2.attach(PIN_MAN_FAN_2, INPUT);
  btnFan3.attach(PIN_MAN_FAN_3, INPUT);
  btnFan4.attach(PIN_MAN_FAN_4, INPUT);

  // Setup button input debounce interval
  btnLightControl.interval(50);
  btnFan1.interval(50);
  btnFan2.interval(50);
  btnFan3.interval(50);
  btnFan4.interval(50);

  // Set default state
  if (btnLightControl.read() == HIGH)
  {
    manualLightState = true;
    lastManualLightState = true;
    setLightState(true);
  }
  else
  {
    manualLightState = false;
    lastManualLightState = false;
    setLightState(false);
  }

  if (btnFan1.read() == HIGH)
  {
    lastManualFanSpeed = 1;
    manualFanSpeed = 1;
    setFanSpeed(1);
  }
  else if (btnFan2.read() == HIGH)
  {
    lastManualFanSpeed = 2;
    manualFanSpeed = 2;
    setFanSpeed(2);
  }
  else if (btnFan3.read() == HIGH)
  {
    lastManualFanSpeed = 3;
    manualFanSpeed = 3;
    setFanSpeed(3);
  }
  else if (btnFan4.read() == HIGH)
  {
    lastManualFanSpeed = 4;
    manualFanSpeed = 4;
    setFanSpeed(4);
  }
  else
  {
    lastManualFanSpeed = 0;
    manualFanSpeed = 0;
    setFanSpeed(0);
  }

  if (!manualLightState && manualFanSpeed == 0)
  {
    Serial.println("Manual control not used. Starting IR receiver.");
    irrecv.enableIRIn();
  }
}