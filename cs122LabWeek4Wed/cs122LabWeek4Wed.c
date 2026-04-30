#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/spi.h"

// SPI Pin Definitions
#define SPI_PORT spi0
#define PIN_SCK  6
#define PIN_MOSI 7
#define PIN_CS   8  

// Other Pin Definitions
#define PIN_LED_BASE 18 
#define PIN_DIP_SW   16 
#define ADC_POT_PIN  26

volatile uint32_t ms_counter = 0;
volatile int current_frame  = 0;

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint8_t spi_data = 0;
int testCount = 0;
bool timer_callback(struct repeating_timer *t) {
    
    uint16_t raw_adc = adc_read();
    uint32_t threshold_ms = map(raw_adc, 0, 4095, 1000, 100);

    ms_counter++;

    if (ms_counter >= threshold_ms) {
        ms_counter = 0; 

        bool mode = gpio_get(PIN_DIP_SW);

        uint16_t full_pattern = 0;

        if (mode) {
            
            int position = current_frame % 8; 
            if (position < 8) {
                full_pattern = (1 << (position + 1)) - 1;
            } else {
                full_pattern = 0;
                position = 0;
            }
            spi_data = (uint8_t)((full_pattern >> 4) & 0xFF);
        } else {
            if (current_frame % 2 == 0) {
                full_pattern = 0xAAA; // 1010 1010 1010 there incase i did extra credit
                spi_data = (uint8_t)(0xAA);
                
            } else {
                full_pattern = 0x555; // 0101 0101 0101
                spi_data = (uint8_t)(0x55);
            }
        }

        uint8_t local_bits = (uint8_t)(full_pattern & 0xF); 
        for (int i = 0; i < 4; i++) {
            gpio_put(PIN_LED_BASE + i, (local_bits >> i) & 1);
        }

        
        //uint8_t spi_data = (uint8_t)((full_pattern >> 4) & 0xFF);
        

        gpio_put(PIN_CS, 0); 
        spi_write_blocking(SPI_PORT, &spi_data, 1);
        gpio_put(PIN_CS, 1); 

        current_frame++; 
        testCount++;
        if(testCount >= 16) {
            testCount = 0;
        }
    }

    return true; 
}

int main() {
    stdio_init_all();
    
    //used AI for init stuff
    // --- SPI Initialization ---
    spi_init(SPI_PORT, 1000* 1000); // 1MHz 
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    
    // Manual Chip Select initialization 
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1); // Default High

    // --- Hardware Initializations ---
    adc_init();
    adc_gpio_init(ADC_POT_PIN);
    adc_select_input(0); // GP26 is ADC 0 [cite: 2]

    gpio_init(PIN_DIP_SW);
    gpio_set_dir(PIN_DIP_SW, GPIO_IN);
    // Pull-down ensures 0V until switch connects pin to 3.3V 
    gpio_pull_down(PIN_DIP_SW);

    for (int i = PIN_LED_BASE; i <= 21; i++) {
        gpio_init(i);
        gpio_set_dir(i, GPIO_OUT);
    }

    // Initialize Timer to fire every 1ms [cite: 2]
    struct repeating_timer timer;
    add_repeating_timer_ms(-1, timer_callback, NULL, &timer);

    while(1) {
       // tight_loop_contents();
    }
}