#include "interrupt_pwm.h"
#include "esphome/core/log.h"

#ifdef USE_ESP8266
extern "C" {
#include "user_interface.h"
}
#endif

namespace esphome {
namespace interrupt_pwm {

static const char *const TAG = "interrupt_pwm";

// Initialize static members
PWMChannel InterruptPWMOutput::channels_[MAX_PWM_CHANNELS] = {};
uint32_t InterruptPWMOutput::pwm_period_ticks_ = 100;  // ticks per cycle (=PWM levels)
volatile uint32_t InterruptPWMOutput::tick_counter_ = 0;
volatile bool InterruptPWMOutput::timer_initialized_ = false;

void InterruptPWMOutput::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Interrupt PWM Output...");
  ESP_LOGCONFIG(TAG, "  Channel ID: %u", this->channel_id_);
  ESP_LOGCONFIG(TAG, "  GPIO Pin: %u", this->pin_);
  
  if (this->channel_id_ >= MAX_PWM_CHANNELS) {
    ESP_LOGE(TAG, "Channel ID %u exceeds maximum of %u", this->channel_id_, MAX_PWM_CHANNELS);
    this->mark_failed();
    return;
  }

  // Configure GPIO pin directly
  pinMode(this->pin_, OUTPUT);
  digitalWrite(this->pin_, LOW);
  
  // Register this channel
  noInterrupts();
  channels_[this->channel_id_].pin = this->pin_;
  channels_[this->channel_id_].duty_ticks = 0;
  channels_[this->channel_id_].active = true;
  channels_[this->channel_id_].pin_state = false;
  interrupts();
  
  ESP_LOGCONFIG(TAG, "  Channel %u registered successfully", this->channel_id_);
  
  // Initialize Timer1 only once (first channel)
  if (!timer_initialized_) {
    ESP_LOGCONFIG(TAG, "Initializing Timer1...");
    
    // Disable timer first
    timer1_disable();
    
    // Small delay to ensure timer is stopped
    delayMicroseconds(10);
    
    // Initialize and attach interrupt
    timer1_isr_init();
    timer1_attachInterrupt(InterruptPWMOutput::timer_isr);
    
    // Set initial timer value
    timer1_write(PWM_TICK_PERIOD);
    
    // Enable timer with divider 1, edge mode, auto-reload
    timer1_enable(TIM_DIV1, TIM_EDGE, TIM_LOOP);
    
    timer_initialized_ = true;
    ESP_LOGCONFIG(TAG, "  Timer1 initialized successfully");
  }
}

void InterruptPWMOutput::dump_config() {
  ESP_LOGCONFIG(TAG, "Interrupt PWM Output:");
  ESP_LOGCONFIG(TAG, "  Pin: GPIO%u", this->pin_);
  ESP_LOGCONFIG(TAG, "  Channel: %u", this->channel_id_);
}

void InterruptPWMOutput::write_state(float state) {
  if (this->channel_id_ >= MAX_PWM_CHANNELS) {
    return;
  }
  
  // Clamp state to valid range
  if (state < 0.0f) state = 0.0f;
  if (state > 1.0f) state = 1.0f;
  
  uint32_t new_duty = static_cast<uint32_t>(state * pwm_period_ticks_);
  
  // Update atomically
  noInterrupts();
  channels_[this->channel_id_].duty_ticks = new_duty;
  interrupts();
}

void IRAM_ATTR InterruptPWMOutput::timer_isr() {
  // Increment tick counter
  tick_counter_++;
  if (tick_counter_ >= pwm_period_ticks_) {
    tick_counter_ = 0;
  }
  
  // Process all channels
  for (uint8_t i = 0; i < MAX_PWM_CHANNELS; i++) {
    if (!channels_[i].active) continue;

    bool new_state = false;
    
    // Edge cases - full off / full on
    if (channels_[i].duty_ticks == 0) {
      new_state = false;
    } else if (channels_[i].duty_ticks >= pwm_period_ticks_) {
      new_state = true;
    } else {
      // For odd channels, PWM pulse is bound to the end of the cycle
      if (i & 1) {
        // Odd channel
        if (tick_counter_ >= (pwm_period_ticks_ - channels_[i].duty_ticks)) {
          new_state = true;
        }
      } else {
        // Even channel
        if (tick_counter_ < channels_[i].duty_ticks) {
          new_state = true;
        }
      }
    }
    
    // Only write if state changed
    if (new_state != channels_[i].pin_state) {
      channels_[i].pin_state = new_state;
      // Direct register access for speed
      if (new_state) {
        GPOS = (1 << channels_[i].pin);
      } else {
        GPOC = (1 << channels_[i].pin);
      }
    }
  }
  
  // Reload timer - CRITICAL!
  timer1_write(PWM_TICK_PERIOD); 
}

}  // namespace interrupt_pwm
}  // namespace esphome