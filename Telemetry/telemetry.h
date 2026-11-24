#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stddef.h>
#include <stdint.h>

/* Clang reads these annotations during generation. Other compilers omit them.
 */
#if defined(__clang__)
#define TELEMETRY_READ __attribute__((annotate("telemetry")))
#define TELEMETRY_READ_WRITE __attribute__((annotate("telemetry:rw")))
#else
#define TELEMETRY_READ
#define TELEMETRY_READ_WRITE
#endif

typedef struct Telemetry_Metadata {
  char *name;
  volatile void *pointer;
  uint8_t type;
  uint8_t writable;
} Telemetry_Metadata;

/* Storage is defined once in telemetry.g.c. Counts exclude unused placeholders.
 */
extern const Telemetry_Metadata telemetry_metadata[];
extern const size_t telemetry_metadata_count;

/* Entry i uses word i / 32, bit i % 32. Zero means not requested.
 * All words start at zero. Only indices below telemetry_metadata_count are
 * valid. Callers must coordinate access if interrupts or tasks share this
 * bitmap. */
extern uint32_t telemetry_requested[];
extern const size_t telemetry_requested_word_count;

#define TELEMETRY_VARIABLE_NAME_MAX_LENGTH 128

typedef enum Telemetry_Packet_Type {
  TELEMETRY_PACKET_HANDSHAKE_INIT,
  TELEMETRY_PACKET_HANDSHAKE_INIT_OK,
  TELEMETRY_PACKET_HANDSHAKE_VARIABLE,
  TELEMETRY_PACKET_HANDSHAKE_VARIABLES_OK,
  TELEMETRY_PACKET_SUBSCRIBE,
  TELEMETRY_PACKET_SUBSCRIBE_OK,
  TELEMETRY_PACKET_SUBSCRIBE_ERR,
  TELEMETRY_PACKET_UNSUBSCRIBE,
  TELEMETRY_PACKET_UNSUBSCRIBE_OK,
  TELEMETRY_PACKET_DATA,
  TELEMETRY_PACKET_RESET,
} Telemetry_Packet_Type;

typedef struct Telemetry_Handshake_Init {
	uint8_t packet_type;
  uint8_t variable_count[2];
} Telemetry_Handshake_Init;

typedef struct Telemetry_Handshake_Init_Ok {
	uint8_t packet_type;
} Telemetry_Handshake_Init_Ok;

typedef struct Telemetry_Handshake_Variable {
	uint8_t packet_type;
  uint8_t variable_id[2];
  uint8_t type;
  /* Up to TELEMETRY_VARIABLE_NAME_MAX_LENGTH ASCII characters plus a null byte. */
  char variable_name[];
} Telemetry_Handshake_Variable;

/* Sent once after the ESP accepts the advertised nonzero variable count. */
typedef struct Telemetry_Handshake_Variables_Ok {
  uint8_t packet_type;
} Telemetry_Handshake_Variables_Ok;

typedef struct Telemetry_Subscribe {
	uint8_t packet_type;
  uint8_t variable_id[2];
} Telemetry_Subscribe;

typedef struct Telemetry_Subscribe_Ok {
	uint8_t packet_type;
  uint8_t variable_id[2];
} Telemetry_Subscribe_Ok;

typedef struct Telemetry_Subscribe_Err {
	uint8_t packet_type;
  uint8_t variable_id[2];
} Telemetry_Subscribe_Err;

typedef struct Telemetry_Unsubscribe {
	uint8_t packet_type;
  uint8_t variable_id[2];
} Telemetry_Unsubscribe;

typedef struct Telemetry_Unsubscribe_Ok {
	uint8_t packet_type;
  uint8_t variable_id[2];
} Telemetry_Unsubscribe_Ok;

typedef struct Telemetry_Data {
	uint8_t packet_type;
  uint8_t variable_id[2];
  /* The payload length includes the ID and these little-endian value bytes. */
  uint8_t value[];
} Telemetry_Data;

typedef struct Telemetry_Reset {
	uint8_t packet_type;
} Telemetry_Reset;

const uint8_t *telemetry_decode_command(const uint8_t *packet, size_t length);
/* Each chip target supplies these handlers. Length counts the bytes from the
 * packet type byte to the end of the input buffer. Check length before reading
 * fields. Return the byte after the packet on success, or NULL if the packet
 * cannot be handled. */
const uint8_t *telemetry_handle_handshake_init(
    const Telemetry_Handshake_Init *packet, size_t length);
const uint8_t *telemetry_handle_handshake_init_ok(
    const Telemetry_Handshake_Init_Ok *packet, size_t length);
const uint8_t *telemetry_handle_handshake_variable(
    const Telemetry_Handshake_Variable *packet, size_t length);
const uint8_t *telemetry_handle_handshake_variables_ok(
    const Telemetry_Handshake_Variables_Ok *packet, size_t length);
const uint8_t *telemetry_handle_subscribe(
    const Telemetry_Subscribe *packet, size_t length);
const uint8_t *telemetry_handle_subscribe_ok(
    const Telemetry_Subscribe_Ok *packet, size_t length);
const uint8_t *telemetry_handle_subscribe_err(
    const Telemetry_Subscribe_Err *packet, size_t length);
const uint8_t *telemetry_handle_unsubscribe(
    const Telemetry_Unsubscribe *packet, size_t length);
const uint8_t *telemetry_handle_unsubscribe_ok(
    const Telemetry_Unsubscribe_Ok *packet, size_t length);
const uint8_t *telemetry_handle_data(const Telemetry_Data *packet, size_t length);
const uint8_t *telemetry_handle_reset(const Telemetry_Reset *packet, size_t length);

#define TL_TYPE_NAME_MASK 0b0011
#define TL_TYPE_SIZE_MASK 0b1100

#define TL_INT   0b00
#define TL_UINT  0b01
#define TL_FLOAT 0b10
#define TL_BOOL  0b11

#define TL_8BIT   (0b00 << 2)
#define TL_16BIT  (0b01 << 2)
#define TL_32BIT  (0b10 << 2)
#define TL_64BIT  (0b11 << 2)

#define TL_RDONLY (0 << 7)
#define TL_RDWR   (1 << 7)

// Not sure about the sizes of bool and float, maybe using asserts?
#define TL_TYPE_ENCODE(type) _Generic((type), \
	int8_t:   TL_INT   | TL_8BIT,      \
	int16_t:  TL_INT   | TL_16BIT,     \
	int32_t:  TL_INT   | TL_32BIT,     \
	int64_t:  TL_INT   | TL_64BIT,     \
	uint8_t:  TL_UINT  | TL_8BIT,      \
	uint16_t: TL_UINT  | TL_16BIT,     \
	uint32_t: TL_UINT  | TL_32BIT,     \
	uint64_t: TL_UINT  | TL_64BIT,     \
	float:    TL_FLOAT | TL_32BIT,     \
	bool:     TL_BOOL  | TL_8BIT,      \
	default:  TL_INT   | TL_64BIT      \
) \

#endif
