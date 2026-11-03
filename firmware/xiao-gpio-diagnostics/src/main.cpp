#include <Arduino.h>
#include <driver/gpio.h>
#include <soc/gpio_struct.h>
#include <soc/io_mux_reg.h>

namespace {
char command[40];
size_t commandLength = 0;
bool commandOverflow = false;

void snapshot(gpio_num_t pin, const char* mode, const char* phase, int err) {
  const uint32_t mask = 1UL << pin;
  Serial.printf("GPIO_TEST gpio=%d mode=%s phase=%s err=%d input=%d latch=%u output_enabled=%u mux=0x%08lx route=0x%08lx\n",
    int(pin), mode, phase, err, gpio_get_level(pin), unsigned(bool(GPIO.out & mask)),
    unsigned(bool(GPIO.enable & mask)),
    static_cast<unsigned long>(REG_READ(pin == GPIO_NUM_7 ? IO_MUX_GPIO7_REG : IO_MUX_GPIO8_REG)),
    static_cast<unsigned long>(GPIO.func_out_sel_cfg[pin].val));
}

void pulse(gpio_num_t pin, bool native) {
  const uint32_t mask = 1UL << pin;
  const char* mode = native ? "idf" : "arduino";
  int configError = ESP_OK;
  if (native) {
    gpio_hold_dis(pin);
    gpio_config_t config = {};
    config.pin_bit_mask = mask;
    config.mode = GPIO_MODE_INPUT_OUTPUT;
    config.intr_type = GPIO_INTR_DISABLE;
    configError = gpio_config(&config);
  } else {
    pinMode(pin, INPUT | OUTPUT);
  }
  gpio_set_level(pin, 0);
  delay(5);
  snapshot(pin, mode, "low", configError);
  // The user must physically disconnect all loads before issuing the command.
  // Capture while high, but release BEFORE any serial output can block.
  const int writeError = native ? gpio_set_level(pin, 1) : (digitalWrite(pin, HIGH), ESP_OK);
  delay(5);
  const int input = gpio_get_level(pin);
  const bool latch = GPIO.out & mask;
  const bool enabled = GPIO.enable & mask;
  const uint32_t mux = REG_READ(pin == GPIO_NUM_7 ? IO_MUX_GPIO7_REG : IO_MUX_GPIO8_REG);
  const uint32_t route = GPIO.func_out_sel_cfg[pin].val;
  gpio_set_level(pin, 0);
  delay(5);
  Serial.printf("GPIO_TEST gpio=%d mode=%s phase=high err=%d input=%d latch=%u output_enabled=%u mux=0x%08lx route=0x%08lx\n",
    int(pin), mode, writeError, input, unsigned(latch), unsigned(enabled),
    static_cast<unsigned long>(mux), static_cast<unsigned long>(route));
  snapshot(pin, mode, "low_after", ESP_OK);
  gpio_set_direction(pin, GPIO_MODE_INPUT);
}

void runCommand() {
  command[commandLength] = 0;
  if (!commandOverflow && (strcmp(command, "TEST_D8_EMPTY") == 0 || strcmp(command, "TEST_D9_EMPTY") == 0)) {
    const gpio_num_t pin = command[6] == '8' ? GPIO_NUM_7 : GPIO_NUM_8;
    pulse(pin, false);
    pulse(pin, true);
    Serial.println("GPIO_TEST complete; tested pin released to input");
  } else if (commandLength || commandOverflow) {
    Serial.println("GPIO_TEST rejected; requires TEST_D8_EMPTY or TEST_D9_EMPTY with that pin physically disconnected");
  }
  commandLength = 0;
  commandOverflow = false;
}
} // namespace

void setup() {
  // Keep all known motor/heater nets and the active-HIGH pump relay off.
  for (int pin : {2, 3, 39, 11, 12, 42}) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }
  pinMode(7, INPUT);
  pinMode(8, INPUT);
  Serial.begin(115200);
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') runCommand();
    else if (c != '\r') {
      if (commandLength < sizeof(command)-1) command[commandLength++] = c;
      else commandOverflow = true;
    }
  }
  static uint32_t lastStatus = 0;
  if (millis()-lastStatus > 5000) {
    lastStatus = millis();
    Serial.println("GPIO_TEST ready; empty D8/D9 only; no Wi-Fi/BLE, no NVS writes");
  }
  delay(2);
}
