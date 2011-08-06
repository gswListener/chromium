// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_PPP_VIDEO_DECODER_PROXY_H_
#define PPAPI_PROXY_PPP_VIDEO_DECODER_PROXY_H_

#include "ppapi/c/dev/ppp_video_decoder_dev.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/proxy/host_resource.h"
#include "ppapi/proxy/interface_proxy.h"

struct PP_Picture_Dev;
struct PP_Size;

namespace pp {
namespace proxy {

class PPP_VideoDecoder_Proxy : public InterfaceProxy {
 public:
  PPP_VideoDecoder_Proxy(Dispatcher* dispatcher, const void* target_interface);
  virtual ~PPP_VideoDecoder_Proxy();

  static const Info* GetInfo();

  const PPP_VideoDecoder_Dev* ppp_video_decoder_target() const {
    return static_cast<const PPP_VideoDecoder_Dev*>(target_interface());
  }

  // InterfaceProxy implementation.
  virtual bool OnMessageReceived(const IPC::Message& msg);

 private:
  // Message handlers.
  void OnMsgProvidePictureBuffers(const HostResource& decoder,
                                  uint32_t req_num_of_buffers,
                                  const PP_Size& dimensions);
  void OnMsgDismissPictureBuffer(const HostResource& decoder,
                                 int32_t picture_id);
  void OnMsgPictureReady(
      const HostResource& decoder, const PP_Picture_Dev& picture_buffer);
  void OnMsgNotifyEndOfStream(const HostResource& decoder);
  void OnMsgNotifyError(const HostResource& decoder,
                        PP_VideoDecodeError_Dev error);

  DISALLOW_COPY_AND_ASSIGN(PPP_VideoDecoder_Proxy);
};

}  // namespace proxy
}  // namespace pp

#endif  // PPAPI_PROXY_PPP_VIDEO_DECODER_PROXY_H_
