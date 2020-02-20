#pragma once

#include <exception>

#include "common/config/host_address.hpp"

namespace phemex::common::config
{
struct WebsocketClient
{
  HostAddress addr{"wss://phemex.com/ws"};
  bool ssl                    = true;
  int32_t timeout             = 30;
  double reconnect_interval   = 1;
  int32_t response_size_limit = 0;
  bool enable_sni             = true;
};

} // namespace phemex::common::config
