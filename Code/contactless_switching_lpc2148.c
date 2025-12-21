#include <LPC214x.h>

// Define GPIO pin masks
#define IR_SENSOR_PIN_1   (1 << 8)   // P0.8 for IR sensor 1
#define RELAY_PIN_1       (1 << 9)   // P0.9 for relay 1 (LED)
#define EXTERNAL_LED_PIN  (1 << 10)  // P0.10 for external LED
#define IR_SENSOR_PIN_2   (1 << 11)  // P0.11 for IR sensor 2
#define RELAY_PIN_2       (1 << 12)  // P0.12 for relay 2 (DC motor fan)
#define MOTOR_IN1_PIN     (1 << 13)  // P0.13 for motor IN1 (L298N)
#define MOTOR_IN2_PIN     (1 << 14)  // P0.14 for motor IN2 (L298N)

// Function to set up the PLL for a system clock of 60 MHz
void init_PLL() {
    PLL0CFG = 0x24;    // M = 4, P = 2 (Multiplier = 4, Divider = 2)
    PLL0CON = 0x01;    // Enable the PLL
    PLL0FEED = 0xAA;   // Feed sequence for PLL configuration
    PLL0FEED = 0x55;
    while (!(PLL0STAT & 0x00000400));  // Wait for the PLL to lock
    PLL0CON = 0x03;    // Connect the PLL as the clock source
    PLL0FEED = 0xAA;   // Feed sequence to finalize PLL configuration
    PLL0FEED = 0x55;
    VPBDIV = 0x01;     // Set Peripheral Clock (PCLK) to be the same as CPU Clock (CCLK)
}

// Function to introduce a delay in milliseconds
void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < 6000; j++) {
            __asm volatile ("nop");  // No Operation (do nothing), to make the delay more accurate
        }
    }
}

// Function to initialize GPIO pins
void init_GPIO() {
    // Set P0.8 as input for IR sensor 1
    IO0DIR &= ~IR_SENSOR_PIN_1; // Configure P0.8 as input
    IO0SET |= IR_SENSOR_PIN_1;  // Enable internal pull-up resistor for P0.8

    // Set P0.9 as output for relay 1
    IO0DIR |= RELAY_PIN_1;  // Configure P0.9 as output
    IO0CLR = RELAY_PIN_1;   // Initially turn off relay 1

    // Set P0.10 as output for external LED
    IO0DIR |= EXTERNAL_LED_PIN;  // Configure P0.10 as output
    IO0CLR = EXTERNAL_LED_PIN;   // Initially turn off external LED

    // Set P0.11 as input for IR sensor 2
    IO0DIR &= ~IR_SENSOR_PIN_2; // Configure P0.11 as input
    IO0SET |= IR_SENSOR_PIN_2;  // Enable internal pull-up resistor for P0.11

    // Set P0.12 as output for relay 2
    IO0DIR |= RELAY_PIN_2;  // Configure P0.12 as output
    IO0CLR = RELAY_PIN_2;   // Initially turn off relay 2

    // Set P0.13 and P0.14 as output for motor control (L298N)
    IO0DIR |= MOTOR_IN1_PIN;  // Configure P0.13 as output
    IO0DIR |= MOTOR_IN2_PIN;  // Configure P0.14 as output
    IO0CLR = MOTOR_IN1_PIN;   // Initially set IN1 low
    IO0CLR = MOTOR_IN2_PIN;   // Initially set IN2 low
}

int main(void) {
    int wave_count_1 = 0;
    int wave_count_2 = 0;
    int previous_state_1 = 1; // Assume IR sensor 1 is initially not blocked
    int previous_state_2 = 1; // Assume IR sensor 2 is initially not blocked
    int current_state_1;
    int current_state_2;
    unsigned int debounce_delay = 200; // Delay to debounce the sensors
    unsigned int max_wave_time = 1000; // Maximum time allowed between waves in ms
    unsigned int wave_start_time_1 = 0;
    unsigned int wave_start_time_2 = 0;

    init_PLL();     // Initialize PLL and system clock to 60 MHz
    init_GPIO();    // Initialize GPIO pins

    while (1) {
        // Read IR sensor 1 state
        current_state_1 = (IO0PIN & IR_SENSOR_PIN_1) ? 1 : 0;

        // Process IR sensor 1
        if (current_state_1 == 0 && previous_state_1 == 1) { // Detect falling edge for sensor 1
            wave_count_1++;
            wave_start_time_1 = 0; // Reset wave start time
            delay_ms(debounce_delay); // Debounce delay
        }

        if (wave_count_1 > 0) {
            wave_start_time_1 += debounce_delay;
            if (wave_start_time_1 >= max_wave_time) {
                if (wave_count_1 == 1) {
                    // Single wave on IR sensor 1: Turn on relay 1 and external LED
                    IO0SET = RELAY_PIN_1;
                    IO0CLR = EXTERNAL_LED_PIN;
                } else if (wave_count_1 == 2) {
                    // Double wave on IR sensor 1: Turn off relay 1 and external LED
                    IO0CLR = RELAY_PIN_1;
                    IO0SET = EXTERNAL_LED_PIN;
                }
                wave_count_1 = 0; // Reset wave count after action
                wave_start_time_1 = 0; // Reset wave start time
            }
        }

        // Read IR sensor 2 state
        current_state_2 = (IO0PIN & IR_SENSOR_PIN_2) ? 1 : 0;

        // Process IR sensor 2
        if (current_state_2 == 0 && previous_state_2 == 1) { // Detect falling edge for sensor 2
            wave_count_2++;
            wave_start_time_2 = 0; // Reset wave start time
            delay_ms(debounce_delay); // Debounce delay
        }

        if (wave_count_2 > 0) {
            wave_start_time_2 += debounce_delay;
            if (wave_start_time_2 >= max_wave_time) {
                if (wave_count_2 == 1) {
                    // Single wave on IR sensor 2: Turn off relay 2 and motor
                    IO0SET = RELAY_PIN_2;        // Turn off relay 2
                    IO0CLR = MOTOR_IN1_PIN;      // Set IN1 low
                    IO0SET = MOTOR_IN2_PIN;      // Set IN2 high
                } else if (wave_count_2 == 2) {
                    // Double wave on IR sensor 2: Turn on relay 2 and motor
                    IO0CLR = RELAY_PIN_2;        // Turn on relay 2
                    IO0SET = MOTOR_IN1_PIN;      // Set IN1 high
                    IO0CLR = MOTOR_IN2_PIN;      // Set IN2 low 
                }
                wave_count_2 = 0; // Reset wave count after action
                wave_start_time_2 = 0; // Reset wave start time
            }
        }

        // Update previous states
        previous_state_1 = current_state_1;
        previous_state_2 = current_state_2;

        delay_ms(debounce_delay); // Delay to avoid rapid polling
    }
}
