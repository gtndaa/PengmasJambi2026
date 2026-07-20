#pragma once

// RF
#define RF_FREQ_MHZ     433.92f
#define RF_MODULATION   2          // ASK/OOK
#define RF_DATARATE     4.8f
#define RF_RXBW         203.12f

// Pulse timing untuk WH5300
#define PULSE_1_MIN     350
#define PULSE_1_MAX     650
#define PULSE_0_MIN     1100
#define PULSE_0_MAX     1590

// Rain
#define RAIN_UNINIT     0xFF

// CRC
#define CRC_POLY        0x31
#define CRC_INIT        0x00

// Ukuran buffer
#define MAX_PULSES      300
#define MAX_BYTES       12
#define PACKET_TIMEOUT  30

// SD
#define SD_MAX_RECORDS  1000
#define SD_FILENAME     "/weather.log"

// HTTP
#define HTTP_TIMEOUT    10000
#define MAX_RETRIES     3