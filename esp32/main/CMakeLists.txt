# We need to compile our C++ emulator code which is located in ../../src

idf_component_register(
    SRCS
        "app_main.cpp"
        "ESP32Screen.cpp"
        "../../src/utils/Logger.cpp"
        "../../src/utils/BinaryFileLoader.cpp"
        "../../src/utils/RegisterUtils.cpp"
        "../../src/spectrum/Rom.cpp"
        "../../src/spectrum/Memory.cpp"
        "../../src/spectrum/Processor.cpp"
        "../../src/spectrum/Processor_Index.cpp"
        "../../src/spectrum/Processor_Extended.cpp"
        "../../src/spectrum/Processor_Ops.cpp"
        "../../src/spectrum/video/VideoBuffer.cpp"
        "../../src/utils/PeriodTimer.cpp"
        "../../src/spectrum/ProcessorState.cpp"
        "../../src/spectrum/Tape.cpp"
        "../../src/utils/TZXLoader.cpp"
        "../../src/spectrum/Keyboard.cpp"
        "../../src/spectrum/SnapshotLoader.cpp"
        "../../src/spectrum/TapeLoader.cpp"
        "BleKeyboardHost.cpp"
        "esp_hid_gap.c"

    INCLUDE_DIRS
        "."
        "../../src"
        
    REQUIRES 
        # Added TFT_eSPI dependency for hardware display
        tft_espi
        esp_hid
        bt
        nvs_flash
        driver

    EMBED_FILES
        "gw03.rom"
)

# ESP-IDF compiling requires C++17 for our emulator
set_source_files_properties(
    "../../src/utils/Logger.cpp"
    "../../src/spectrum/Processor.cpp"
    PROPERTIES COMPILE_FLAGS "-std=gnu++17"
)
