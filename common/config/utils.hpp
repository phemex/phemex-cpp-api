#pragma once

#include <sstream>
#include <string>

namespace phemex::common::config
{
inline auto& PutHeader(
    std::ostringstream& oss, int32_t indent, const std::string& key)
{
  oss << std::string(indent, ' ') << key << ":\n";
  return oss;
}

template <class... Args>
inline auto& Put(std::ostringstream& oss, const Args&... args)
{
  (oss << ... << args);
  return oss;
}

template <class... Args>
inline auto& PutLine(
    std::ostringstream& oss, int32_t indent, const std::string& key,
    const Args&... values)
{
  oss << std::string(indent, ' ') << " - " << key << ": ";
  (oss << ... << values);
  oss << "\n";
  return oss;
}
} // namespace phemex::common::config
