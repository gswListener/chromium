// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This is an application of a minimal host process in a Chromoting
// system. It serves the purpose of gluing different pieces together
// to make a functional host process for testing.
//
// It peforms the following functionality:
// 1. Connect to the GTalk network and register the machine as a host.
// 2. Accepts connection through libjingle.
// 3. Receive mouse / keyboard events through libjingle.
// 4. Sends screen capture through libjingle.

#include <iostream>
#include <string>
#include <stdlib.h>

#include "build/build_config.h"

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/file_path.h"
#include "base/logging.h"
#include "base/scoped_nsautorelease_pool.h"
#include "base/thread.h"
#include "base/waitable_event.h"
#include "remoting/host/capturer_fake.h"
#include "remoting/host/chromoting_host.h"
#include "remoting/host/encoder_verbatim.h"
#include "remoting/host/json_host_config.h"

#if defined(OS_WIN)
#include "remoting/host/capturer_gdi.h"
#include "remoting/host/event_executor_win.h"
#elif defined(OS_LINUX)
#include "remoting/host/capturer_linux.h"
#include "remoting/host/event_executor_linux.h"
#elif defined(OS_MACOSX)
#include "remoting/host/capturer_mac.h"
#include "remoting/host/event_executor_mac.h"
#endif

#if defined(OS_WIN)
const std::wstring kDefaultConfigPath = L".ChromotingConfig.json";
const wchar_t kHomePath[] = L"HOMEPATH";
const wchar_t* GetEnvironmentVar(const wchar_t* x) { return _wgetenv(x); }
#else
static char* GetEnvironmentVar(const char* x) { return getenv(x); }
const char kHomePath[] = "HOME";
const std::string kDefaultConfigPath = ".ChromotingConfig.json";
#endif

const std::string kFakeSwitchName = "fake";
const std::string kConfigSwitchName = "config";

int main(int argc, char** argv) {
  // Needed for the Mac, so we don't leak objects when threads are created.
  base::ScopedNSAutoreleasePool pool;

  CommandLine::Init(argc, argv);
  const CommandLine* cmd_line = CommandLine::ForCurrentProcess();

  base::AtExitManager exit_manager;

  scoped_ptr<remoting::Capturer> capturer;
  scoped_ptr<remoting::Encoder> encoder;
  scoped_ptr<remoting::EventExecutor> executor;
#if defined(OS_WIN)
  capturer.reset(new remoting::CapturerGdi());
  executor.reset(new remoting::EventExecutorWin());
#elif defined(OS_LINUX)
  capturer.reset(new remoting::CapturerLinux());
  executor.reset(new remoting::EventExecutorLinux());
#elif defined(OS_MACOSX)
  capturer.reset(new remoting::CapturerMac());
  executor.reset(new remoting::EventExecutorMac());
#endif
  encoder.reset(new remoting::EncoderVerbatim());

  // Check the argument to see if we should use a fake capturer and encoder.
  bool fake = cmd_line->HasSwitch(kFakeSwitchName);

  FilePath config_path(GetEnvironmentVar(kHomePath));
  config_path = config_path.Append(kDefaultConfigPath);
  if (cmd_line->HasSwitch(kConfigSwitchName)) {
    config_path = cmd_line->GetSwitchValuePath(kConfigSwitchName);
  }

  if (fake) {
    // Inject a fake capturer.
    capturer.reset(new remoting::CapturerFake());
  }

  base::Thread file_io_thread("FileIO");
  file_io_thread.Start();

  scoped_refptr<remoting::JsonHostConfig> config(
      new remoting::JsonHostConfig(
          config_path, file_io_thread.message_loop_proxy()));

  if (!config->Read()) {
    LOG(ERROR) << "Failed to read configuration file " << config_path.value();
    return 1;
  }

  base::WaitableEvent host_done(false, false);
  scoped_refptr<remoting::ChromotingHost> host =
      new remoting::ChromotingHost(config,
                                   capturer.release(),
                                   encoder.release(),
                                   executor.release(),
                                   &host_done);
  host->Run();
  host_done.Wait();

  file_io_thread.Stop();
  return 0;
}
