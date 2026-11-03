#include "robo_config.h"
#if ROBO_PRINTER_BLE
#include "ble_printer.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <esp_gattc_api.h>
#include <algorithm>
#include <cstring>

namespace printer_ble {
namespace {
constexpr uint16_t AppId = 0x51;
constexpr uint32_t ConnectMs = 30000, SendMs = 45000, Rate = 9600;
SemaphoreHandle_t mutex;
esp_gatt_if_t interfaceId = ESP_GATT_IF_NONE;
uint8_t target[6]{};
uint16_t connection = 0xffff, handle = 0, serviceStart = 0, serviceEnd = 0;
State state;
bool abandoned = false, pendingWrite = false, writeOk = false, congested = false;
uint8_t buffer[224];
size_t buffered = 0, chunkSize = sizeof(buffer);
uint32_t sendStarted = 0;

struct Guard {
  Guard() { xSemaphoreTake(mutex, portMAX_DELAY); }
  ~Guard() { xSemaphoreGive(mutex); }
};

void fail(const char* error) {
  state.error = error;
  state.phase = "error";
}

void searchService() {
  esp_bt_uuid_t uuid{}; uuid.len = ESP_UUID_LEN_16; uuid.uuid.uuid16 = 0x18f0;
  if (esp_ble_gattc_search_service(interfaceId, connection, &uuid) != ESP_OK) {
    state.connecting = false;
    fail("printer_ble_service_failed");
  }
}

void event(esp_gattc_cb_event_t type, esp_gatt_if_t source, esp_ble_gattc_cb_param_t* p) {
  Guard guard;
  if (type == ESP_GATTC_REG_EVT) {
    if (p->reg.app_id != AppId) return;
    if (p->reg.status == ESP_GATT_OK) interfaceId = source;
    else fail("printer_ble_init_failed");
    return;
  }
  if (interfaceId == ESP_GATT_IF_NONE || (source != interfaceId && source != ESP_GATT_IF_NONE)) return;
  if (type == ESP_GATTC_OPEN_EVT) {
    if (!isPeer(p->open.remote_bda)) return;
    Serial.printf("Printer BLE open status=%u mtu=%u\n", p->open.status, p->open.mtu);
    if (p->open.status != ESP_GATT_OK) {
      state.connecting = false; fail("printer_ble_unavailable"); return;
    }
    if (abandoned || !state.connecting) {
      esp_ble_gattc_close(interfaceId, p->open.conn_id);
      state.connecting = false; return;
    }
    connection = p->open.conn_id;
    state.connected = true;
    state.mtu = p->open.mtu;
    state.phase = "discovering";
    if (esp_ble_gattc_send_mtu_req(interfaceId, connection) != ESP_OK) searchService();
  } else if (type == ESP_GATTC_CFG_MTU_EVT && p->cfg_mtu.conn_id == connection) {
    if (p->cfg_mtu.status == ESP_GATT_OK) state.mtu = p->cfg_mtu.mtu;
    searchService();
  } else if (type == ESP_GATTC_SEARCH_RES_EVT && p->search_res.conn_id == connection) {
    const auto& uuid = p->search_res.srvc_id.uuid;
    if (uuid.len == ESP_UUID_LEN_16 && uuid.uuid.uuid16 == 0x18f0) {
      serviceStart = p->search_res.start_handle; serviceEnd = p->search_res.end_handle;
    }
  } else if (type == ESP_GATTC_SEARCH_CMPL_EVT && p->search_cmpl.conn_id == connection) {
    state.connecting = false;
    if (abandoned) { esp_ble_gattc_close(interfaceId, connection); return; }
    esp_bt_uuid_t uuid{}; uuid.len = ESP_UUID_LEN_16; uuid.uuid.uuid16 = 0x2af1;
    esp_gattc_char_elem_t chars[2]{}; uint16_t count = 2;
    if (p->search_cmpl.status != ESP_GATT_OK || !serviceStart || !serviceEnd ||
        esp_ble_gattc_get_char_by_uuid(interfaceId, connection, serviceStart, serviceEnd,
                                      uuid, chars, &count) != ESP_GATT_OK || count != 1 ||
        !(chars[0].properties & ESP_GATT_CHAR_PROP_BIT_WRITE_NR) || state.mtu < 23) {
      fail("printer_ble_service_failed");
      esp_ble_gattc_close(interfaceId, connection); return;
    }
    handle = chars[0].char_handle;
    state.ready = true; state.phase = "ready"; state.error = "";
    Serial.printf("Printer BLE ready mtu=%u handle=%u\n", state.mtu, handle);
  } else if (type == ESP_GATTC_WRITE_CHAR_EVT && p->write.conn_id == connection && p->write.handle == handle) {
    writeOk = p->write.status == ESP_GATT_OK;
    pendingWrite = false;
    if (!writeOk) fail("printer_ble_write_failed");
  } else if (type == ESP_GATTC_CONGEST_EVT && p->congest.conn_id == connection) {
    congested = p->congest.congested;
  } else if ((type == ESP_GATTC_DISCONNECT_EVT && isPeer(p->disconnect.remote_bda)) ||
             (type == ESP_GATTC_CLOSE_EVT && isPeer(p->close.remote_bda))) {
    Serial.println("Printer BLE disconnected");
    state.connected = state.ready = state.connecting = false;
    connection = 0xffff; handle = 0;
    pendingWrite = false; writeOk = false; congested = false;
    if (!strcmp(state.phase, "sending")) fail("printer_ble_disconnected");
    else state.phase = "disconnected";
  }
}

bool timedOut() { return uint32_t(millis() - sendStarted) >= SendMs; }

bool flush() {
  if (!buffered) return true;
  // Wait for local controller credit. Never resubmit a packet after an
  // ambiguous completion, and never fall back to UART mid-receipt.
  for (;;) {
    {
      Guard guard;
      if (!state.ready || timedOut()) { fail("printer_ble_disconnected"); return false; }
      if (!congested) break;
    }
    vTaskDelay(1);
  }
  {
    Guard guard;
    pendingWrite = true; writeOk = false;
    const auto result = esp_ble_gattc_write_char(interfaceId, connection, handle, buffered,
        buffer, ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
    if (result != ESP_OK) { pendingWrite = false; fail("printer_ble_write_failed"); return false; }
    state.bytes += buffered; // Queued to BLE, not evidence of physical printing.
  }
  const uint32_t queued = millis();
  for (;;) {
    {
      Guard guard;
      if (!pendingWrite) { if (!writeOk || !state.ready) return false; break; }
      if (uint32_t(millis() - queued) >= 3000 || timedOut()) {
        fail("printer_ble_write_timeout"); return false;
      }
    }
    vTaskDelay(1);
  }
  buffered = 0;
  const uint32_t due = (snapshot().bytes * 1000UL + Rate - 1) / Rate;
  while (uint32_t(millis() - sendStarted) < due) vTaskDelay(1);
  return true;
}
}

bool isPeer(const uint8_t* address) { return address && !memcmp(target, address, 6); }

bool begin() {
  unsigned value[6]{};
  if (sscanf(ROBO_PRINTER_BLE_ADDRESS, "%x:%x:%x:%x:%x:%x",
             &value[0], &value[1], &value[2], &value[3], &value[4], &value[5]) != 6) return false;
  bool nonzero = false;
  for (unsigned i = 0; i < 6; ++i) {
    if (value[i] > 255) return false;
    target[i] = value[i]; nonzero |= value[i] != 0;
  }
  if (!nonzero) return false;
  mutex = xSemaphoreCreateMutex();
  if (!mutex) return false;
  BLEDevice::setCustomGattcHandler(event);
  return esp_ble_gattc_app_register(AppId) == ESP_OK;
}

State snapshot() { Guard guard; return state; }

bool startJob() {
  buffered = 0;
  sendStarted = 0;
  const uint32_t started = millis();
  {
    Guard guard;
    state.bytes = 0; state.elapsedMs = 0;
    if (interfaceId == ESP_GATT_IF_NONE || state.connecting) { fail("printer_ble_unavailable"); return false; }
    if (!state.ready) {
      if (state.connected) { fail("printer_ble_unavailable"); return false; }
      abandoned = false; serviceStart = serviceEnd = 0;
      state.connecting = true; state.phase = "connecting"; state.error = "";
      if (esp_ble_gattc_open(interfaceId, target, static_cast<esp_ble_addr_type_t>(ROBO_PRINTER_BLE_ADDRESS_TYPE), true) != ESP_OK) {
        state.connecting = false; fail("printer_ble_unavailable"); return false;
      }
    }
  }
  while (!snapshot().ready) {
    {
      Guard guard;
      if (!state.connecting) return false;
      if (uint32_t(millis() - started) >= ConnectMs) {
        abandoned = true; fail("printer_ble_connect_timeout");
        if (state.connected) esp_ble_gattc_close(interfaceId, connection);
        // IDF 4.4 has no public cancel-open API. A late OPEN is closed in the
        // callback; no later job can reuse this outstanding connect attempt.
        return false;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  {
    Guard guard;
    chunkSize = std::min<size_t>(sizeof(buffer), state.mtu - 3);
    state.phase = "sending"; state.error = "";
    sendStarted = millis();
  }
  return true;
}

bool writeByte(uint8_t value) {
  if (timedOut()) return false;
  buffer[buffered++] = value;
  return buffered < chunkSize || flush();
}

bool finishJob() {
  if (!flush()) return false;
  // Keep the link open for the next ticket; do not repeatedly rediscover and
  // disconnect the printer while the phone remains connected to our server.
  vTaskDelay(pdMS_TO_TICKS(1000));
  Guard guard;
  state.elapsedMs = millis() - sendStarted;
  if (!state.ready) return false;
  state.phase = "ready";
  return true;
}

void abortJob() {
  Guard guard;
  buffered = 0;
  state.elapsedMs = sendStarted ? millis() - sendStarted : 0;
  if (state.bytes && state.connected) esp_ble_gattc_close(interfaceId, connection);
}
}
#endif
