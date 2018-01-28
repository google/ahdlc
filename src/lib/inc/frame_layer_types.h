/* Copyright 2018 Google LLC

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef LIB_INC_FRAME_LAYER_TYPES_H_
#define LIB_INC_FRAME_LAYER_TYPES_H_

#include <stdint.h>

/* Callback for CRC calculation */
typedef uint16_t (*crc_callback)(uint16_t crc, const uint8_t* buffer,
    uint32_t lenth);

/* Callback for encryption */
typedef uint32_t (*enc_callback)(uint32_t ctr, const uint8_t* buffer,
    uint32_t lenth);

#define CRC_ARRAY_SIZE (3)

/* TODO  (skeys) we may want this part of init() */
extern const uint16_t initial_crc_value;

typedef enum {
  AHDLC_FALSE = 0,
  AHDLC_TRUE = 1
}ahdlc_true_false;

/* Function return codes */
typedef enum {
  AHDLC_INVALID_FRAME      = -6,
  AHDLC_BAD_CRC            = -5,
  AHDLC_ERROR              = -4,
  AHDLC_ACK_REQUEST_SET    = -3,
  AHDLC_CRC_ENGINE_FAILURE = -2,
  AHDLC_BUFFER_TOO_SMALL   = -1,
  AHDLC_OK                 =  0,
  AHDLC_COMPLETE           =  1
}ahdlc_op_return;


typedef struct {
  /* Test for a little-endian machine */
  #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  uint8_t low;
  uint8_t high;
  #else
  uint8_t high;
  uint8_t low;
  #endif
}crc_16_bytes_t;

typedef union {
  crc_16_bytes_t bytes;
  uint16_t crc_value;
}crc_16_t;

typedef struct {
  crc_16_t crc_array[CRC_ARRAY_SIZE];
  uint8_t crc_index;
}crc_16_stack_t;

typedef enum {
  ENCODE_SEND_BYTE_TO_CALLBACK  = 0,
  ENCODE_SEND_BYTE_TO_BUFFER    = 1,
  ENCODE_SEND_FRAME_TO_CALLBACK = 2
}encode_modes;

/* Individual frame status */
typedef enum {
  ENCODE_BUFFER_TOO_SMALL = -1,
  ENCODE_READY            = 0,
  ENCODE_FINALIZED        = 1,
}mmwave_encoder_machine_state;

/* Individual frame status */
typedef enum {
  /* Error states */
  DECODE_NO_VALID_FRAME_BIT  = -6,
  DECODE_INVALID_ESCAPE_SEQ  = -5,
  DECODE_COMPLETE_BAD_CRC    = -4,
  DECODE_ACK_SIZE_MISMATCH   = -3,
  DECODE_UNIMPLIMENTED       = -2,
  DECODE_BUFFER_TOO_SMALL    = -1,
  /* Non error states */
  DECODE_RESERVED            = 0,
  DECODE_EXPECTING_SEQUENCE  = 1,
  DECODE_EXPECTING_FLAGS     = 2,
  DECODE_EXPECTING_ACK_NUM   = 3,
  DECODE_EXPECTING_ENC_CTR   = 4,
  DECODE_EXPECTING_PDU       = 5,
  DECODE_EXPECTING_LENGTH    = 6,
  DECODE_EXPECTING_ESCAPE    = 7,
  DECODE_COMPLETE_GOOD       =  8,
}ahdlc_decoder_machine_state;

/* Decoded frame stats */
typedef struct {
  uint32_t num_decoded_bad_crc;
  uint32_t good_frame_cnt;
  uint32_t invalid_escape_cnt;
  uint32_t out_of_sequence_cnt;
  uint32_t out_of_frame_byte_cnt;
  uint32_t encryption_engine_callback_cnt;
  uint32_t crc_calc_callback_cnt;
  uint32_t frame_too_small_cnt;
  uint8_t expected_sequence_number;
}ahdlc_decoder_stats;

typedef struct {
  unsigned int ack_requested      : 1;
  unsigned int frame_is_ack       : 1;
  unsigned int frame_is_encrypted : 1;
  unsigned int frame_is_continued : 1;
  unsigned int reserved           : 2;
  unsigned int frame_valid        : 1;  /* Bit to make field non-zero */
  unsigned int extended_bits      : 1;
}__attribute__((packed)) frame_bits_t ;

typedef union {
  uint8_t value;
  frame_bits_t bit;
}frame_control_field_t;


typedef struct {
  uint32_t enc_ctr;
  uint32_t *pdu;
  crc_16_t calculated_crc_16;
  uint16_t buffer_index;
  frame_bits_t flags;
  uint8_t sequence;
  uint8_t ack_num;
  frame_control_field_t control_bits;
}ahdlc_frame_t;

typedef struct {
  uint32_t encoded_frame_cnt;
  uint32_t encryption_engine_callback_count;
  uint32_t crc_calc_callback_count;
  uint32_t sequence_number;
  mmwave_encoder_machine_state encoder_state;
}ahdlc_encoder_stats;

typedef struct {
  crc_callback crc_cb;
  enc_callback encryption_cb;
  uint8_t* frame_buffer;
  uint32_t buffer_len;
  ahdlc_encoder_stats stats;
  ahdlc_frame_t frame_info;
}ahdlc_frame_encoder_t;

/* Callback for writing a decoded byte. */
typedef ahdlc_op_return (*decoder_write_callback)(void *hdl, uint8_t byte);

typedef struct {
  crc_callback crc_cb;
  enc_callback enc_cb;
  decoder_write_callback dec_w_cb;
  uint8_t* pdu_buffer;
  uint32_t buffer_len;
  frame_control_field_t control_bits;
  ahdlc_frame_t frame_info;
  ahdlc_decoder_stats stats;
  ahdlc_decoder_machine_state decoder_state;
  crc_16_stack_t crc_stack;
  uint8_t expecting_escape; /* This should be part of state_machine */
  uint8_t reset_on_next_byte;
}ahdlc_frame_decoder_t;

#endif /* LIB_INC_FRAME_LAYER_TYPES_H_ */
