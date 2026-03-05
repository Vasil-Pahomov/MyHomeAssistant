#include "interrupt_pwm.h"
#include "esphome/core/log.h"

namespace esphome {
namespace interrupt_pwm {

static const char *const TAG = "interrupt_pwm";

PWMChannel InterruptPWMOutput::channels_[MAX_PWM_CHANNELS] = {};
volatile uint32_t InterruptPWMOutput::current_max_period_ = 100; 
volatile uint32_t InterruptPWMOutput::tick_counter_ = 0;
volatile bool InterruptPWMOutput::timer_initialized_ = false;

void InterruptPWMOutput::write_state(float state) {
  if (this->channel_id_ >= MAX_PWM_CHANNELS) return;
  
  state = clamp(state, 0.0f, 1.0f);
  uint32_t duty = 0;
  uint32_t target_max_period = 100; // Default 1000Hz (100 * 10µs)
  

  //ESP_LOGCONFIG(TAG, "Channel ID: %u, state %f", this->channel_id_, state);

  if (state <= 0.0f) {
    duty = 0;
  } /*else if (state < 0.01f) {
	// --- STEADY FREQUENCY REDUCTION LOGIC ---
    // Below 1%, we keep the pulse at 1 tick (10µs) and stretch the period.
    // 0.01 (1%) -> Period 100 (1000Hz)
    // 0.001 (0.1%) -> Period 1000 (100Hz)

	//use logic only if the other complement channel has minimum duty
	uint32_t complementary_channel_duty = 
	  (this->channel_id_ & 1) 
	  ? channels_[this->channel_id_-1].duty_ticks
      : channels_[this->channel_id_+1].duty_ticks;
    
    ESP_LOGCONFIG(TAG, "  Compl channel duty: %u", complementary_channel_duty);

	if (complementary_channel_duty <= 1)
	{
		// Calculate required period to achieve 'state' with a 1-tick pulse:
		// state = duty / period  =>  period = 1 / state
		float calc_period = 1.0f / state;
		target_max_period = static_cast<uint32_t>(clamp(calc_period, 100.0f, 1000.0f));
		duty = 1; 
	} else {
		// Standard 1000Hz PWM when complementary channel is not set
		duty = static_cast<uint32_t>(state * 100.0f);
		target_max_period = 100;
	}
	
  } */else {
    // Standard 1000Hz PWM for brightness >= 1%
    duty = static_cast<uint32_t>(state * 100.0f);
    target_max_period = 100;
  }

  //ESP_LOGCONFIG(TAG, "PWM period: %u, channel %u, duty %d", target_max_period, this->channel_id_, duty);

  noInterrupts();
  current_max_period_ = target_max_period; 
  channels_[this->channel_id_].duty_ticks = duty;
  channels_[this->channel_id_].active = (duty > 0);
  interrupts();
}

void IRAM_ATTR InterruptPWMOutput::timer_isr() {
  tick_counter_++;
  
  if (tick_counter_ >= current_max_period_) {
    tick_counter_ = 0;
  }
  
  for (uint8_t i = 0; i < MAX_PWM_CHANNELS; i++) {
    if (!channels_[i].active) continue;

    bool new_state = false;
    uint32_t duty = channels_[i].duty_ticks;
    
    if (duty == 0) {
      new_state = false;
    } else if (duty >= current_max_period_) {
      new_state = true;
    } else {
      // Offset Even/Odd channels to balance H-Bridge load
      if (i & 1) { 
        if (tick_counter_ >= (current_max_period_ - duty)) new_state = true;
      } else { 
        if (tick_counter_ < duty) new_state = true;
      }
    }
    
    if (new_state != channels_[i].pin_state) {
      channels_[i].pin_state = new_state;
      if (new_state) GPOS = (1 << channels_[i].pin); //
      else GPOC = (1 << channels_[i].pin);           //
    }
  }
  
  timer1_write(800); 
}

// ... setup() and dump_config() remain as in original file
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

}  // namespace interrupt_pwm
}  // namespace esphome