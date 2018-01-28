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

#ifndef LIB_INC_FRAME_LAYER_H_
#define LIB_INC_FRAME_LAYER_H_

#include <stdint.h>

#include "frame_layer_types.h"

#define START_DELIMITER_OFFSET  (0)
#define SEQUENCE_OFFSET         (1)
#define CONTROL_BITS_OFFSET     (2)

#define START_OF_PDU_ACKLESS_UNENCRYPTED (3)

/* Special bytes */
extern const uint8_t frame_marker;
extern const uint8_t escape_marker;
extern const uint8_t escaped_start;
extern const uint8_t escaped_escape;

/* Other constants */
extern const uint8_t min_payload_size; /* end marker not actually required */
extern const uint8_t crc_size;
extern const uint8_t ack_frame_size_unencrypted;

#ifdef __cplusplus
extern "C" {
#endif

  ahdlc_op_return AhdlcDecoderInit(ahdlc_frame_decoder_t *handle,
      crc_callback crc_function, decoder_write_callback dec_w_cb);
  ahdlc_op_return ahdlcEncoderInit(ahdlc_frame_encoder_t *handle,
      crc_callback crc_function);

  /* Creates a new packet after resetting any current operation. */
  ahdlc_op_return EncodeNewFrame(
      ahdlc_frame_encoder_t *handle);
  /* Adds a byte to a packet */
  ahdlc_op_return EncodeAddByteToFrameBuffer(
      ahdlc_frame_encoder_t *handle, uint8_t byte);

  ahdlc_op_return EncodeBuffer(ahdlc_frame_encoder_t *handle,
      const uint8_t *buffer, uint32_t len);
  /* Write length and calculate CRC */
  ahdlc_op_return EncodeFinalize(ahdlc_frame_encoder_t *handle);

  /* Decode one byte at a time, returning the state of the decode machine */
  ahdlc_op_return decodeMMwaveFrame(uint8_t *raw_data, uint8_t num_bytes);
  ahdlc_op_return DecodeFrameByte(ahdlc_frame_decoder_t *handle,
      uint8_t byte);

  /*
   * When this function is called, it is expected that a frame is found in the
   * supplied buffer. If not and error is returned. Most applications will use
   * the decodeFrameByte function.
   */
  ahdlc_op_return DecoderBuffer(ahdlc_frame_decoder_t *handle,
      uint8_t *buffer, uint32_t buffer_length);

  void PrintBuffer(uint8_t *buffer, uint32_t buffer_len);

  void decoderResetState(ahdlc_frame_decoder_t *handle);

  static inline void decoderPushCRC(crc_16_stack_t *crc_stack, uint16_t crc) {
    ++crc_stack->crc_index;

    if (crc_stack->crc_index > 2) {
      crc_stack->crc_index = 0;
    }

    crc_stack->crc_array[crc_stack->crc_index].crc_value = crc;
  }

  static inline crc_16_t decoderGetCurrentCRC(ahdlc_frame_decoder_t *handle) {
    return  handle->crc_stack.crc_array[handle->crc_stack.crc_index];
  }

  static inline crc_16_t decoderGetFrameCRC(crc_16_stack_t *crc_stack) {
    crc_16_t crc;
    crc.crc_value = 0;

    switch (crc_stack->crc_index) {
    case 0:
      crc = crc_stack->crc_array[1];
      break;
    case 1:
      crc = crc_stack->crc_array[2];
      break;
    case 2:
      crc = crc_stack->crc_array[0];
      break;
    default:
      break;
    }

    return crc;
  }

  static inline ahdlc_op_return encoderWriteByte(
      ahdlc_frame_encoder_t *hdl, uint8_t byte) {

  if (hdl->buffer_len > hdl->frame_info.buffer_index) {
    hdl->frame_buffer[hdl->frame_info.buffer_index++] = byte;
  } else {
    hdl->stats.encoder_state = ENCODE_BUFFER_TOO_SMALL;
    return AHDLC_ERROR;
  }
    return AHDLC_OK;
  }

  static inline ahdlc_op_return decoderWriteByte(
      void *ctx, uint8_t byte) {

    ahdlc_frame_decoder_t* hdl = (ahdlc_frame_decoder_t*)ctx;
    if (hdl->buffer_len > hdl->frame_info.buffer_index) {
      hdl->pdu_buffer[hdl->frame_info.buffer_index++] = byte;
    } else {
      hdl->decoder_state = DECODE_BUFFER_TOO_SMALL;
      return AHDLC_ERROR;
    }
    return AHDLC_OK;
  }

#ifdef __cplusplus
 }
#endif

#endif
