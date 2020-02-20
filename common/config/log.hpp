#pragma once

#include <exception>

namespace phemex::common::config
{
struct Log
{
  std::string dir{"logs"};
  std::string log_level{"debug"};
  uint64_t max_size       = (uint64_t)128 * 1024 * 1024 * 1024;
  uint64_t rotation_size  = (uint64_t)4 * 1024 * 1024 * 1024;
  uint64_t min_free_space = (uint64_t)100 * 1024 * 1024;
  uint64_t max_log_files  = (uint64_t)1024;
  bool flush_flag         = true;
};

} // namespace phemex::common::config
