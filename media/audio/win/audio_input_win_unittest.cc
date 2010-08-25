// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

#include "base/basictypes.h"
#include "media/audio/audio_io.h"
#include "media/audio/audio_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// This class allows to find out if the callbacks are occurring as
// expected and if any error has been reported.
class TestInputCallback : public AudioInputStream::AudioInputCallback {
 public:
  explicit TestInputCallback(int max_data_bytes)
      : callback_count_(0),
        had_error_(0),
        was_closed_(0),
        max_data_bytes_(max_data_bytes) {
  }
  virtual void OnData(AudioInputStream* stream, const uint8* data,
                      uint32 size) {
    ++callback_count_;
    // Read the first byte to make sure memory is good.
    if (size) {
      ASSERT_LE(static_cast<int>(size), max_data_bytes_);
      EXPECT_TRUE(data[0] >= 0);
    }
  }
  virtual void OnClose(AudioInputStream* stream) {
    ++was_closed_;
  }
  virtual void OnError(AudioInputStream* stream, int code) {
    ++had_error_;
  }
  // Returns how many times OnData() has been called.
  int callback_count() const {
    return callback_count_;
  }
  // Returns how many times the OnError callback was called.
  int had_error() const {
    return had_error_;
  }

  void set_error(bool error) {
    had_error_ += error ? 1 : 0;
  }
  // Returns how many times the OnClose callback was called.
  int was_closed() const {
    return was_closed_;
  }

 private:
  int callback_count_;
  int had_error_;
  int was_closed_;
  int max_data_bytes_;
};

// Specializes TestInputCallback to simulate a sink that blocks for some time
// in the OnData callback.
class TestInputCallbackBlocking : public TestInputCallback {
 public:
  TestInputCallbackBlocking(int max_data_bytes, int block_after_callback,
                            int block_for_ms)
      : TestInputCallback(max_data_bytes),
        block_after_callback_(block_after_callback),
        block_for_ms_(block_for_ms) {
  }
  virtual void OnData(AudioInputStream* stream, const uint8* data,
                      uint32 size) {
    // Call the base, which increments the callback_count_.
    TestInputCallback::OnData(stream, data, size);
    if (callback_count() > block_after_callback_)
      ::Sleep(block_for_ms_);
  }

 private:
  int block_after_callback_;
  int block_for_ms_;
};

bool CanRunAudioTests() {
  AudioManager* audio_man = AudioManager::GetAudioManager();
  if (NULL == audio_man)
    return false;
  return (audio_man->HasAudioInputDevices() &&
          ::GetEnvironmentVariableW(L"CHROME_HEADLESS", NULL, 0) == 0);
}

}  // namespace.

// Test that AudioInputStream rejects out of range parameters.
TEST(WinAudioInputTest, SanityOnMakeParams) {
  if (!CanRunAudioTests())
    return;
  AudioManager* audio_man = AudioManager::GetAudioManager();
  AudioManager::Format fmt = AudioManager::AUDIO_PCM_LINEAR;
  EXPECT_TRUE(NULL == audio_man->MakeAudioInputStream(fmt, 8, 8000, 16, 0));
  EXPECT_TRUE(NULL == audio_man->MakeAudioInputStream(fmt, 1, 1024 * 1024, 16,
                                                      0));
  EXPECT_TRUE(NULL == audio_man->MakeAudioInputStream(fmt, 2, 8000, 80, 0));
  EXPECT_TRUE(NULL == audio_man->MakeAudioInputStream(fmt, 2, 8000, 80,
                                                      1024 * 4096));
  EXPECT_TRUE(NULL == audio_man->MakeAudioInputStream(fmt, -2, 8000, 16, 0));
  EXPECT_TRUE(NULL == audio_man->MakeAudioInputStream(fmt, 2, -8000, 16, 0));
  EXPECT_TRUE(NULL == audio_man->MakeAudioInputStream(fmt, 2, 8000, -16, 0));
  EXPECT_TRUE(NULL == audio_man->MakeAudioInputStream(fmt, 2, 8000, 16, -1024));
}

// Test create and close of an AudioInputStream without recording audio.
TEST(WinAudioInputTest, CreateAndClose) {
  if (!CanRunAudioTests())
    return;
  AudioManager* audio_man = AudioManager::GetAudioManager();
  AudioInputStream* ais = audio_man->MakeAudioInputStream(
      AudioManager::AUDIO_PCM_LINEAR, 2, 8000, 16, 0);
  ASSERT_TRUE(NULL != ais);
  ais->Close();
}

// Test create, open and close of an AudioInputStream without recording audio.
TEST(WinAudioInputTest, OpenAndClose) {
  if (!CanRunAudioTests())
    return;
  AudioManager* audio_man = AudioManager::GetAudioManager();
  AudioInputStream* ais = audio_man->MakeAudioInputStream(
      AudioManager::AUDIO_PCM_LINEAR, 2, 8000, 16, 0);
  ASSERT_TRUE(NULL != ais);
  EXPECT_TRUE(ais->Open());
  ais->Close();
}

// Test a normal recording sequence using an AudioInputStream.
TEST(WinAudioInputTest, Record) {
  if (!CanRunAudioTests())
    return;
  AudioManager* audio_man = AudioManager::GetAudioManager();
  const int kSamplingRate = 8000;
  const int kSamplesPerPacket = kSamplingRate / 20;
  AudioInputStream* ais = audio_man->MakeAudioInputStream(
      AudioManager::AUDIO_PCM_LINEAR, 2, kSamplingRate, 16, kSamplesPerPacket);
  ASSERT_TRUE(NULL != ais);
  EXPECT_TRUE(ais->Open());

  TestInputCallback test_callback(kSamplesPerPacket * 4);
  ais->Start(&test_callback);
  // Verify at least 500ms worth of audio was recorded, after giving sufficient
  // extra time.
  Sleep(590);
  EXPECT_GE(test_callback.callback_count(), 10);
  EXPECT_FALSE(test_callback.had_error());

  ais->Stop();
  ais->Close();
}

// Test a recording sequence with delays in the audio callback.
TEST(WinAudioInputTest, RecordWithSlowSink) {
  if (!CanRunAudioTests())
    return;
  AudioManager* audio_man = AudioManager::GetAudioManager();
  const int kSamplingRate = 8000;
  const int kSamplesPerPacket = kSamplingRate / 20;
  AudioInputStream* ais = audio_man->MakeAudioInputStream(
      AudioManager::AUDIO_PCM_LINEAR, 2, kSamplingRate, 16, kSamplesPerPacket);
  ASSERT_TRUE(NULL != ais);
  EXPECT_TRUE(ais->Open());

  // We should normally get a callback every 50ms, and a 20ms delay inside each
  // callback should not change this sequence.
  TestInputCallbackBlocking test_callback(kSamplesPerPacket * 4, 0, 20);
  ais->Start(&test_callback);
  // Verify at least 500ms worth of audio was recorded, after giving sufficient
  // extra time.
  Sleep(590);
  EXPECT_GE(test_callback.callback_count(), 10);
  EXPECT_FALSE(test_callback.had_error());

  ais->Stop();
  ais->Close();
}
