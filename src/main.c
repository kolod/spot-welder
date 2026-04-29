#include <fw_hal.h>

static inline void display_init() {
    // Set segments as output open-drain (for common cathode)
    GPIO_P1_SetMode(GPIO_Pin_All, GPIO_Mode_InOut_OD);
    // Set digits as output push-pull
    GPIO_P3_SetMode(GPIO_Pin_1 | GPIO_Pin_3 | GPIO_Pin_4, GPIO_Mode_Output_PP);

    // Turn off all segments and digits
    P1 = 0xFF; // All segments off (open-drain, so set to 1)
    P3 &= ~(GPIO_Pin_1 | GPIO_Pin_3 | GPIO_Pin_4); // All digits off (push-pull, so set to 0)
}

static inline void display_digit(uint8_t digit, uint8_t value) {

    // Turn off all digits before updating segments
    P3 &= ~(GPIO_Pin_1 | GPIO_Pin_3 | GPIO_Pin_4);

    // Define segment patterns for digits 0-9 (assuming common cathode)
    // DP, G, F, E, D, C, B, A (bit 7 to bit 0)
    const uint8_t segment_patterns[10] = {
        ~0b00111111, // 0
        ~0b00000110, // 1
        ~0b01011011, // 2
        ~0b01001111, // 3
        ~0b01100110, // 4
        ~0b01101101, // 5
        ~0b01111101, // 6
        ~0b00000111, // 7
        ~0b01111111, // 8
        ~0b01101111, // 9
    };

    // Validate input
    if (digit > 3 || value > 9) return;

    // Set segments for the value (0-9)
    P1 = segment_patterns[value]; 

    // Activate the corresponding digit (1-based index)
    if (digit == 1) {
        P3 |= GPIO_Pin_1; // Activate digit 1
    } else if (digit == 2) {
        P3 |= GPIO_Pin_3; // Activate digit 2
    } else if (digit == 3) {
        P3 |= GPIO_Pin_4; // Activate digit 3
    }

    // Short delay to allow the digit to be visible
    SYS_Delay(5);
}

static inline void display_number(uint16_t number) {
    display_digit(1, (number / 100) % 10);  // Hundreds
    display_digit(2, (number / 10) % 10);   // Tens
    display_digit(3, number % 10);          // Units
}

void main(void) {

    display_init();

    uint8_t sub_cycle = 0;
    uint16_t cycle = 0;

    for (;;) {
        display_number(cycle);
        if (++sub_cycle > (1000/15)) {
            sub_cycle = 0;
            if (++cycle > 999) cycle = 0;
        }    
    }
}