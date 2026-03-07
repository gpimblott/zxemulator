#ifndef BLE_KEYBOARD_HOST_H
#define BLE_KEYBOARD_HOST_H

#include "../../src/spectrum/Processor.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class BleKeyboardHost {
public:
  BleKeyboardHost();
  ~BleKeyboardHost();

  // Initialize the BLE stack, NVS, and start scanning for HID devices
  void init(Processor *proc);

  // Private/static callbacks need access to this instance
  static BleKeyboardHost *instance;

  void handleKeyboardReport(uint8_t *report, uint16_t length);

private:
  Processor *processor;

  // Store the previous report to calculate keyup events
  uint8_t lastReport[8] = {0};

  // Helper to map USB HID usage ID to ZX Spectrum matrix (line, bit)
  bool mapHidToZx(uint8_t hidCode, uint8_t &outLine, uint8_t &outBit);
};

#endif // BLE_KEYBOARD_HOST_H
