#include "interrupt_pwm.h"
#include "esphome/core/log.h"

extern "C" {
#include "user_interface.h"
}

namespace esphome {
namespace interrupt_pwm {

static const char *const TAG = "interrupt_pwm";

// Initialize static members
PWMChannel InterruptPWMOutput::channels_[MAX_PWM_CHANNELS] = {};
uint32_t InterruptPWMOutput::pwm_period_ticks_ = 100;  // ticks per cycle (=PWM levels)
volatile uint32_t InterruptPWMOutput::tick_counter_ = 0;
bool InterruptPWMOutput::timer_initialized_ = false;

void InterruptPWMOutput::setup() {
  ESP_LOGI(TAG, "========== SETUP START ==========");
  ESP_LOGI(TAG, "Channel ID: %u", this->channel_id_);
  ESP_LOGI(TAG, "GPIO Pin: %u", this->pin_);
  
  if (this->channel_id_ >= MAX_PWM_CHANNELS) {
    ESP_LOGE(TAG, "ERROR: Channel ID %u exceeds maximum of %u", this->channel_id_, MAX_PWM_CHANNELS);
    this->mark_failed();
    return;
  }

  // Test GPIO pin
  ESP_LOGI(TAG, "Setting pinMode for GPIO%u to OUTPUT", this->pin_);
  pinMode(this->pin_, OUTPUT);
  
  // Register this channel
  channels_[this->channel_id_].pin = this->pin_;
  channels_[this->channel_id_].duty_ticks = 0;
  channels_[this->channel_id_].active = true;
  
  ESP_LOGI(TAG, "Channel %u registered", this->channel_id_);
  
  // Initialize Timer1 only once (first channel)
  if (!timer_initialized_) {
    ESP_LOGI(TAG, "Initializing Timer1...");
    
    // Disable timer first
    timer1_disable();
    
    // Initialize and attach interrupt
    timer1_isr_init();
    timer1_attachInterrupt(InterruptPWMOutput::timer_isr);
    
    timer1_write(PWM_TICK_PERIOD);
    
    // Enable timer
    timer1_enable(TIM_DIV1, TIM_EDGE, TIM_LOOP);
    
    timer_initialized_ = true;
  }
  
  ESP_LOGI(TAG, "========== SETUP COMPLETE ==========");
}

void InterruptPWMOutput::write_state(float state) {
  if (this->channel_id_ >= MAX_PWM_CHANNELS) {
    return;
  }
  
  if (state < 0.0f) state = 0.0f;
  if (state > 1.0f) state = 1.0f;
  
  uint32_t new_duty = static_cast<uint32_t>(state * pwm_period_ticks_);
  
  //ESP_LOGD(TAG, "Ch%u: %.0f%% (duty=%u/%u)", this->channel_id_, state * 100.0f, new_duty, pwm_period_ticks_);
  
  // Update atomically
  channels_[this->channel_id_].duty_ticks = new_duty;
}

void IRAM_ATTR InterruptPWMOutput::timer_isr() {
  // ISR fires every 100 microseconds
  
  // Increment tick counter
  tick_counter_++;
  if (tick_counter_ >= pwm_period_ticks_) {
    tick_counter_ = 0;
  }
  
  // Process all channels
  for (uint8_t i = 0; i < MAX_PWM_CHANNELS; i++) {
    if (!channels_[i].active) continue;

	//edge cases - full off / full on
	if (channels_[i].duty_ticks == 0) 
	{
		digitalWrite(channels_[i].pin, LOW);
		continue;
	}
	if (channels_[i].duty_ticks >= pwm_period_ticks_) 
	{
		digitalWrite(channels_[i].pin, HIGH);
		continue;
	}
    
    //for odd channels, PWM pulse is bound to the end of the cycle rather that to the start
	if (i % 2) {
		if (tick_counter_ == pwm_period_ticks_ - channels_[i].duty_ticks - 1) {
		  // Start of cycle
			digitalWrite(channels_[i].pin, HIGH);
		} else if (tick_counter_ == pwm_period_ticks_ - 1) {
		  // End of duty cycle
			digitalWrite(channels_[i].pin, LOW);
		}
	} else {
		if (tick_counter_ == 0) {
		  // Start of cycle
			digitalWrite(channels_[i].pin, HIGH);
		} else if (tick_counter_ == channels_[i].duty_ticks) {
		  // End of duty cycle
			digitalWrite(channels_[i].pin, LOW);
		}
	}
  }
  
  // Reload timer - CRITICAL!
  timer1_write(PWM_TICK_PERIOD); 
}

}  // namespace interrupt_pwm
}  // namespace esphome
