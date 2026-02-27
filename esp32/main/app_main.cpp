#include <stdio.h>
#include "spectrum/Processor.h"
#include "spectrum/video/VideoBuffer.h"

extern "C" void app_main(void)
{
    printf("Starting ZX Spectrum Emulator on ESP32!\n");

    // Initialize the core Z80 processor
    Processor processor;
    
    // In a real ESP-IDF build, this ROM would be loaded from SPIFFS/SD Card
    // or embedded directly into the flash via an assembly binary blob.
    // For now, we will just prove the Processor compiles and instantiates.
    
    printf("Processor instantiated successfully. Memory allocated.\n");
    
    // We would initialize our ESP32 Screen driver here that takes the 
    // VideoBuffer and pushes it via DMA to the SPI screen.
    // auto* videoBuffer = processor.getVideoBuffer();

    printf("ESP32 Build Integration Complete.\n");
}
