#include <Arduino.h>
#include <Wire.h>
#include <driver/gpio.h>
#include <soc/gpio_struct.h>
#include <soc/gpio_periph.h>
#include <soc/io_mux_reg.h>

namespace {
char command[40];
size_t commandLength = 0;
bool commandOverflow = false;

void snapshot(gpio_num_t pin, const char* mode, const char* phase, int err) {
  const uint64_t mask = 1ULL << pin;
  const uint64_t output = (uint64_t(GPIO.out1.val) << 32) | GPIO.out;
  const uint64_t enabled = (uint64_t(GPIO.enable1.val) << 32) | GPIO.enable;
  Serial.printf("GPIO_TEST gpio=%d mode=%s phase=%s err=%d input=%d latch=%u output_enabled=%u mux=0x%08lx route=0x%08lx\n",
    int(pin), mode, phase, err, gpio_get_level(pin), unsigned(bool(output & mask)),
    unsigned(bool(enabled & mask)),
    static_cast<unsigned long>(REG_READ(GPIO_PIN_MUX_REG[pin])),
    static_cast<unsigned long>(GPIO.func_out_sel_cfg[pin].val));
}

void pulse(gpio_num_t pin, bool native) {
  const uint64_t mask = 1ULL << pin;
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
  const bool latch = (((uint64_t(GPIO.out1.val) << 32) | GPIO.out) & mask) != 0;
  const bool enabled = (((uint64_t(GPIO.enable1.val) << 32) | GPIO.enable) & mask) != 0;
  const uint32_t mux = REG_READ(GPIO_PIN_MUX_REG[pin]);
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
  if (!commandOverflow && (strcmp(command, "TEST_D8_EMPTY") == 0 || strcmp(command, "TEST_D9_EMPTY") == 0 ||
                          strcmp(command, "TEST_D3_EMPTY") == 0 || strcmp(command, "TEST_D7_EMPTY") == 0)) {
    const gpio_num_t pin = command[6] == '8' ? GPIO_NUM_7 : command[6] == '9' ? GPIO_NUM_8 :
                          command[6] == '7' ? GPIO_NUM_44 : GPIO_NUM_4;
    pulse(pin, false);
    pulse(pin, true);
    Serial.println("GPIO_TEST complete; tested pin released to input");
  } else if (!commandOverflow && strcmp(command, "PROBE_BARE_I2C") == 0) {
    // Empty address probes only: no sensor configuration or measurement writes.
    const bool started = Wire.begin(5, 6, 100000);
    Wire.setTimeOut(30);
    if (started) {
      for (uint8_t address : {uint8_t(0x38), uint8_t(0x44), uint8_t(0x69), uint8_t(0x6a)}) {
        Wire.beginTransmission(address);
        const unsigned result = Wire.endTransmission();
        Serial.printf("GPIO_TEST i2c=0x%02x status=%u ack=%u\n", address, result, unsigned(result == 0));
      }
      Wire.end();
    } else Serial.println("GPIO_TEST i2c_init_failed");
    pinMode(5, INPUT); pinMode(6, INPUT);
    Serial.println("GPIO_TEST complete; I2C probe finished");
  } else if (commandLength || commandOverflow) {
    Serial.println("GPIO_TEST rejected; use documented commands only with the tested pins disconnected");
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
