/*
A custom `climate` module for ESPHome for controlling Daikin reverse-cycle
air conditioners.

Based on https://github.com/crankyoldgit/IRremoteESP8266

Basic usage in the ESPHome YAML file:

esphome:
 name: <name for your config>
 platform: ESP8266
 board: <your board>
  includes:
    - path_to/daikin_climate.h
  libraries:
    - "IRremoteESP8266"

climate:
- platform: custom
  lambda: |-
    auto daikin_climate = new DaikinClimate();
    App.register_component(daikin_climate);
    return {daikin_climate};
  climates:
    - name: "<name for your climate control>"

TODO:
 - Configure pin in the constructor
 - Set reasonable default temp on startup if empty?
 - Implement the Switch interface to expose PowerfulMode
 - Implement the Fan interface? (Are custom fans supported?)
*/

#include "esphome.h"
#include "IRremoteESP8266.h"
#include "ir_Daikin.h"

// Set the GPIO pin with the IR blaster to use for sending.
IRDaikinESP ac(D2);

class DaikinClimate : public Component, public Climate {
 public:
  void setup() override {
    // This will be called by App.setup()
    ac.begin();
    // Some sensible defaults.
    ac.setFan(kDaikinFanAuto);
    ac.setSwingVertical(true);
    ac.setSwingHorizontal(true);
  }

  ClimateTraits traits() override  {
    auto traits = ClimateTraits();
    traits.set_supports_current_temperature(false);
    traits.set_supports_auto_mode(true);
    traits.set_supports_cool_mode(true);
    traits.set_supports_heat_mode(true);
    traits.set_supports_two_point_target_temperature(false);
    traits.set_supports_away(false);
    traits.set_visual_min_temperature(kDaikinMinTemp);
    traits.set_visual_max_temperature(kDaikinMaxTemp);
    traits.set_visual_temperature_step(1);
    return traits;
  }

  void control(const ClimateCall &call) override {
    if (call.get_mode().has_value()) {
      // User requested mode change
      this->mode = *call.get_mode();
    }
    if (call.get_target_temperature().has_value()) {
      // User requested target temperature change
      this->target_temperature = *call.get_target_temperature();
    }
    this->transmit_state_();
    this->publish_state();
  }
 protected:
  void transmit_state_() {
    ac.on();
    // Handle settings.
    switch (this->mode) {
    case CLIMATE_MODE_AUTO:
      ac.setMode(kDaikinAuto);
      break;
    case CLIMATE_MODE_HEAT:
      ac.setMode(kDaikinHeat);
      break;
    case CLIMATE_MODE_COOL:
      ac.setMode(kDaikinCool);
      break;
    case CLIMATE_MODE_OFF:
    default:
      ac.off();
      break;
    }
    auto t = (uint8_t) roundf(this->target_temperature);
    ac.setTemp(t);
    ac.send();
  }
};