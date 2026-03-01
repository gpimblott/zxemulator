idf_component_register(SRCS "TFT_eSPI.cpp"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES espressif__arduino-esp32)

target_compile_definitions(${COMPONENT_LIB} PUBLIC
    USER_SETUP_LOADED=1
    TFT_ESPI_USER_SETUP_PATH="${CMAKE_CURRENT_LIST_DIR}/User_Setup.h"
)