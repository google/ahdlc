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

#include <gtest/gtest.h>

#include <string.h>
#include <stdlib.h>

#include <string>

#include "../../lib/inc/crc_16.h"
#include "../../lib/inc/frame_layer.h"

using std::string;

/* A simple string message including escape bytes */
static const uint8_t test_ascii_message[] = "Sophie {~the~} Scientist";
/* A framed version of the above test message */
static const uint8_t test_message_encoded[] = {
    0x7e,0x40,0x1,0x53,0x6f,0x70,0x68,0x69,0x65,0x20,0x7b,0x7d,0x5e,0x74,0x68,
    0x65,0x7d,0x5e,0x7d,0x5d,0x20,0x53,0x63,0x69,0x65,0x6e,0x74,0x69,0x73,0x74,
    0x0,0xfb,0xc0,0x7e
};

const double theoretical_random_success_rate = 1.0 / 65535.0;

static ahdlc_frame_encoder_t encoder_handle;
static ahdlc_frame_decoder_t decoder_handle;

void encodeTestFrame(ahdlc_frame_encoder_t *handle) {
  EXPECT_EQ(EncodeNewFrame(handle), AHDLC_OK);
  EXPECT_EQ(EncodeBuffer(handle, test_ascii_message,
      sizeof(test_ascii_message)), AHDLC_OK);
  EXPECT_EQ(EncodeFinalize(handle), AHDLC_OK);
}

class FrameTest : public ::testing::Test {
 protected:
  // You can remove any or all of the following functions if its body
  // is empty.

  FrameTest() {
    // You can do set-up work for each test here.
  }

  virtual ~FrameTest() {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  virtual void SetUp() {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  virtual void TearDown() {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  // Objects declared here can be used by all tests in the test case for Foo.
};

TEST_F(FrameTest, InitTests) {

  ahdlc_frame_encoder_t encoder_handle;

  encoder_handle.buffer_len = min_payload_size;
  encoder_handle.frame_buffer = (uint8_t*)malloc(encoder_handle.buffer_len);

  /* Test for init checks */
  EXPECT_EQ(ahdlcEncoderInit(&encoder_handle, CRC16), AHDLC_OK);
}

TEST_F(FrameTest, EncodeHandleInitTest) {
  encoder_handle.buffer_len = 1024;
  encoder_handle.frame_buffer = (uint8_t*)malloc(encoder_handle.buffer_len);

  EXPECT_EQ(ahdlcEncoderInit(&encoder_handle, CRC16), AHDLC_OK);
}

TEST_F(FrameTest, EncodeNewFrameTest) {
  EXPECT_EQ(EncodeNewFrame(&encoder_handle), AHDLC_OK);
  EXPECT_EQ(encoder_handle.frame_info.buffer_index, 3u);
}

TEST_F(FrameTest, EncodeBufferTest) {
  EXPECT_EQ(EncodeBuffer(&encoder_handle, test_ascii_message,
      sizeof(test_ascii_message)), AHDLC_OK);
}

TEST_F(FrameTest, EncoderEndianTest) {
  EXPECT_EQ(EncodeNewFrame(&encoder_handle), AHDLC_OK);
  EXPECT_EQ(EncodeBuffer(&encoder_handle, test_ascii_message,
      sizeof(test_ascii_message)), AHDLC_OK);

  // From the above test message (0xfb,0xc0),
  const uint8_t high_byte = 0xFB;
  const uint8_t low_byte = 0xC0;

  // Check for network byte order (BE)
  EXPECT_EQ(high_byte, encoder_handle.frame_buffer[encoder_handle.frame_info.buffer_index - 3]);
  EXPECT_EQ(low_byte, encoder_handle.frame_buffer[encoder_handle.frame_info.buffer_index - 2]);
}

TEST_F(FrameTest, EncodeFinalizeTest) {
  EXPECT_EQ(EncodeFinalize(&encoder_handle), AHDLC_OK);
}

TEST_F(FrameTest, EncodeTestStartDelimiter) {
  EXPECT_EQ(encoder_handle.frame_buffer[0], frame_marker);
}

TEST_F(FrameTest, EncodeTestFlags) {
  EXPECT_EQ(encoder_handle.frame_buffer[1],
      encoder_handle.frame_info.control_bits.value);
}

TEST_F(FrameTest, EncodeTestSequence) {
  uint8_t current_seq = encoder_handle.frame_info.sequence;

  encodeTestFrame(&encoder_handle);

  EXPECT_EQ(current_seq + 1,
      encoder_handle.frame_info.sequence);
}

TEST_F(FrameTest, EncodeTestEndDelimiter) {
  EXPECT_EQ(frame_marker,
      encoder_handle.frame_buffer[encoder_handle.frame_info.buffer_index - 1]);
}

TEST_F(FrameTest, DecodeFrameInitTest) {
  decoder_handle.buffer_len = 1024;
  decoder_handle.pdu_buffer = (uint8_t*)malloc(decoder_handle.buffer_len);
  EXPECT_EQ(AhdlcDecoderInit(&decoder_handle, CRC16, NULL), AHDLC_OK);
}

TEST_F(FrameTest, DecodeFrameTest) {
  encodeTestFrame(&encoder_handle);

  EXPECT_EQ(AHDLC_COMPLETE,
      DecoderBuffer(&decoder_handle,
            encoder_handle.frame_buffer, encoder_handle.buffer_len));
  //  Decoded buffer should match original payload
  int test_message_differences = memcmp(test_ascii_message,
      decoder_handle.pdu_buffer, decoder_handle.frame_info.buffer_index);
  //  Test decoded message
  EXPECT_EQ(0, test_message_differences);
}


TEST_F(FrameTest, CorruptFrameTest) {
  /* Reach in and corrupt a byte of the frame */
  uint8_t saved_byte =
      encoder_handle.frame_buffer[(sizeof(test_ascii_message) / 2)];

  encoder_handle.frame_buffer[(sizeof(test_ascii_message) / 2)] =
      saved_byte + 2;

  /* We should now see an error */
  EXPECT_EQ(DecoderBuffer(&decoder_handle,
      encoder_handle.frame_buffer, encoder_handle.buffer_len), AHDLC_ERROR);
}

TEST_F(FrameTest, EncodeTestBufferLimit) {

  uint8_t test_byte = 'b'; /* A non escaped byte */

  ahdlc_op_return code;

  EXPECT_EQ(EncodeNewFrame(&encoder_handle), AHDLC_OK);

  uint32_t i = encoder_handle.frame_info.buffer_index;

  for (; i < (encoder_handle.buffer_len + 1); ++i) {
    code = EncodeAddByteToFrameBuffer(&encoder_handle, test_byte);
  }

  EXPECT_EQ(ENCODE_BUFFER_TOO_SMALL, encoder_handle.stats.encoder_state);
  EXPECT_EQ(code, AHDLC_ERROR);
  EXPECT_EQ(EncodeNewFrame(&encoder_handle), AHDLC_OK);
  EXPECT_EQ(encoder_handle.frame_info.buffer_index, 3u);

}

TEST_F(FrameTest, DecodeConsecutiveFrameMarkers) {

  uint8_t test_frame[256];

  memset(test_frame, frame_marker, sizeof(test_frame));

  /* Reset machine state */
  DecodeFrameByte(&decoder_handle, frame_marker);

  uint32_t bad_crc_count       = decoder_handle.stats.num_decoded_bad_crc;
  uint32_t out_of_frame_count  = decoder_handle.stats.out_of_frame_byte_cnt;
  uint32_t good_frame_count    = decoder_handle.stats.good_frame_cnt;
  uint16_t previous_buffer_index = decoder_handle.frame_info.buffer_index;

  /* Should yield an error since no frames were extracted */
  EXPECT_EQ(AHDLC_ERROR, DecoderBuffer(&decoder_handle,
      test_frame, sizeof(test_frame)));
  /* Should have a higher error count */
  EXPECT_EQ(bad_crc_count, decoder_handle.stats.num_decoded_bad_crc);
  EXPECT_EQ(out_of_frame_count, decoder_handle.stats.out_of_frame_byte_cnt);
  EXPECT_EQ(good_frame_count, decoder_handle.stats.good_frame_cnt);
  EXPECT_EQ(decoder_handle.frame_info.buffer_index, previous_buffer_index);
}

TEST_F(FrameTest, PaddedFrameMarkers) {

  uint8_t test_frame[256];
  const uint8_t num_starts = 10;

  memset(test_frame, frame_marker, sizeof(test_frame));

  encodeTestFrame(&encoder_handle);

  // Set test_frame to all frame_marker bytes
  memset(test_frame, frame_marker, sizeof(test_frame));

  memcpy(&test_frame[num_starts], encoder_handle.frame_buffer,
      encoder_handle.frame_info.buffer_index);

  uint32_t num_start_errors = decoder_handle.stats.num_decoded_bad_crc;

  EXPECT_EQ(AHDLC_COMPLETE, DecoderBuffer(&decoder_handle,
      test_frame, sizeof(test_frame)));
  EXPECT_EQ(decoder_handle.stats.num_decoded_bad_crc, num_start_errors);
}

TEST_F(FrameTest, DecoderOverrunTest) {
  uint8_t *big_frame = (uint8_t*)malloc(decoder_handle.buffer_len * 2);
  //  Set the flags up so that the state machine does not reset, on
  //  every byte.
  memset(big_frame, encoder_handle.frame_info.control_bits.value,
      decoder_handle.buffer_len * 2);
  big_frame[0] = frame_marker;

  DecodeFrameByte(&decoder_handle, frame_marker);
  DecodeFrameByte(&decoder_handle, frame_marker);

  /* Should yield an error since no frames were extracted */
  EXPECT_EQ(AHDLC_ERROR, DecoderBuffer(&decoder_handle,
      big_frame, decoder_handle.buffer_len * 2));
  EXPECT_EQ(DECODE_BUFFER_TOO_SMALL, decoder_handle.decoder_state);
}

TEST_F(FrameTest, DecoderStateReset) {
  DecodeFrameByte(&decoder_handle, frame_marker);
  ahdlc_decoder_machine_state saved_state = decoder_handle.decoder_state;
  DecodeFrameByte(&decoder_handle, frame_marker);
  EXPECT_EQ(saved_state, decoder_handle.decoder_state);
  DecodeFrameByte(&decoder_handle, 'a');
  EXPECT_EQ(DECODE_EXPECTING_SEQUENCE, decoder_handle.decoder_state);
}

TEST_F(FrameTest, DecoderMinimalPayload) {
    ahdlc_frame_decoder_t dec;          // decoder handle
    ahdlc_frame_encoder_t enc;          // encoder handle

    dec.buffer_len = 1024;
    dec.pdu_buffer = (uint8_t*) malloc(dec.buffer_len);

    enc.buffer_len = 1024;
    enc.frame_buffer = (uint8_t*) malloc(enc.buffer_len);

    ahdlcEncoderInit(&enc, CRC16);
    AhdlcDecoderInit(&dec, CRC16, NULL);

    EncodeNewFrame(&enc);
    ahdlc_op_return enc_ret = EncodeFinalize(&enc);

    EXPECT_EQ(AHDLC_OK, enc_ret);

    ahdlc_op_return ret = DecoderBuffer(&dec,
        enc.frame_buffer, enc.buffer_len);

    EXPECT_EQ(AHDLC_COMPLETE, ret);
    EXPECT_EQ(6, enc.frame_info.buffer_index);
}

TEST_F(FrameTest, ConsecutiveFrameTest) {
  uint32_t num_test_frames = 50;

  ahdlc_frame_decoder_t dec;          // decoder handle
  ahdlc_frame_encoder_t enc;          // encoder handle

  dec.buffer_len = 1024;
  dec.pdu_buffer = (uint8_t*) malloc(dec.buffer_len);

  enc.buffer_len = 1024;
  enc.frame_buffer = (uint8_t*) malloc(enc.buffer_len);

  ahdlcEncoderInit(&enc, CRC16);
  AhdlcDecoderInit(&dec, CRC16, NULL);

  uint32_t i;

  for (i = 0; i < num_test_frames; ++i) {

    char test_buffer[] = "Framed Packet";

    EncodeNewFrame(&enc);
    EncodeBuffer(&enc, (uint8_t*) test_buffer, sizeof(test_buffer));
    EncodeFinalize(&enc);

    uint32_t read_index = 0;

    ahdlc_op_return code;

    do {
       code = DecodeFrameByte(&dec, enc.frame_buffer[read_index++]);

      if (read_index > enc.buffer_len) {
        break;
      }

    } while (code != AHDLC_COMPLETE);
  }
}

TEST_F(FrameTest, ConsecutiveSingleMarkerFrameTest) {
  uint32_t num_test_frames = 5;

  ahdlc_frame_decoder_t dec;          // decoder handle
  ahdlc_frame_encoder_t enc;          // encoder handle

  dec.buffer_len = 1024;
  dec.pdu_buffer = (uint8_t*) malloc(dec.buffer_len);

  enc.buffer_len = 1024;
  enc.frame_buffer = (uint8_t*) malloc(enc.buffer_len);

  ahdlcEncoderInit(&enc, CRC16);
  AhdlcDecoderInit(&dec, CRC16, NULL);

  char test_message[] = "Framed Packet";

  EncodeNewFrame(&enc);
  EncodeBuffer(&enc, (uint8_t*) test_message, sizeof(test_message));
  EncodeFinalize(&enc);

  uint32_t shorted_frame_size = (enc.frame_info.buffer_index - 1);
  uint32_t test_buffer_size = shorted_frame_size * num_test_frames;
  uint8_t *test_buffer = (uint8_t*)malloc(test_buffer_size);
  bzero(test_buffer, test_buffer_size);

  uint32_t i;
  /* populate a continuous buffer with missing end markers */
  for (i = 0; i < num_test_frames; ++i) {
    memcpy(&test_buffer[i * shorted_frame_size], enc.frame_buffer, shorted_frame_size);
  }
  /* parse all the bytes */
  for (i = 0; i < test_buffer_size; ++i) {
    DecodeFrameByte(&dec, test_buffer[i]);
  }

  EXPECT_EQ(num_test_frames - 1, dec.stats.good_frame_cnt);
  /* Sending an end marker should trigger another successful decode */
  DecodeFrameByte(&dec, frame_marker);
  EXPECT_EQ(num_test_frames, dec.stats.good_frame_cnt);
}

TEST_F(FrameTest, B34501481) {
  #define message_one_payload  0x08, 0x02, 0x50, 0x80, 0xc4, 0xaa, 0x22
  #define message_two_payload  0x08, 0x01, 0x50, 0x00

  //  sizeof rx buffer as of B/34501481
  const uint8_t wombat_buffer_size = 128;

  uint8_t message_one_proper[] = {0x7e, 0x40, 0x00, message_one_payload, 0x30, 0x01, 0x7e};
  uint8_t message_one_offset[] = {0x40, 0x00, message_one_payload, 0x30, 0x01, 0x7e, 0x00};

  uint8_t message_two_proper[] = {0x7e, 0x40, 0x01, message_two_payload, 0xe4, 0x41, 0x7e};
  uint8_t message_two_offset[] = {0x40, 0x01, message_two_payload, 0xe4, 0x41, 0x7e, 0x00};

  //  Setup a fresh decoder for tests
  ahdlc_frame_decoder_t dec;
  dec.buffer_len = wombat_buffer_size;
  dec.pdu_buffer = (uint8_t*) malloc(dec.buffer_len);
  AhdlcDecoderInit(&dec, CRC16, NULL);

  //  Decode both messages in proper frame sequence, to verify that
  //  our decoder is working properly.
  ahdlc_op_return ret = DecoderBuffer(&dec,
      message_one_proper, sizeof(message_one_proper));

  uint8_t message_one_pdu[] = {message_one_payload};
  //  Should complete a frame decode and our contents should match the payload
  EXPECT_EQ(AHDLC_COMPLETE, ret);
  EXPECT_EQ(0, memcmp(dec.pdu_buffer, message_one_pdu,
      dec.frame_info.buffer_index));

  ret = DecoderBuffer(&dec, message_two_proper, sizeof(message_two_proper));
  //  Should complete a frame decode and our contents should match the payload
  uint8_t message_two_pdu[] = {message_two_payload};
  EXPECT_EQ(AHDLC_COMPLETE, ret);
  EXPECT_EQ(0, memcmp(dec.pdu_buffer, message_two_pdu, dec.frame_info.buffer_index));
  //  In total, we should now have two good frames.
  EXPECT_EQ(dec.stats.good_frame_cnt, 2u);

  //  Reset decoder state, and pass in the offset frame bytes, sequentially.
  //  This is how Wombat processes bytes coming in from the UART.
  AhdlcDecoderInit(&dec, CRC16, NULL);

  //  First frame...
  for (unsigned int i = 0; i < sizeof(message_one_offset); ++i) {
    ret = DecodeFrameByte(&dec, message_one_offset[i]);
    if (ret == AHDLC_COMPLETE) {
      EXPECT_EQ(AHDLC_COMPLETE, ret);
      EXPECT_EQ(0, memcmp(dec.pdu_buffer, message_one_pdu, dec.frame_info.buffer_index));
    }
  }

  //  Second frame...
  for (unsigned int i = 0; i < sizeof(message_two_offset); ++i) {
    ret = DecodeFrameByte(&dec, message_two_offset[i]);
    if (ret == AHDLC_COMPLETE) {
      EXPECT_EQ(AHDLC_COMPLETE, ret);
      EXPECT_EQ(0, memcmp(dec.pdu_buffer, message_two_pdu, dec.frame_info.buffer_index));
    }
  }

}

TEST_F(FrameTest, DecodeRandomDataTest1Gig) {
  double good_frames = decoder_handle.stats.good_frame_cnt;

  uint32_t i = 1024 * 1000 * 1000;  // 1 Gigabyte

  do {
    uint8_t random_byte = (uint8_t)(random() % 256);
    DecodeFrameByte(&decoder_handle, random_byte);
  } while (--i);

  double measured_random_success_rate;

  uint32_t random_good_frames = decoder_handle.stats.good_frame_cnt - good_frames;

  if (random_good_frames) {
    measured_random_success_rate = (theoretical_random_success_rate *
        (decoder_handle.stats.good_frame_cnt - good_frames)) /
            random_good_frames;
  } else {
    measured_random_success_rate =  0;
  }

  /* Measured rate should be less than or equal to the theoretical rate */
  EXPECT_LE(measured_random_success_rate, theoretical_random_success_rate);
}

