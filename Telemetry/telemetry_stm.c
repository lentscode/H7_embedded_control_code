#include "telemetry.h"

#include "stm32h7xx_hal.h"
#include "usart.h"
#include <stdint.h>
#include <string.h>

#define TELEMETRY_SPI 0
#define TELEMETRY_CHANNEL huart10 
#define TELEMETRY_TIMEOUT 500

static void telemetry_send(uint8_t *data, uint32_t len, uint32_t timeout) {
	#if TELEMETRY_SPI 
		HAL_SPI_Transmit(&(TELEMETRY_CHANNEL), data, len, timeout);
	#else
		HAL_UART_Transmit(&(TELEMETRY_CHANNEL), data, len, timeout);
	#endif
}

volatile uint8_t handshake_init_ack;
volatile uint8_t handshake_variables_ack;

static uint32_t send_handshake_init() {
    Telemetry_Handshake_Init p = {
        .packet_type = TELEMETRY_PACKET_HANDSHAKE_INIT,
        .variable_count = telemetry_metadata_count,
    };

    telemetry_send((uint8_t *)&p, sizeof(Telemetry_Handshake_Init), TELEMETRY_TIMEOUT);

    return sizeof(Telemetry_Handshake_Init);
}

static uint32_t send_handshake_variables() {
    for (uint32_t i = 0; i < telemetry_metadata_count; ++i) {
        Telemetry_Metadata metadata = telemetry_metadata[i];

        Telemetry_Handshake_Variable p = {
            .packet_type = TELEMETRY_PACKET_HANDSHAKE_VARIABLE,
            .type = metadata.type,
            .variable_id = i,
        };

        strcpy(p.variable_name, metadata.name);

        telemetry_send((uint8_t*)&p, sizeof(Telemetry_Handshake_Init), TELEMETRY_TIMEOUT);
    }

    return sizeof(Telemetry_Handshake_Init);
}

void TelemetryCommunicationTask(void *params) {
    handshake_init_ack = 0;
    handshake_variables_ack = 0;

    for (;;) {
        if (!handshake_init_ack) {
            send_handshake_init();
        }

        if (!handshake_variables_ack) {
            send_handshake_variables();
        }
    }
}
