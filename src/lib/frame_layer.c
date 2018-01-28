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

#include "inc/frame_layer.h"

#include "stdint.h"
#include <string.h>

/* Special bytes */
const uint8_t frame_marker   = 0x7E;
const uint8_t escape_marker  = 0x7D;
const uint8_t escaped_start  = 0x5E;
const uint8_t escaped_escape = 0x5D;

/* Other constants */
const uint8_t min_payload_size = 0; /* end marker not actually required */
const uint8_t crc_size = sizeof(uint16_t);
const uint8_t ack_frame_size_unencrypted = 6;
const uint16_t initial_crc_value = 0;


ahdlc_op_return ahdlcEncoderInit(ahdlc_frame_encoder_t *handle,
    crc_callback crc_function) {
  /* Set CRC calc function */
  handle->crc_cb = crc_function;
  memset(&handle->stats, 0, sizeof(ahdlc_encoder_stats));
  memset(&handle->frame_info, 0, sizeof(ahdlc_frame_t));
  memset(handle->frame_buffer, 0, sizeof(uint8_t) * handle->buffer_len);
  if (handle->buffer_len < min_payload_size) {
    return AHDLC_BUFFER_TOO_SMALL;
  }

  return AHDLC_OK;
}

/* Creates a new packet after resetting any current operation. */
ahdlc_op_return EncodeNewFrame(ahdlc_frame_encoder_t *handle) {
  ahdlc_op_return code = AHDLC_OK;

  if (handle->frame_info.control_bits.bit.frame_is_ack) {
    code = AHDLC_ERROR;  // No support yet
  } else if (handle->frame_info.control_bits.bit.frame_is_encrypted) {
    code = AHDLC_ERROR;  // No support yet
  } else {
    handle->frame_info.control_bits.bit.frame_valid = AHDLC_TRUE;
    handle->frame_info.calculated_crc_16.crc_value = initial_crc_value;
    handle->frame_info.buffer_index = 0;
    handle->stats.encoder_state = ENCODE_READY;
    code = encoderWriteByte(handle, frame_marker);
    code = EncodeAddByteToFrameBuffer(handle,
                                      handle->frame_info.control_bits.value);
    code = EncodeAddByteToFrameBuffer(handle, handle->frame_info.sequence++);
  }

  return code;
}

/* Takes a raw data buffer and encodes it into a frame */
ahdlc_op_return EncodeBuffer(ahdlc_frame_encoder_t *handle,
                             const uint8_t *buffer, uint32_t buffer_len) {
  uint32_t i;
  ahdlc_op_return status = AHDLC_OK;

  for (i = 0; i < buffer_len; ++i) {
    status = EncodeAddByteToFrameBuffer(handle, buffer[i]);
    if (status < 0) {
      break;
    }
  }

  if (status == AHDLC_OK) {
    status = EncodeFinalize(handle);
  }

  return status;
}

/* Adds a byte to a frame buffer */
ahdlc_op_return EncodeAddByteToFrameBuffer(ahdlc_frame_encoder_t *handle,
                                           uint8_t byte) {
  ahdlc_op_return code = AHDLC_OK;

  if (handle->stats.encoder_state < 0) {
    // inc error counter stat
    code = AHDLC_ERROR;
  } else {
      /* Add CRC before escape block */
      handle->frame_info.calculated_crc_16.crc_value = handle->crc_cb(
          handle->frame_info.calculated_crc_16.crc_value, &byte, sizeof(byte));

      if (byte == frame_marker) {
        code = encoderWriteByte(handle, escape_marker);
        code = encoderWriteByte(handle, escaped_start);
      } else if (byte == escape_marker) {
        code = encoderWriteByte(handle, escape_marker);
        code = encoderWriteByte(handle, escaped_escape);
      } else {
        code = encoderWriteByte(handle, byte);
      }
    }

  return code;
}

/* Write length and calculate CRC */
ahdlc_op_return EncodeFinalize(ahdlc_frame_encoder_t *hdl) {
  ahdlc_op_return code = AHDLC_OK;
  crc_16_t crc;

  if (hdl->stats.encoder_state == ENCODE_FINALIZED) {
    return AHDLC_OK;
  }

  /* Calc CRC */
  crc.crc_value = hdl->frame_info.calculated_crc_16.crc_value;
  /* Endian handled in header, always send in BE */
  code = EncodeAddByteToFrameBuffer(hdl, crc.bytes.high);
  code = EncodeAddByteToFrameBuffer(hdl, crc.bytes.low);
  code = encoderWriteByte(hdl, frame_marker);

  hdl->stats.encoder_state = ENCODE_FINALIZED;

  return code;
}

ahdlc_op_return AhdlcDecoderInit(ahdlc_frame_decoder_t *handle,
    crc_callback crc_function, decoder_write_callback dec_w_cb) {
  /* If user has provided custom write callback set it. Otherwise use
   * provided function.
   */
  if (dec_w_cb) {
    handle->dec_w_cb = dec_w_cb;
  } else {
	if (handle->buffer_len < min_payload_size) {
	  return AHDLC_BUFFER_TOO_SMALL;
	}
    handle->dec_w_cb = decoderWriteByte;
  }
  /* Set CRC calc function */
  handle->crc_cb = crc_function;
  handle->reset_on_next_byte = 1;
  handle->expecting_escape = 0;
  memset(&handle->stats, 0, sizeof(handle->stats));

  return AHDLC_OK;
}

ahdlc_op_return DecoderBuffer(ahdlc_frame_decoder_t *handle, uint8_t *raw_data,
                              uint32_t buffer_length) {
  ahdlc_op_return code = AHDLC_OK;

  uint32_t i;
  uint32_t frames_decoded = handle->stats.good_frame_cnt;

  /* scan over buffer and find a packet then stop */
  for (i = 0; i < buffer_length; ++i) {
    code = DecodeFrameByte(handle, raw_data[i]);
    /* stop when we have decoded a packet */
    if (code == AHDLC_COMPLETE) {
      break;
    }
  }
  /* if we did not decode a frame return error */
  if (frames_decoded == handle->stats.good_frame_cnt) {
    code = AHDLC_ERROR;
  }

  return code;
}

ahdlc_op_return DecodeFrameByte(ahdlc_frame_decoder_t *handle,
                                uint8_t raw_byte) {
  ahdlc_op_return code = AHDLC_OK;
  crc_16_t crc;
  uint8_t decoded_byte;

   if (raw_byte == frame_marker) {
     crc_16_t calculated_crc;

    if (handle->reset_on_next_byte) {
      // TODO (skeys) inc idle frame marker counter
    } else if (handle->frame_info.buffer_index > min_payload_size) {
      crc_16_t frame_crc;
      frame_crc.bytes.low =
          handle->pdu_buffer[--(handle->frame_info.buffer_index)];
      frame_crc.bytes.high =
          handle->pdu_buffer[--(handle->frame_info.buffer_index)];

      calculated_crc = decoderGetFrameCRC(&handle->crc_stack);
      if (frame_crc.crc_value == calculated_crc.crc_value) {
        //        printf("Decode complete. Good frame !!!\n");
        handle->decoder_state = DECODE_COMPLETE_GOOD;
        ++handle->stats.good_frame_cnt;
        code = AHDLC_COMPLETE;
      } else {
        handle->decoder_state = DECODE_COMPLETE_BAD_CRC;
        ++handle->stats.num_decoded_bad_crc;
        code = AHDLC_ERROR;
      }
    } else {
      ++handle->stats.frame_too_small_cnt;
    }

    handle->reset_on_next_byte = 1;
    return code;
  }

  if (handle->reset_on_next_byte) {
    memset(&(handle->crc_stack), 0, sizeof(crc_16_stack_t));
    memset(&(handle->frame_info), 0, sizeof(ahdlc_frame_t));
    handle->decoder_state = DECODE_EXPECTING_FLAGS;
    handle->reset_on_next_byte = 0;
    handle->expecting_escape = 0;
  }

  /* Check to see if we need to process an escaped byte */
  if (raw_byte == escape_marker) {
    handle->expecting_escape = 1;
    return code;
  }

  decoded_byte = raw_byte;

  /* Potentially extract an escaped value */
  if (handle->expecting_escape) {
    handle->expecting_escape = 0;

    if (raw_byte == escaped_start) {
      decoded_byte = frame_marker;
    } else if (raw_byte == escaped_escape) {
      decoded_byte = escape_marker;
    } else {
      ++handle->stats.invalid_escape_cnt;
      handle->decoder_state = DECODE_INVALID_ESCAPE_SEQ;
      handle->reset_on_next_byte = 1;
      code = AHDLC_ERROR;
      return code;
    }
  }

  /* Add CRC */
  /* TODO (skeys) instead of calling and caching the CRC we can just cache
   * a couple bytes and process the cache when we see a frame marker. */
  crc = decoderGetCurrentCRC(handle);
  crc.crc_value = handle->crc_cb(crc.crc_value, &decoded_byte, sizeof(decoded_byte));
  decoderPushCRC(&(handle->crc_stack), crc.crc_value);

  /* Run escaped byte though the state machine */
  switch (handle->decoder_state) {
    case DECODE_EXPECTING_SEQUENCE:
      handle->frame_info.sequence = decoded_byte;
      handle->decoder_state = DECODE_EXPECTING_PDU;
      break;
    case DECODE_EXPECTING_FLAGS:
      handle->control_bits.value = decoded_byte;
      handle->reset_on_next_byte = AHDLC_TRUE;  /*  Default to error */
      if (!handle->control_bits.bit.frame_valid) {
        code = AHDLC_INVALID_FRAME;
      } else if (handle->control_bits.bit.frame_is_ack
          || handle->control_bits.bit.frame_is_encrypted) {
        code = AHDLC_CRC_ENGINE_FAILURE;  /* Unimplemented */
      } else {
        handle->decoder_state = DECODE_EXPECTING_SEQUENCE;
        handle->reset_on_next_byte = AHDLC_FALSE;
      }
      break;
    case DECODE_EXPECTING_PDU:
      code = handle->dec_w_cb(handle, decoded_byte);
      break;
    default:
      code = AHDLC_CRC_ENGINE_FAILURE;  /* Unimplemented */
      break;
  }

  return code;
}
