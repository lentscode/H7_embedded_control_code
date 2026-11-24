#include "telemetry.h"

#include <string.h>

/* A target can provide its own data handler without a generated table. */
#pragma weak telemetry_metadata
#pragma weak telemetry_metadata_count

static void TL_TYPE_DECODE(uint8_t byte) {
	char *type_name;
	switch (byte & TL_TYPE_NAME_MASK) {
		case TL_INT:   type_name = "int";   break;
		case TL_UINT:  type_name = "uint";  break;
		case TL_FLOAT: type_name = "float"; break;
		case TL_BOOL:  type_name = "bool";  break;
	}
			
	uint8_t type_size;
	switch (byte & TL_TYPE_SIZE_MASK) {
		case TL_8BIT:  type_size = 8;  break;
		case TL_16BIT: type_size = 16; break;
		case TL_32BIT: type_size = 32; break;
		case TL_64BIT: type_size = 64; break;
	}
}

const uint8_t *telemetry_decode_command(const uint8_t *packet, size_t length) {
	if (packet == NULL || length == 0) {
		return NULL;
	}

	switch (packet[0]) {
		case TELEMETRY_PACKET_HANDSHAKE_INIT:
			return telemetry_handle_handshake_init(
			    (const Telemetry_Handshake_Init *)packet, length);
		case TELEMETRY_PACKET_HANDSHAKE_INIT_OK:
			return telemetry_handle_handshake_init_ok(
			    (const Telemetry_Handshake_Init_Ok *)packet, length);
		case TELEMETRY_PACKET_HANDSHAKE_VARIABLE:
			return telemetry_handle_handshake_variable(
			    (const Telemetry_Handshake_Variable *)packet, length);
		case TELEMETRY_PACKET_HANDSHAKE_VARIABLES_OK:
			return telemetry_handle_handshake_variables_ok(
			    (const Telemetry_Handshake_Variables_Ok *)packet, length);
		case TELEMETRY_PACKET_SUBSCRIBE:
			return telemetry_handle_subscribe(
			    (const Telemetry_Subscribe *)packet, length);
		case TELEMETRY_PACKET_SUBSCRIBE_OK:
			return telemetry_handle_subscribe_ok(
			    (const Telemetry_Subscribe_Ok *)packet, length);
		case TELEMETRY_PACKET_SUBSCRIBE_ERR:
			return telemetry_handle_subscribe_err(
			    (const Telemetry_Subscribe_Err *)packet, length);
		case TELEMETRY_PACKET_UNSUBSCRIBE:
			return telemetry_handle_unsubscribe(
			    (const Telemetry_Unsubscribe *)packet, length);
		case TELEMETRY_PACKET_UNSUBSCRIBE_OK:
			return telemetry_handle_unsubscribe_ok(
			    (const Telemetry_Unsubscribe_Ok *)packet, length);
		case TELEMETRY_PACKET_DATA:
			return telemetry_handle_data((const Telemetry_Data *)packet, length);
		case TELEMETRY_PACKET_RESET:
			return telemetry_handle_reset((const Telemetry_Reset *)packet, length);
		default:
			return NULL;
	}
}

static const uint8_t *telemetry_skip_packet(const void *packet, size_t length,
                                           size_t packet_size) {
	if (packet == NULL || length < packet_size) {
		return NULL;
	}
	return (const uint8_t *)packet + packet_size;
}

__attribute__((weak)) const uint8_t *telemetry_handle_handshake_init(
    const Telemetry_Handshake_Init *packet, size_t length) {
	return telemetry_skip_packet(packet, length, sizeof(*packet));
}

__attribute__((weak)) const uint8_t *telemetry_handle_handshake_init_ok(
    const Telemetry_Handshake_Init_Ok *packet, size_t length) {
	return telemetry_skip_packet(packet, length, sizeof(*packet));
}

__attribute__((weak)) const uint8_t *telemetry_handle_handshake_variable(
    const Telemetry_Handshake_Variable *packet, size_t length) {
	size_t header_size = offsetof(Telemetry_Handshake_Variable, variable_name);
	if (packet == NULL || length <= header_size) {
		return NULL;
	}

	size_t name_limit = length - header_size;
	if (name_limit > TELEMETRY_VARIABLE_NAME_MAX_LENGTH + 1) {
		name_limit = TELEMETRY_VARIABLE_NAME_MAX_LENGTH + 1;
	}

	const uint8_t *name = (const uint8_t *)packet + header_size;
	const uint8_t *terminator = memchr(name, '\0', name_limit);
	return terminator == NULL ? NULL : terminator + 1;
}

__attribute__((weak)) const uint8_t *telemetry_handle_handshake_variables_ok(
    const Telemetry_Handshake_Variables_Ok *packet, size_t length) {
	return telemetry_skip_packet(packet, length, sizeof(*packet));
}

__attribute__((weak)) const uint8_t *telemetry_handle_subscribe(
    const Telemetry_Subscribe *packet, size_t length) {
	return telemetry_skip_packet(packet, length, sizeof(*packet));
}

__attribute__((weak)) const uint8_t *telemetry_handle_subscribe_ok(
    const Telemetry_Subscribe_Ok *packet, size_t length) {
	return telemetry_skip_packet(packet, length, sizeof(*packet));
}

__attribute__((weak)) const uint8_t *telemetry_handle_subscribe_err(
    const Telemetry_Subscribe_Err *packet, size_t length) {
	return telemetry_skip_packet(packet, length, sizeof(*packet));
}

__attribute__((weak)) const uint8_t *telemetry_handle_unsubscribe(
    const Telemetry_Unsubscribe *packet, size_t length) {
	return telemetry_skip_packet(packet, length, sizeof(*packet));
}

__attribute__((weak)) const uint8_t *telemetry_handle_unsubscribe_ok(
    const Telemetry_Unsubscribe_Ok *packet, size_t length) {
	return telemetry_skip_packet(packet, length, sizeof(*packet));
}

__attribute__((weak)) const uint8_t *telemetry_handle_data(
    const Telemetry_Data *packet, size_t length) {
	size_t header_size = offsetof(Telemetry_Data, value);
	if (packet == NULL || length <= header_size) {
		return NULL;
	}

	uint16_t variable_id = packet->variable_id[0] | ((uint16_t)packet->variable_id[1] << 8);
	if (telemetry_metadata == NULL || &telemetry_metadata_count == NULL ||
	    variable_id >= telemetry_metadata_count) {
		return NULL;
	}

	/* The size bits encode 1, 2, 4 or 8 bytes as exponents of two. */
	uint8_t type = telemetry_metadata[variable_id].type;
	size_t value_size = 1u << ((type & TL_TYPE_SIZE_MASK) >> 2);
	return telemetry_skip_packet(packet, length, header_size + value_size);
}

__attribute__((weak)) const uint8_t *telemetry_handle_reset(
    const Telemetry_Reset *packet, size_t length) {
	return telemetry_skip_packet(packet, length, sizeof(*packet));
}
