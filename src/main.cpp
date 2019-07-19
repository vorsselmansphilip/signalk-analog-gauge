#include <Arduino.h>

#include "sensesp_app.h"
#include "transforms/linear.h"
#include "devices/analog_input.h"
#include "devices/digital_input.h"

#include "DFRobot_ST7687S_Latch.h"

#include "displays/AnalogGauge.h"
#include "transforms/analogvoltage.h"
#include "transforms/voltagedividerR2.h"
#include "transforms/temperatureinterpreter.h"
#include "transforms/kelvintocelsius.h"
#include "transforms/kelvintofahrenheit.h"
#include "transforms/debounce.h"
#include "signalk/signalk_output.h"

const char* sk_path = "electrical.generator.engine.waterTemp";

const float Vin = 3.3; // Voltage sent into the voltage divider circuit that includes the analog sender
const float R1 = 1000.0; // The resistance, in Ohms, of the R1 resitor in the analog sender divider circuit


const float minAnalogGaugeVal = 327.594; // Minimum value to display on analog gauge
const float maxAnalogGaugeVal = 377.594; // Max value to display on analog gauge

// Wiring configuration and setup for TFT display
#define Display DFRobot_ST7687S_Latch
uint8_t pin_cs = D2, pin_rs = D1, pin_wr = D3, pin_lck = D4;
uint8_t button_pin = D8;

ReactESP app([] () {

#ifdef ARDUINO
  #ifndef SERIAL_DEBUG_DISABLED
  Serial.begin(115200);

  // A small arbitrary delay is required to let the
  // serial port catch up

  delay(1000);
  Debug.setSerialEnabled(true);
  #endif
#endif

  sensesp_app = new SensESPApp();

  // Initialize the LCD display
  Display* pDisplay = new Display(pin_cs, pin_rs, pin_wr, pin_lck);
  pDisplay->begin();
  AnalogGauge *pGauge = new AnalogGauge(pDisplay, minAnalogGaugeVal, maxAnalogGaugeVal, "/gauge/display");
  pGauge->addValueRange(AnalogGauge::ValueColor(349.817, 360.928, DISPLAY_GREEN));
  pGauge->addValueRange(AnalogGauge::ValueColor(360.928, 369.261, DISPLAY_YELLOW));
  pGauge->addValueRange(AnalogGauge::ValueColor(369.261, maxAnalogGaugeVal, DISPLAY_RED));

  AnalogInput* pAnalogInput = new AnalogInput();
  DigitalInputValue* pButton = new DigitalInputValue(button_pin, INPUT, CHANGE);
  

  Linear* pTempInKelvin;
  AnalogVoltage* pVoltage;

  pAnalogInput->connectTo(pVoltage = new AnalogVoltage()) ->
                connectTo(new VoltageDividerR2(R1, Vin, "/gauge/inputs")) ->
                connectTo(new TemperatureInterpreter("/gauge/temp_curve")) ->
                connectTo(pTempInKelvin = new Linear(1.0, 0.0, "/gauge/final_temp/calibrate")) ->
                connectTo(new SKOutputNumber(sk_path, "/gauge/final_temp/sk")) ->
                connectTo(pGauge);
  pGauge->setValueSuffix('k', 0);


  pTempInKelvin->connectTo(new KelvinToFahrenheit()) -> connectTo(pGauge, 1);
  pGauge->setValueSuffix('f', 1);
  pGauge->setDefaultDisplayIndex(1);
  
  
  pTempInKelvin->connectTo(new KelvinToCelsius()) -> connectTo(pGauge, 2);
  pGauge->setValueSuffix('c', 2);
    
  pVoltage->connectTo(pGauge, 3);
  pGauge->setValueSuffix('v', 3);

  pButton->connectTo(new Debounce()) -> connectTo(pGauge);

  sensesp_app->enable();

});
