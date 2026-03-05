#pragma once

#include "esphome/core/component.h"
#include "esphome/components/output/float_output.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace interrupt_pwm {

#define MAX_PWM_CHANNELS 8
// 800 ticks = 10µs at 80MHz. This is our base resolution.
#define PWM_TICK_PERIOD 800 

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
  // This must be volatile because it is updated in write_state 
  // and read inside the IRAM_ATTR timer_isr.
  static volatile uint32_t current_max_period_; 
  static volatile uint32_t tick_counter_;
  static volatile bool timer_initialized_;
};

}  // namespace interrupt_pwm
}  // namespace esphome