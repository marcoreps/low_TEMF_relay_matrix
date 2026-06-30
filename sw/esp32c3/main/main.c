#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "MATRIX_CTRL";

// Hardware Pin Definitions
#define PIN_NUM_MISO -1
#define PIN_NUM_MOSI GPIO_NUM_3  // DATA
#define PIN_NUM_CLK  GPIO_NUM_1  // SRCLK
#define PIN_NUM_LATCH GPIO_NUM_0 // RCLK

// Matrix Configuration
#define NUM_SHIFT_REGS 12        // 10 for rows, 2 for columns
#define PULSE_DURATION_uS 1000     // Choose minimal reliable value

// Lets abuse SPI for shifting out data
spi_device_handle_t spi;

// Array to hold all outgoing bits
uint8_t sr_data[NUM_SHIFT_REGS];

void fast_boot_flush(void);
void init_hardware(void);
void clear_shift_registers(void);
void update_shift_registers(void);
void set_driver(uint8_t sr_index, uint8_t driver_idx, bool enable_driver, bool control_high);
void process_command(const char* cmd);

void console_rx_task(void *pvParameters);

void app_main(void) {
    fast_boot_flush();
    
    ESP_LOGI(TAG, "Initializing the Matrix ...");
    init_hardware();

    clear_shift_registers();
    update_shift_registers();

    ESP_LOGI(TAG, "Matrix Ready. Listening but not mirroring ...");

    xTaskCreate(console_rx_task, "console_rx_task", 4096, NULL, 10, NULL);
}

void console_rx_task(void *pvParameters) {
    char buf[16];
    int idx = 0;

    while (1) {
        int c = getchar();

        if (c == EOF || c == 0xFF) {
            vTaskDelay(pdMS_TO_TICKS(10)); 
            continue;
        }

        if (c == '\r' || c == '\n') {
            if (idx > 0) {
                buf[idx] = '\0'; 
                
                for(int i = 0; buf[i]; i++) buf[i] = toupper((unsigned char)buf[i]);
                
                if (strlen(buf) == 4)
                    process_command(buf);
                else
                    ESP_LOGE(TAG, "Command '%s' invalid. Must be exactly 4 characters.", buf);
                idx = 0; 
            }
        } 
        else if (idx < sizeof(buf) - 1)
            buf[idx++] = c;
    }
}

void fast_boot_flush(void) {
    gpio_set_direction(PIN_NUM_MOSI, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_CLK, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_LATCH, GPIO_MODE_OUTPUT);

    gpio_set_level(PIN_NUM_LATCH, 0);
    gpio_set_level(PIN_NUM_MOSI, 1);
    
    // Flushes the bucket brigade to guarantee all shift registers are packed with 1s
    for (int i = 0; i < 100; i++) {
        gpio_set_level(PIN_NUM_CLK, 1);
        esp_rom_delay_us(1);
        gpio_set_level(PIN_NUM_CLK, 0);
        esp_rom_delay_us(1);
    }
    
    gpio_set_level(PIN_NUM_LATCH, 1);
}

void init_hardware(void) {
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32 
    };
    
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 100 * 1000, // with 96 kHz we could still get 1ms pulses on any pin
        .mode = 0,
        .spics_io_num = -1,
        .queue_size = 1,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi));
}

void set_driver(uint8_t sr_index, uint8_t driver_index, bool enable_driver, bool control_high) {
    uint8_t sleep_bit, control_bit;
    
    switch(driver_index) {
        case 0: sleep_bit = 6; control_bit = 1; break; // Driver 1: QG/QB
        case 1: sleep_bit = 7; control_bit = 5; break; // Driver 2: QH/QF
        case 2: sleep_bit = 3; control_bit = 4; break; // Driver 3: QD/QE
        case 3: sleep_bit = 0; control_bit = 2; break; // Driver 4: QA/QC
        default: return;
    }

    if (enable_driver)
        sr_data[sr_index] &= ~(1 << sleep_bit); // 0 = Driver Enabled
    else
        sr_data[sr_index] |=  (1 << sleep_bit); // probably never needed but eh

    if (control_high)
        sr_data[sr_index] |=  (1 << control_bit);
    else
        sr_data[sr_index] &= ~(1 << control_bit);
}

void process_command(const char* cmd) {
    int row = (cmd[0] - '0') * 10 + (cmd[1] - '0');
    char col_char = cmd[2];
    char flip_char = cmd[3];
    
    if (row < 1 || row > 40) {
        ESP_LOGE(TAG, "Row error. Must be 01-40.");
        return;
    }

    int col_idx = 0;
    if (col_char >= 'A' && col_char <= 'D')
        col_idx = col_char - 'A';
    else {
        ESP_LOGE(TAG, "Column error. Must be A-D.");
        return;
    }

    bool do_flip = false;
    if (flip_char == '1')
        do_flip = true;
    else if (flip_char == '0')
        do_flip = false;
    else {
        ESP_LOGE(TAG, "Action error. Must be 1 (close relay / connect) or 0 (open relay / disconnect).");
        return;
    }

    int row_idx = row - 1; 
    int row_sr_index = row_idx / 4; 
    int row_driver = row_idx % 4;


    const uint8_t col_plus_sr[4]   = {10, 11, 10, 11};
    const uint8_t col_plus_drv[4]  = { 0,  1,  3,  3};
    
    const uint8_t col_minus_sr[4]  = {11, 10, 11, 10};
    const uint8_t col_minus_drv[4] = { 2,  2,  0,  1};

    ESP_LOGI(TAG, "Executing: Row %02d, Col %c, Action: %s", row, col_char, do_flip ? "FLIP" : "DE-FLIP");

    clear_shift_registers(); 

    if (do_flip) {
        // FLIP: Row acts as source, Column Minus acts as sink
        set_driver(row_sr_index, row_driver, true, true);
        set_driver(col_minus_sr[col_idx], col_minus_drv[col_idx], true, false);
    } else {
        // DE-FLIP: Column Plus acts as source, Row acts as sink
        set_driver(row_sr_index, row_driver, true, false);
        set_driver(col_plus_sr[col_idx], col_plus_drv[col_idx], true, true);
    }

    update_shift_registers(); 

    // Pulse duration
    esp_rom_delay_us(PULSE_DURATION_uS);

    // Isolate everybody
    clear_shift_registers();    
    update_shift_registers();   

    ESP_LOGI(TAG, "Pulse Complete.");
}

void clear_shift_registers(void) {
    memset(sr_data, 0xC9, sizeof(sr_data));
}

void update_shift_registers(void) {
    uint8_t tx_buffer[NUM_SHIFT_REGS];
    
    for (int i = 0; i < NUM_SHIFT_REGS; i++) {
        tx_buffer[i] = sr_data[NUM_SHIFT_REGS - 1 - i];
    }

    spi_transaction_t t = {
        .length = NUM_SHIFT_REGS * 8, 
        .tx_buffer = tx_buffer,
    };

    gpio_set_level(PIN_NUM_LATCH, 0);
    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
    gpio_set_level(PIN_NUM_LATCH, 1);
}