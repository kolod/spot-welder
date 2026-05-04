// Spot Welder Controller

#include "8h1k08.h"

#define MOSFET         P3_0
#define DIGIT_1        P3_1
#define ADC_INPUT      P3_2
#define DIGIT_2        P3_3
#define DIGIT_3        P3_4
#define BUZZER         P3_5
#define KEY1           P3_6
#define KEY2           P3_7

#define PEDAL          P5_4

// Global variables
uint16_t voltage;               // To hold the calculated voltage in 0.1 volts (e.g., 123 means 12.3V)
uint8_t digit[3];               // To hold the current digits to display

static inline void display_digit_1() {
    DIGIT_3 = 0;                   // Disable digit 3 (P3.4)
    P1      = digit[0];            // Set segment data for the current digit
    DIGIT_1 = 1;                   // Enable digit 1 (P3.1)
}

static inline void display_digit_2() {
    DIGIT_1 = 0;                   // Disable digit 1 (P3.1)
    P1      = digit[1];            // Set segment data for the current digit
    DIGIT_2 = 1;                   // Enable digit 2 (P3.3)
}

static inline void display_digit_3() {
    DIGIT_2 = 0;                   // Disable digit 2 (P3.3)
    P1      = digit[2];            // Set segment data for the current digit
    DIGIT_3 = 1;                   // Enable digit 3 (P3.4)
}

static inline void display_off() {
    DIGIT_1 = 0;                   // Disable digit 1 (P3.1)
    DIGIT_2 = 0;                   // Disable digit 2 (P3.3)
    DIGIT_3 = 0;                   // Disable digit 3 (P3.4)
}

// Function to update the digit array based on the voltage value
void display_show_value(uint16_t value) {
    const uint8_t digit_map[10] = {
        (uint8_t)~0b00111111, // 0
        (uint8_t)~0b00000110, // 1
        (uint8_t)~0b01011011, // 2
        (uint8_t)~0b01001111, // 3
        (uint8_t)~0b01100110, // 4
        (uint8_t)~0b01101101, // 5
        (uint8_t)~0b01111101, // 6
        (uint8_t)~0b00000111, // 7
        (uint8_t)~0b01111111, // 8
        (uint8_t)~0b01101111  // 9
    };

    // if the value exceeds 999, display "Err"
    if (value > 999) {
        digit[0] = (uint8_t)~0b01111001; // 'E'
        digit[1] = (uint8_t)~0b01010000; // 'r'
        digit[2] = (uint8_t)~0b01010000; // 'r'
        return;
    }

    // Update the digit array based on the value
    digit[0] = digit_map[value / 100];        // Hundreds
    digit[1] = digit_map[(value / 10) % 10];  // Tens
    digit[2] = digit_map[value % 10];         // Units
}

// Function to display the voltage on the 7-segment display with one decimal place (e.g., 12.3V)
static inline void display_voltage(uint16_t voltage) {
    display_show_value(voltage);         // Update the digit array for display
    digit[1] &= ~0b10000000;             // Set the decimal point on the units digit
}

// Moving average filter using ring buffer
// Returns average of last 8 values, optimized for 8-bit x51 MCU
uint16_t filter(uint16_t value) {
    static uint16_t buffer[8] = {0};    // Ring buffer to store last 8 values
    static uint8_t index = 0;           // Current position in ring buffer (0-7)
    static uint32_t sum = 0;            // Running sum of all 8 values
    
    // Subtract the oldest value that will be replaced
    sum -= buffer[index];
    
    // Store new value in buffer
    buffer[index] = value;
    
    // Add new value to running sum
    sum += value;
    
    // Move to next position, wrap around using mask (efficient for power of 2)
    index = (index + 1) & 0x07;
    
    // Return average: divide by 8 using right shift (optimized)
    return (uint16_t)(sum >> 3);
}

// Poll the ADC and return the raw ADC value (10-bit)
// Voltage divider R1 = 200k, R2 = 39k
// V = ADC_value * (5.0 / 1023) * ((R1 + R2) / R2) = ADC_value * 2.98 (approximate to 3.0 for simplicity, so 0.1V per 10 ADC counts)
static inline uint16_t poll_adc() {
    ADC_CONTR |= 0x40;                        // Start ADC conversion
    while (!(ADC_CONTR & 0x20));              // Wait for ADC conversion to complete (polling the interrupt flag)
    ADC_CONTR &= ~0x20;                       // Clear ADC interrupt flag
    return ((ADC_RES << 8) | ADC_RESL) * 3;   // Combine high and low bytes to get the full ADC value
}

// Simple delay function F_osc = 16MHz
// Tuned for actual timing: calibrated to ~0.8x multiplier
static inline void delay_us(uint16_t us) {
    for (volatile uint16_t i = (us>>1) + (us>>2) + (us>>4); i; i--);  // us * 0.8125 (calibrated)
}

// System initialization: configure GPIOs, ADC, and display
static inline void system_init() {

    // Initialize global variables
    voltage = 0; // Start with 0 voltage
    digit[0] = 0xFF; // Initialize digit patterns to 0
    digit[1] = 0xFF; // Initialize digit patterns to 0
    digit[2] = 0xFF; // Initialize digit patterns to 0

    // Initialize GPIO
    // PnM1.x = 0, PnM0.x = 0 - quasi-bidirectional (default state after reset)
    // PnM1.x = 0, PnM0.x = 1 - push-pull output
    // PnM1.x = 1, PnM0.x = 0 - high-impedance input input
    // PnM1.x = 1, PnM0.x = 1 - open-drain output

    // P1 as open-drain outputs for segments
    P1M0 = 0b11111111; P1M1 = 0b11111111; // All as open-drain outputs

    // P3.0 as open-drain output (for mosfet control)
    // P3.1 as push-pull output (for digit 1 common cathode control)
    // P3.2 as high-impedance input (for ADC10 input)
    // P3.3 as push-pull output (for digit 2 common cathode control)
    // P3.4 as push-pull output (for digit 3 common cathode control)
    // P3.5 as open-drain output (for buzzer control)
    // P3.6 as high-impedance input (for buttons 1)
    // P3.7 as high-impedance input (for buttons 2)
    P3M0 = 0b00011011; P3M1 = 0b11100101; // P3.0, P3.5 as open-drain; P3.1, P3.3, P3.4 as push-pull; P3.2, P3.6, P3.7 as input

    // P5.4 as high-impedance input (for trigger input)
    // P5.5 as high-impedance input (for ADC reference)
    //P5M0 = 0b00000000; P5M1 = 0b00110000; // P5.4, P5.5 as input; others as default

    // Setup pull-up resistors
    SFRX_ON();
    P1PU = 0b00000000; // No pull-ups on P1 (segments)
    P3PU = 0b11000001; // Enable pull-up for P3.6 and P3.7 (buttons)
    P5PU = 0b00000000; // No pull-ups on P5 (trigger and ADC reference)

    ADCTIM = 0x3F; 
    SFRX_OFF();

    ADCCFG = 0x2F; // Set ADC clock to Fosc/16 for stable operation (assuming Fosc = 16MHz, ADC clock = 1MHz)

    // Initialize ADC on P3.2 (ADC channel 10) and start continuous conversion
    // ADC_CONTR:
    // Bit 7: ADC power control (1 = on, 0 = off)
    // Bit 6: ADC start control (1 = start conversion, auto-clears when conversion is complete)
    // Bit 5: ADC interrupt flag (1 = conversion complete, cleared by software)
    // Bit 4: Enable PWM synchronization (not used here, set to 0)
    // Bit 3..0: ADC channel selection (000 = ADC0, 001 = ADC1, ..., 111 = ADC7)
    // For 8h1k08, ADC channels are mapped as follows:
    // 0000: ADC0 (P1.0)
    // 0001: ADC1 (P1.1)
    // 1000: ADC8 (P3.0)
    // 1001: ADC9 (P3.1)
    // 1010: ADC10 (P3.2) <- we want this one
    // 1011: ADC11 (P3.3)
    // 1100: ADC12 (P3.4)
    // 1101: ADC13 (P3.5)
    // 1110: ADC14 (P3.6)
    // 1111: Internal test 1.19V
    ADC_CONTR = 0x80 | 10; // Power on ADC, select channel 10
}

// Main loop function
static inline void loop(void) {
    // Update the display 
    display_digit_1(); delay_us(10);
    display_digit_2(); delay_us(10);
    display_digit_3(); delay_us(10);
    display_off();

    // Get the latest ADC value, filter it, and update the voltage variable
    voltage = filter(poll_adc());
    
    // Update the digit array based on the new voltage value
    display_voltage(voltage);
}

// Main function
void main(void) {

    // Initialize system
    system_init();

    // Main loop
    for (;;) loop();
}