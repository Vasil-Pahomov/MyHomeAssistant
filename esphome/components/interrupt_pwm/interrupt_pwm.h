#pragma once

#include "esphome/core/component.h"
#include "esphome/components/output/float_output.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace interrupt_pwm {

#define MAX_PWM_CHANNELS 8
#define PWM_TICK_PERIOD 800 // At 80 MHz, 800 ticks = 10 µs

struct PWMChannel {
  uint8_t pin;
  volatile uint32_t duty_ticks;
  volatile bool active;
  volatile bool pin_state;
};

class InterruptPWMOutput : public output::FloatOutput, public Component {
 public:
  explicit InterruptPWMOutput(uint8_t pin, uint8_t channel_id) 
    : pin_(pin), channel_id_(channel_id) {}

  void setup() override;
  void dump_config() override;
  void write_state(float state) override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  static void IRAM_ATTR timer_isr();

 protected:
  uint8_t pin_;
  uint8_t channel_id_;

  static PWMChannel channels_[MAX_PWM_CHANNELS];
  static uint32_t pwm_period_ticks_;
  static volatile uint32_t tick_counter_;
  static volatile bool timer_initialized_;
};

}  // namespace interrupt_pwm
}  // namespace esphome
