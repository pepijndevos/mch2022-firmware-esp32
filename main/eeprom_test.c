#include "eeprom.h"

void eeprom_test() {
    EEPROM eeprom = {
        .i2c_bus = 0,
        .i2c_address = 0x50
    };
    uint8_t* buffer = malloc(128);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Failed to malloc buffer for EEPROM test");
        return;
    }
    /*memset(buffer, 0, 128);
    esp_err_t res = eeprom_read(&eeprom, 0, buffer, 128);
    
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read from EEPROM (%d)", res);
        return;
    }
    
    printf("Read result 1: ");
    for (size_t position = 0; position < 128; position++) {
        printf("%02X ", buffer[position]);
    }
    printf("\n");
    
    for (size_t position = 0; position < 128; position++) {
        buffer[position] = position;
    }*/
    
    
    /*sprintf((char*) buffer, "Hello world!");
    
    esp_err_t res = eeprom_write(&eeprom, 0, buffer, 128);
    
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write to EEPROM (%d)", res);
        return;
    }*/
    
    memset(buffer, 0, 128);
    esp_err_t res = eeprom_read(&eeprom, 0, buffer, 128);
    
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read from EEPROM (%d)", res);
        return;
    }
    
    printf("Read result 1: ");
    for (size_t position = 0; position < 128; position++) {
        printf("%02X (%c) ", buffer[position], buffer[position]);
    }
    printf("\n");
}
