/*
 * BleKeyboardHost.cpp
 *
 * Uses the official ESP-IDF BLE HID Host pattern:
 *  esp_hid_gap_init -> esp_ble_gattc_register_callback -> esp_hidh_init ->
 * esp_hid_scan
 */
#include "BleKeyboardHost.h"
#include "esp_hid_gap.h"
#include <cstring>
#include <esp_bt.h>
#include <esp_bt_defs.h>
#include <esp_bt_main.h>
#include <esp_event.h>
#include <esp_gap_ble_api.h>
#include <esp_gattc_api.h>
#include <esp_hidh.h>
#include <esp_log.h>
#include <nvs_flash.h>

static const char *TAG = "BLE_KEYBOARD";

BleKeyboardHost *BleKeyboardHost::instance = nullptr;

// ZX Spectrum Keyboard Matrix coordinates
struct ZxKeyMap {
  uint8_t line;
  uint8_t bit;
};

// USB HID Usage ID -> ZX Spectrum matrix (line, bit)
static const ZxKeyMap hidToZxMap[] = {
    {0, 0}, // 0x00 - Reserved
    {0, 0}, // 0x01 - ErrorRollOver
    {0, 0}, // 0x02 - POSTFail
    {0, 0}, // 0x03 - ErrorUndefined
    {1, 0}, // 0x04 - 'a'
    {7, 4}, // 0x05 - 'b'
    {0, 3}, // 0x06 - 'c'
    {1, 2}, // 0x07 - 'd'
    {2, 2}, // 0x08 - 'e'
    {1, 3}, // 0x09 - 'f'
    {1, 4}, // 0x0A - 'g'
    {6, 4}, // 0x0B - 'h'
    {5, 2}, // 0x0C - 'i'
    {6, 3}, // 0x0D - 'j'
    {6, 2}, // 0x0E - 'k'
    {6, 1}, // 0x0F - 'l'
    {7, 2}, // 0x10 - 'm'
    {7, 3}, // 0x11 - 'n'
    {5, 1}, // 0x12 - 'o'
    {5, 0}, // 0x13 - 'p'
    {2, 0}, // 0x14 - 'q'
    {2, 3}, // 0x15 - 'r'
    {1, 1}, // 0x16 - 's'
    {2, 4}, // 0x17 - 't'
    {5, 3}, // 0x18 - 'u'
    {0, 4}, // 0x19 - 'v'
    {2, 1}, // 0x1A - 'w'
    {0, 2}, // 0x1B - 'x'
    {5, 4}, // 0x1C - 'y'
    {0, 1}, // 0x1D - 'z'
    {3, 0}, // 0x1E - '1'
    {3, 1}, // 0x1F - '2'
    {3, 2}, // 0x20 - '3'
    {3, 3}, // 0x21 - '4'
    {3, 4}, // 0x22 - '5'
    {4, 4}, // 0x23 - '6'
    {4, 3}, // 0x24 - '7'
    {4, 2}, // 0x25 - '8'
    {4, 1}, // 0x26 - '9'
    {4, 0}, // 0x27 - '0'
    {6, 0}, // 0x28 - Return
    {0, 0}, // 0x29 - Escape (unmapped)
    {4, 0}, // 0x2A - Backspace (handled as CAPS+0)
    {0, 0}, // 0x2B - Tab (unmapped)
    {7, 0}, // 0x2C - Space
};

static void hidh_callback(void *handler_args, esp_event_base_t base, int32_t id,
                          void *event_data) {
  esp_hidh_event_t event = (esp_hidh_event_t)id;
  esp_hidh_event_data_t *param = (esp_hidh_event_data_t *)event_data;

  switch (event) {
  case ESP_HIDH_OPEN_EVENT:
    if (param->open.status == ESP_OK) {
      const uint8_t *bda = esp_hidh_dev_bda_get(param->open.dev);
      if (bda) {
        ESP_LOGI(TAG,
                 "HID Device Connected: %02x:%02x:%02x:%02x:%02x:%02x (%s)",
                 bda[0], bda[1], bda[2], bda[3], bda[4], bda[5],
                 esp_hidh_dev_name_get(param->open.dev));
        esp_hidh_dev_dump(param->open.dev, stdout);
      }
    } else {
      ESP_LOGE(TAG, "HID Device Connection Failed");
    }
    break;
  case ESP_HIDH_BATTERY_EVENT:
    ESP_LOGI(TAG, "HID Battery: %d%%", param->battery.level);
    break;
  case ESP_HIDH_INPUT_EVENT:
    if (BleKeyboardHost::instance &&
        param->input.usage == ESP_HID_USAGE_KEYBOARD) {
      BleKeyboardHost::instance->handleKeyboardReport(param->input.data,
                                                      param->input.length);
    }
    break;
  case ESP_HIDH_CLOSE_EVENT:
    ESP_LOGI(TAG, "HID Device Disconnected");
    break;
  default:
    break;
  }
}

// Runs in a FreeRTOS task — allows RTOS to schedule Bluedroid internals
static void ble_hid_task(void *arg) {
  // Wait a bit to give Bluedroid time to fully stabilize
  vTaskDelay(pdMS_TO_TICKS(500));

  ESP_LOGI(TAG, "Initializing HID Host...");
  esp_hidh_config_t config = {
      .callback = hidh_callback,
      .event_stack_size = 4096,
      .callback_arg = nullptr,
  };
  ESP_ERROR_CHECK(esp_hidh_init(&config));
  ESP_LOGI(TAG, "HID Host ready. Scanning for BLE HID devices...");

  // Scan repeatedly — esp_hid_scan is blocking for the duration
  while (true) {
    size_t results_len = 0;
    esp_hid_scan_result_t *results = nullptr;
    ESP_LOGD(TAG, "Starting 10s BLE scan...");
    esp_hid_scan(10, &results_len, &results);
    ESP_LOGD(TAG, "Scan complete: %u device(s) found", results_len);

    if (results_len > 0) {
      esp_hid_scan_result_t *r = results;
      while (r) {
        ESP_LOGD(TAG,
                 "  Found: %02x:%02x:%02x:%02x:%02x:%02x  RSSI=%d  Usage=%s  "
                 "Name=%s",
                 r->bda[0], r->bda[1], r->bda[2], r->bda[3], r->bda[4],
                 r->bda[5], r->rssi, esp_hid_usage_str(r->usage),
                 r->name ? r->name : "(unnamed)");

        // Open the first keyboard found
        if (r->usage == ESP_HID_USAGE_KEYBOARD ||
            r->usage == ESP_HID_USAGE_GENERIC) {
          ESP_LOGI(TAG, "Connecting to HID keyboard...");
          esp_hidh_dev_open(r->bda, r->transport, r->ble.addr_type);
          break;
        }
        r = r->next;
      }
      esp_hid_scan_results_free(results);
    }
    // Brief delay before next scan
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

BleKeyboardHost::BleKeyboardHost() : processor(nullptr) { instance = this; }
BleKeyboardHost::~BleKeyboardHost() { instance = nullptr; }

void BleKeyboardHost::init(Processor *proc) {
  this->processor = proc;
  ESP_LOGI(TAG, "Initializing BLE HID Host");

  // NVS already initialized by initArduino() — OK to ignore re-init
  nvs_flash_init();

  // Required by esp_hidh
  esp_event_loop_create_default();

  // Initialize BLE (Bluedroid-only, no Classic BT)
  ESP_LOGI(TAG, "Running esp_hid_gap_init...");
  ESP_ERROR_CHECK(esp_hid_gap_init(HIDH_BLE_MODE));

  // Register GATTC callback required by esp_hidh BLE implementation
  ESP_ERROR_CHECK(
      esp_ble_gattc_register_callback(esp_hidh_gattc_event_handler));

  // Spawn task so RTOS can schedule BLE stack internals
  xTaskCreate(ble_hid_task, "ble_hid", 8192, this, 5, nullptr);
}

bool BleKeyboardHost::mapHidToZx(uint8_t hidCode, uint8_t &outLine,
                                 uint8_t &outBit) {
  if (hidCode >= 0x04 && hidCode <= 0x2C) {
    outLine = hidToZxMap[hidCode].line;
    outBit = hidToZxMap[hidCode].bit;
    return true;
  }
  return false;
}

void BleKeyboardHost::handleKeyboardReport(uint8_t *report, uint16_t length) {
  if (!processor || length < 8)
    return;
  Keyboard *kb = processor->getKeyboard();

  bool isShift = (report[0] & 0x02) || (report[0] & 0x20);
  kb->setKey(0, 0, isShift);

  for (int i = 2; i < 8; i++) {
    uint8_t oldKey = lastReport[i];
    if (oldKey == 0)
      continue;
    bool still = false;
    for (int j = 2; j < 8; j++)
      if (report[j] == oldKey) {
        still = true;
        break;
      }
    if (!still) {
      uint8_t l, b;
      if (mapHidToZx(oldKey, l, b))
        kb->setKey(l, b, false);
    }
  }

  for (int i = 2; i < 8; i++) {
    if (report[i] == 0)
      continue;
    uint8_t l, b;
    if (mapHidToZx(report[i], l, b))
      kb->setKey(l, b, true);
  }

  // Backspace = CAPS SHIFT + 0
  for (int i = 2; i < 8; i++)
    if (report[i] == 0x2A) {
      kb->setKey(0, 0, true);
      kb->setKey(4, 0, true);
    }
  for (int i = 2; i < 8; i++) {
    if (lastReport[i] != 0x2A)
      continue;
    bool still = false;
    for (int j = 2; j < 8; j++)
      if (report[j] == 0x2A)
        still = true;
    if (!still) {
      if (!isShift)
        kb->setKey(0, 0, false);
      kb->setKey(4, 0, false);
    }
  }

  memcpy(lastReport, report, 8);
}
