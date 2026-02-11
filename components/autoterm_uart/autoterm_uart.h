#pragma once
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/number/number.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/select/select.h"
#include "esphome/core/hal.h"
#include "esphome/core/preferences.h"
#include "esphome/core/helpers.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>
#include <string>
#include <vector>

namespace esphome {
namespace autoterm_uart {

class AutotermUART;      // Forward declaration
class AutotermClimate;   // Forward declaration

// ===================
// Custom Number Class
// ===================
class AutotermFanLevelNumber : public number::Number, public Component {
 public:
  AutotermUART *parent_{nullptr};
  void setup_parent(AutotermUART *p) { parent_ = p; }

 protected:
  void control(float value) override;
};

class AutotermTempSourceSelect : public select::Select, public Component {
 public:
  void set_parent(AutotermUART *parent);
  void publish_for_source(uint8_t source);

 protected:
  void control(const std::string &value) override;

 private:
  AutotermUART *parent_{nullptr};
  const char *option_from_source_(uint8_t source) const;
  uint8_t source_from_option_(const std::string &option) const;
};

// ===================
// Main UART Class
// ===================
class AutotermUART : public Component, public uart::UARTDevice {
  friend class AutotermTempSourceSelect;

 public:
  AutotermUART() = default;
  explicit AutotermUART(uart::UARTComponent *parent) : uart::UARTDevice(parent) {}

  uart::UARTComponent *uart_display_{nullptr};
  uart::UARTComponent *uart_heater_{nullptr};

  // Sensors
  sensor::Sensor *internal_temp_sensor_{nullptr};
  sensor::Sensor *external_temp_sensor_{nullptr};
  sensor::Sensor *heater_temp_sensor_{nullptr};
  sensor::Sensor *panel_temp_sensor_{nullptr};
  sensor::Sensor *voltage_sensor_{nullptr};
  sensor::Sensor *status_sensor_{nullptr};
  sensor::Sensor *fan_speed_set_sensor_{nullptr};
  sensor::Sensor *fan_speed_actual_sensor_{nullptr};
  sensor::Sensor *pump_frequency_sensor_{nullptr};
  text_sensor::TextSensor *status_text_sensor_{nullptr};
  sensor::Sensor *panel_temp_override_sensor_{nullptr};
  float panel_temp_override_value_c_{NAN};
  AutotermTempSourceSelect *temp_source_select_{nullptr};
  bool manual_temp_source_active_{false};
  uint8_t manual_temp_source_value_{0};
  float last_internal_temp_c_{NAN};
  float last_external_temp_c_{NAN};

  AutotermFanLevelNumber *fan_level_number_{nullptr};
  AutotermClimate *climate_{nullptr};
  sensor::Sensor *runtime_hours_sensor_{nullptr};
  sensor::Sensor *session_runtime_sensor_{nullptr};
  ESPPreferenceObject runtime_hours_pref_;
  float runtime_hours_{0.0f};
  float runtime_hours_last_published_{NAN};
  float session_runtime_hours_{0.0f};
  float session_runtime_last_published_{NAN};
  bool runtime_loaded_{false};
  bool runtime_dirty_{false};
  bool runtime_tracking_initialized_{false};
  bool runtime_storage_initialized_{false};
  bool heater_running_{false};
  uint32_t last_runtime_millis_{0};
  uint32_t last_runtime_save_millis_{0};

  struct Settings {
    uint8_t use_work_time = 1;
    uint8_t work_time = 0;
    uint8_t temperature_source = 4;
    uint8_t set_temperature = 16;
    uint8_t wait_mode = 0;
    uint8_t power_level = 8;
  } settings_;
  bool settings_valid_{false};
  bool display_connected_state_{false};
  uint32_t last_display_activity_{0};
  uint32_t last_status_request_millis_{0};
  uint32_t last_settings_request_millis_{0};
  uint32_t last_panel_temp_send_millis_{0};
  float panel_temp_last_value_c_{NAN};
  std::vector<uint8_t> display_to_heater_buffer_;
  std::vector<uint8_t> heater_to_display_buffer_;
  bool thermostat_active_{false};
  bool thermostat_heating_request_{false};
  bool thermostat_waiting_for_idle_{false};
  float thermostat_target_c_{20.0f};
  float thermostat_hys_on_c_{2.0f};
  float thermostat_hys_off_c_{1.0f};
  uint8_t thermostat_level_{4};
  uint8_t thermostat_sensor_source_{1};
  uint8_t thermostat_last_sent_level_{255};
  uint32_t thermostat_last_command_millis_{0};
  uint32_t thermostat_last_evaluation_millis_{0};

  void set_uart_display(uart::UARTComponent *u) { uart_display_ = u; }
  void set_uart_heater(uart::UARTComponent *u) { uart_heater_ = u; }

  // Sensor setters
  void set_internal_temp_sensor(sensor::Sensor *s) { internal_temp_sensor_ = s; }
  void set_external_temp_sensor(sensor::Sensor *s) { external_temp_sensor_ = s; }
  void set_heater_temp_sensor(sensor::Sensor *s) { heater_temp_sensor_ = s; }
  void set_voltage_sensor(sensor::Sensor *s) { voltage_sensor_ = s; }
  void set_status_sensor(sensor::Sensor *s) { status_sensor_ = s; }
  void set_fan_speed_set_sensor(sensor::Sensor *s) { fan_speed_set_sensor_ = s; }
  void set_fan_speed_actual_sensor(sensor::Sensor *s) { fan_speed_actual_sensor_ = s; }
  void set_pump_frequency_sensor(sensor::Sensor *s) { pump_frequency_sensor_ = s; }
  void set_panel_temp_sensor(sensor::Sensor *s) {
    panel_temp_sensor_ = s;
    if (s != nullptr && std::isfinite(panel_temp_last_value_c_)) {
      s->publish_state(panel_temp_last_value_c_);
    }
  }
  void set_status_text_sensor(text_sensor::TextSensor *s) { status_text_sensor_ = s; }
  void set_runtime_hours_sensor(sensor::Sensor *s);
  void set_session_runtime_sensor(sensor::Sensor *s);

  void set_panel_temp_override_sensor(sensor::Sensor *s);
  void set_temp_source_select(AutotermTempSourceSelect *select);
  void set_temp_source_from_select(uint8_t source);
  void apply_temp_source_from_settings(uint8_t source);
  uint8_t get_manual_temp_source() const { return manual_temp_source_active_ ? manual_temp_source_value_ : 0; }
  uint8_t get_effective_temp_source() const;
  float get_temperature_for_source(uint8_t source) const;

  void set_fan_level_number(AutotermFanLevelNumber *n) {
    fan_level_number_ = n;
    if (n) n->setup_parent(this);
  }
  void set_climate(AutotermClimate *climate);

  // Commands for operating modes
  void send_standby();
  void send_power_mode(bool start, uint8_t level);
  void send_temperature_hold_mode(bool start, uint8_t temp_sensor, uint8_t set_temp);
  void send_temperature_to_fan_mode(bool start, uint8_t temp_sensor, uint8_t set_temp);
  void send_fan_only(uint8_t level);
  void configure_thermostat_mode(float target_c, uint8_t level, uint8_t sensor_source,
                                 float hys_on_c, float hys_off_c);
  void disable_thermostat_mode();

  void loop() override {
    forward_and_sniff(uart_display_, uart_heater_, "display→heater", true);
    forward_and_sniff(uart_heater_, uart_display_, "heater→display");

    uint32_t now = millis();
    bool connected = uart_display_ != nullptr && (now - last_display_activity_) < 5000;
    if (connected != display_connected_state_) {
      display_connected_state_ = connected;
      if (connected) {
        ESP_LOGI("autoterm_uart", "Display connection detected");
        last_status_request_millis_ = now;
        last_settings_request_millis_ = now;
        last_panel_temp_send_millis_ = now;
      } else {
        ESP_LOGW("autoterm_uart", "Display connection lost, switching to autonomous mode");
        last_panel_temp_send_millis_ = 0;
      }
    }

    if (!connected) {
      if (now - last_status_request_millis_ >= 2000) {
        send_status_request();
        last_status_request_millis_ = now;
      }
      if (now - last_settings_request_millis_ >= 10000) {
        request_settings();
        last_settings_request_millis_ = now;
      }
      if (should_override_panel_temperature_() && std::isfinite(panel_temp_override_value_c_)) {
        if (last_panel_temp_send_millis_ == 0 || (now - last_panel_temp_send_millis_) >= 1000) {
          send_panel_temperature_override_frame_();
          last_panel_temp_send_millis_ = now;
        }
      }
    }

    uint32_t runtime_now = millis();
    advance_runtime_time_(runtime_now);
    maybe_save_runtime_hours_(runtime_now);

    if (thermostat_active_)
      evaluate_thermostat_control_();
  }

  void setup() override {
    if (global_preferences != nullptr) {
      runtime_hours_pref_ =
          global_preferences->make_preference<float>(fnv1_hash("autoterm_uart_runtime_hours"));
      runtime_storage_initialized_ = true;
      if (!runtime_hours_pref_.load(&runtime_hours_)) {
        runtime_hours_ = 0.0f;
      }
    } else {
      runtime_storage_initialized_ = false;
      runtime_hours_ = 0.0f;
    }
    runtime_loaded_ = true;
    runtime_hours_last_published_ = NAN;
    publish_runtime_hours_(true);
    session_runtime_hours_ = 0.0f;
    session_runtime_last_published_ = NAN;
    publish_session_runtime_(true);

    uint32_t now = millis();
    last_runtime_millis_ = now;
    last_runtime_save_millis_ = now;
    runtime_tracking_initialized_ = true;

    request_settings();
  }

  float get_setup_priority() const override { return setup_priority::DATA; }
  void dump_config() override;

 protected:
  void forward_and_sniff(uart::UARTComponent *from, uart::UARTComponent *to, const char *direction, bool from_display = false);
  void process_display_to_heater_frame_(const std::vector<uint8_t> &frame);
  void process_heater_to_display_frame_(const std::vector<uint8_t> &frame);
  void parse_status_frame_(const std::vector<uint8_t> &frame);
  void parse_settings_frame_(const std::vector<uint8_t> &frame);
  void send_status_request();
  void request_settings();
  void send_panel_temperature_override_frame_();
  bool should_override_panel_temperature_() const;
  void advance_runtime_time_(uint32_t now);
  void maybe_save_runtime_hours_(uint32_t now);
  void publish_runtime_hours_(bool force = false);
  void publish_session_runtime_(bool force = false);
  void evaluate_thermostat_control_();
  uint16_t calculate_checksum_(const std::vector<uint8_t> &data, size_t start, size_t length) const;
  bool verify_checksum_(const std::vector<uint8_t> &frame) const;
  void send_command_(const std::vector<uint8_t> &cmd);
};

// ===================
// Climate Class
// ===================
class AutotermClimate : public climate::Climate, public Component {
 public:
  AutotermClimate() = default;

  void setup() override {}

  void control(const climate::ClimateCall &call) override;
  void set_parent(AutotermUART *parent) { parent_ = parent; }
  void handle_status_update(uint16_t status_code, float current_temp);
  void handle_settings_update(const AutotermUART::Settings &settings, bool from_display);

  void set_default_temp_sensor(uint8_t sensor) { default_temp_sensor_ = sensor; }
  void set_default_level(uint8_t level) { default_level_ = level; }
  void set_default_temperature(float temp) { default_temperature_ = temp; }
  void set_hysteresis_on(float value) { hysteresis_on_c_ = clamp_hysteresis_on_(value); }
  void set_hysteresis_off(float value) { hysteresis_off_c_ = clamp_hysteresis_off_(value); }
  void set_thermostat_hysteresis(float on, float off) {
    hysteresis_on_c_ = clamp_hysteresis_on_(on);
    hysteresis_off_c_ = clamp_hysteresis_off_(off);
  }

  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  climate::ClimateTraits traits() override {
    auto traits = climate::ClimateTraits();
    
    traits.add_supported_mode(climate::CLIMATE_MODE_OFF);
    traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);
    traits.add_supported_mode(climate::CLIMATE_MODE_FAN_ONLY);
    traits.add_supported_mode(climate::CLIMATE_MODE_AUTO);
    
    traits.set_visual_min_temperature(0.0f);
    traits.set_visual_max_temperature(30.0f);
    traits.set_visual_temperature_step(1.0f);
    traits.set_visual_current_temperature_step(0.1f);

    traits.set_supported_custom_presets({
        "Leistungsmodus",
        "Heizen",
        "Heizen+Lüften",
        "Thermostat",
    });

    static std::vector<const char *> fan_modes_vec = {
        "Stufe 0", "Stufe 1", "Stufe 2", "Stufe 3", "Stufe 4",
        "Stufe 5", "Stufe 6", "Stufe 7", "Stufe 8", "Stufe 9"
    };
    traits.set_supported_custom_fan_modes(fan_modes_vec);

    return traits;
  }

 private:
  AutotermUART *parent_{nullptr};
  std::string preset_mode_{"Leistungsmodus"};
  uint8_t fan_level_{4};
  uint8_t default_level_{4};
  float target_temperature_c_{20.0f};
  float default_temperature_{20.0f};
  float current_temperature_c_{NAN};
  uint8_t default_temp_sensor_{1};
  float hysteresis_on_c_{2.0f};
  float hysteresis_off_c_{1.0f};

  uint8_t clamp_level_(int level) const;
  float clamp_temperature_(float temperature) const;
  float clamp_hysteresis_on_(float value) const;
  float clamp_hysteresis_off_(float value) const;
  std::string fan_mode_label_from_level_(uint8_t level) const;
  uint8_t fan_mode_label_to_level_(const std::string &label) const;
  std::string sanitize_preset_(const std::string &preset) const;
  uint8_t resolve_temp_sensor_() const;
  climate::ClimateMode deduce_mode_from_settings_(const AutotermUART::Settings &settings) const;
  std::string deduce_preset_from_settings_(const AutotermUART::Settings &settings) const;
  std::string preset_from_enum_(climate::ClimatePreset preset);
  uint8_t fan_level_from_enum_(climate::ClimateFanMode mode, uint8_t fallback_level);
  void apply_state_(climate::ClimateMode mode, const std::string &preset, uint8_t level, float target_temp);
  void update_action_from_status_(uint16_t status_code);
};

// Implementation
inline void AutotermFanLevelNumber::control(float value) {
  if (parent_ != nullptr) {
    uint8_t level = static_cast<uint8_t>(std::round(value));
    if (level > 9) level = 9;
    parent_->send_fan_only(level);
  }
  this->publish_state(value);
}

inline void AutotermTempSourceSelect::set_parent(AutotermUART *parent) { parent_ = parent; }

inline void AutotermTempSourceSelect::publish_for_source(uint8_t source) {
  const char *opt = option_from_source_(source);
  if (opt != nullptr && opt[0] != '\0') this->publish_state(opt);
}

inline void AutotermTempSourceSelect::control(const std::string &value) {
  uint8_t source = source_from_option_(value);
  if (parent_ != nullptr && source >= 1 && source <= 4) parent_->set_temp_source_from_select(source);
  this->publish_state(value);
}

inline const char *AutotermTempSourceSelect::option_from_source_(uint8_t source) const {
  switch (source) {
    case 1: return "Internal";
    case 2: return "External";
    case 3: return "Heater";
    case 4: return "Manual";
    default: return "";
  }
}

inline uint8_t AutotermTempSourceSelect::source_from_option_(const std::string &option) const {
  if (option == "Internal") return 1;
  if (option == "External") return 2;
  if (option == "Heater") return 3;
  if (option == "Manual") return 4;
  return 1;
}

inline void AutotermClimate::control(const climate::ClimateCall &call) {
  if (parent_ == nullptr) return;

  climate::ClimateMode new_mode = this->mode;
  std::string new_preset = preset_mode_;
  uint8_t new_level = fan_level_;
  float new_target = target_temperature_c_;

  if (call.get_mode().has_value()) new_mode = *call.get_mode();

  const char *custom_preset_ptr = call.get_custom_preset();
  if (custom_preset_ptr != nullptr) {
    new_preset = std::string(custom_preset_ptr);
  } else if (call.get_preset().has_value()) {
    new_preset = preset_from_enum_(*call.get_preset());
  }

  const char *custom_fan_mode_ptr = call.get_custom_fan_mode();
  if (custom_fan_mode_ptr != nullptr) {
    new_level = fan_mode_label_to_level_(std::string(custom_fan_mode_ptr));
  } else if (call.get_fan_mode().has_value()) {
    new_level = fan_level_from_enum_(*call.get_fan_mode(), fan_level_);
  }

  if (call.get_target_temperature().has_value()) new_target = *call.get_target_temperature();

  new_preset = sanitize_preset_(new_preset);
  new_level = clamp_level_(new_level);
  new_target = clamp_temperature_(new_target);

  if (new_mode == climate::CLIMATE_MODE_OFF) {
    parent_->send_standby();
    apply_state_(new_mode, new_preset, new_level, new_target);
    return;
  }

  if (new_mode == climate::CLIMATE_MODE_FAN_ONLY) {
    parent_->send_fan_only(new_level);
    apply_state_(new_mode, new_preset, new_level, new_target);
    return;
  }

  if (new_preset == "Leistungsmodus") {
    parent_->send_power_mode(true, new_level);
    apply_state_(new_mode, new_preset, new_level, new_target);
    return;
  }

  uint8_t sensor = resolve_temp_sensor_();
  uint8_t temp_val = static_cast<uint8_t>(std::round(new_target));

  if (new_preset == "Heizen") {
    parent_->send_temperature_hold_mode(true, sensor, temp_val);
    apply_state_(climate::CLIMATE_MODE_HEAT, new_preset, new_level, new_target);
    return;
  }

  if (new_preset == "Heizen+Lüften") {
    parent_->send_temperature_to_fan_mode(true, sensor, temp_val);
    apply_state_(climate::CLIMATE_MODE_AUTO, new_preset, new_level, new_target);
    return;
  }

  if (new_preset == "Thermostat") {
    parent_->configure_thermostat_mode(new_target, new_level, sensor, hysteresis_on_c_, hysteresis_off_c_);
    apply_state_(climate::CLIMATE_MODE_AUTO, new_preset, new_level, new_target);
    return;
  }

  apply_state_(new_mode, new_preset, new_level, new_target);
}

inline void AutotermClimate::handle_status_update(uint16_t status_code, float current_temp) {
  bool changed = false;
  float prev_temp = current_temperature_c_;
  current_temperature_c_ = current_temp;

  if (std::isnan(prev_temp) != std::isnan(current_temp) ||
      (!std::isnan(current_temp) && std::abs(current_temp - prev_temp) > 0.01f)) {
    this->current_temperature = std::isnan(current_temp) ? NAN : current_temp;
    changed = true;
  }

  climate::ClimateAction previous_action = this->action;
  update_action_from_status_(status_code);
  if (this->action != previous_action) changed = true;

  if (changed) this->publish_state();
}

inline void AutotermClimate::handle_settings_update(const AutotermUART::Settings &settings, bool from_display) {
  if (!from_display) return;
  apply_state_(deduce_mode_from_settings_(settings), deduce_preset_from_settings_(settings),
               clamp_level_(settings.power_level), clamp_temperature_(static_cast<float>(settings.set_temperature)));
}

inline uint8_t AutotermClimate::clamp_level_(int level) const {
  if (level < 0) return 0;
  if (level > 9) return 9;
  return static_cast<uint8_t>(level);
}

inline float AutotermClimate::clamp_temperature_(float temperature) const {
  if (temperature < 0.0f) return 0.0f;
  if (temperature > 30.0f) return 30.0f;
  return temperature;
}

inline float AutotermClimate::clamp_hysteresis_on_(float value) const {
  if (value < 1.0f) return 1.0f;
  if (value > 5.0f) return 5.0f;
  return value;
}

inline float AutotermClimate::clamp_hysteresis_off_(float value) const {
  if (value < 0.0f) return 0.0f;
  if (value > 2.0f) return 2.0f;
  return value;
}

inline std::string AutotermClimate::fan_mode_label_from_level_(uint8_t level) const {
  return "Stufe " + std::to_string(static_cast<int>(clamp_level_(level)));
}

inline uint8_t AutotermClimate::fan_mode_label_to_level_(const std::string &label) const {
  const std::string prefix = "Stufe ";
  if (label.size() <= prefix.size() || label.compare(0, prefix.size(), prefix) != 0) return fan_level_;
  std::string digits = label.substr(prefix.size());
  if (digits.empty()) return fan_level_;
  int value = 0;
  for (char c : digits) {
    if (!std::isdigit(static_cast<unsigned char>(c))) return fan_level_;
    value = value * 10 + (c - '0');
  }
  return clamp_level_(value);
}

inline std::string AutotermClimate::sanitize_preset_(const std::string &preset) const {
  if (preset == "Leistungsmodus" || preset == "Heizen" || preset == "Heizen+Lüften" || preset == "Thermostat")
    return preset;
  return preset_mode_;
}

inline uint8_t AutotermClimate::resolve_temp_sensor_() const {
  if (parent_ != nullptr) {
    uint8_t manual = parent_->get_manual_temp_source();
    if (manual >= 1 && manual <= 4) return manual;
    if (parent_->settings_valid_) {
      uint8_t src = parent_->settings_.temperature_source;
      if (src >= 1 && src <= 4) return src;
    }
  }
  uint8_t sensor = default_temp_sensor_;
  if (sensor < 1) sensor = 1;
  if (sensor > 4) sensor = 4;
  return sensor;
}

inline climate::ClimateMode AutotermClimate::deduce_mode_from_settings_(const AutotermUART::Settings &settings) const {
  if (settings.wait_mode == 0x00 && settings.power_level == 0) return climate::CLIMATE_MODE_OFF;
  if (settings.temperature_source == 0x04) return climate::CLIMATE_MODE_HEAT;
  if (settings.wait_mode == 0x01) return climate::CLIMATE_MODE_AUTO;
  if (settings.wait_mode == 0x02) return climate::CLIMATE_MODE_HEAT;
  return this->mode;
}

inline std::string AutotermClimate::deduce_preset_from_settings_(const AutotermUART::Settings &settings) const {
  if (settings.temperature_source == 0x04) return "Leistungsmodus";
  if (settings.wait_mode == 0x01) return "Heizen+Lüften";
  if (settings.wait_mode == 0x02) return "Heizen";
  return preset_mode_;
}

inline std::string AutotermClimate::preset_from_enum_(climate::ClimatePreset preset) {
  switch (preset) {
    case climate::CLIMATE_PRESET_NONE: return "Leistungsmodus";
    case climate::CLIMATE_PRESET_HOME:
    case climate::CLIMATE_PRESET_COMFORT:
    case climate::CLIMATE_PRESET_SLEEP: return "Heizen";
    case climate::CLIMATE_PRESET_AWAY:
    case climate::CLIMATE_PRESET_ACTIVITY: return "Heizen+Lüften";
    case climate::CLIMATE_PRESET_BOOST: return "Leistungsmodus";
    case climate::CLIMATE_PRESET_ECO: return "Thermostat";
    default: return "";
  }
}

inline uint8_t AutotermClimate::fan_level_from_enum_(climate::ClimateFanMode mode, uint8_t fallback_level) {
  switch (mode) {
    case climate::CLIMATE_FAN_OFF: return 0;
    case climate::CLIMATE_FAN_LOW: return 1;
    case climate::CLIMATE_FAN_MEDIUM: return 4;
    case climate::CLIMATE_FAN_MIDDLE: return 5;
    case climate::CLIMATE_FAN_HIGH: return 7;
    case climate::CLIMATE_FAN_ON: return 9;
    case climate::CLIMATE_FAN_FOCUS: return 8;
    case climate::CLIMATE_FAN_DIFFUSE: return 3;
    case climate::CLIMATE_FAN_QUIET: return 2;
    case climate::CLIMATE_FAN_AUTO: return fallback_level;
    default: return fallback_level;
  }
}

inline void AutotermClimate::apply_state_(climate::ClimateMode mode, const std::string &preset, uint8_t level, float target_temp) {
  preset_mode_ = sanitize_preset_(preset);
  fan_level_ = clamp_level_(level);
  target_temperature_c_ = clamp_temperature_(target_temp);

  this->mode = mode;
  this->preset.reset();
  
  // Directly set the custom_preset_ member variable
  if (mode != climate::CLIMATE_MODE_FAN_ONLY && mode != climate::CLIMATE_MODE_OFF && !preset_mode_.empty()) {
    this->custom_preset_ = preset_mode_.c_str();
  } else {
    this->custom_preset_ = nullptr;
  }
  
  this->fan_mode.reset();
  
  // Directly set the custom_fan_mode_ member variable
  std::string fan_label = fan_mode_label_from_level_(fan_level_);
  if (!fan_label.empty()) {
    // Store in a static variable to ensure the pointer remains valid
    static std::string stored_fan_label;
    stored_fan_label = fan_label;
    this->custom_fan_mode_ = stored_fan_label.c_str();
  } else {
    this->custom_fan_mode_ = nullptr;
  }
  
  this->target_temperature = target_temperature_c_;
  this->current_temperature = std::isnan(current_temperature_c_) ? NAN : current_temperature_c_;

  if (mode == climate::CLIMATE_MODE_OFF)
    this->action = climate::CLIMATE_ACTION_OFF;
  else if (mode == climate::CLIMATE_MODE_FAN_ONLY)
    this->action = climate::CLIMATE_ACTION_FAN;
  else
    this->action = climate::CLIMATE_ACTION_HEATING;

  this->publish_state();
}

inline void AutotermClimate::update_action_from_status_(uint16_t status_code) {
  climate::ClimateAction action = climate::CLIMATE_ACTION_IDLE;
  switch (status_code) {
    case 0x0000: case 0x0001:
      action = this->mode == climate::CLIMATE_MODE_OFF ? climate::CLIMATE_ACTION_OFF : climate::CLIMATE_ACTION_IDLE;
      break;
    case 0x0101: case 0x0323:
      action = climate::CLIMATE_ACTION_FAN;
      break;
    case 0x0200: case 0x0201: case 0x0202: case 0x0203: case 0x0204: case 0x0300:
      action = climate::CLIMATE_ACTION_HEATING;
      break;
    case 0x0304: case 0x0400:
      action = climate::CLIMATE_ACTION_IDLE;
      break;
    default:
      if (this->mode == climate::CLIMATE_MODE_OFF)
        action = climate::CLIMATE_ACTION_OFF;
      else if (this->mode == climate::CLIMATE_MODE_FAN_ONLY)
        action = climate::CLIMATE_ACTION_FAN;
      else
        action = climate::CLIMATE_ACTION_IDLE;
  }
  this->action = action;
}

inline void AutotermUART::set_climate(AutotermClimate *climate) {
  climate_ = climate;
  if (climate_ != nullptr) {
    climate_->set_parent(this);
    if (settings_valid_) climate_->handle_settings_update(settings_, false);
  }
}

}  // namespace autoterm_uart
}  // namespace esphome
