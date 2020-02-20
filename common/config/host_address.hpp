#pragma once

#include <regex>
#include <string>

#include <boost/lexical_cast.hpp>

#include "common/config/utils.hpp"

namespace phemex::common::config
{
struct HostAddress
{
  using Ptr = std::shared_ptr<HostAddress>;

  HostAddress(const std::string& addr)
  {
    Parse(addr);
  }

  HostAddress(const std::string& host, unsigned short port)
    : host{host}, port{port}, url{host + ":" + std::to_string(port)}
  {
  }

  HostAddress(
      const std::string& host, unsigned short port, const std::string& path)
    : host{host},
      port{port},
      url{host + ":" + std::to_string(port) + "/" + path}
  {
  }

  HostAddress(
      const std::string& protocol, const std::string& host, unsigned short port,
      const std::string& path)
    : protocol{protocol},
      host{host},
      port{port},
      url{protocol + "://" + host + ":" + std::to_string(port) + "/" + path}
  {
  }

  static inline auto GetPort(
      const std::string& protocol, const std::string& url = std::string{})
  {
    if ("https" == protocol || "wss" == protocol)
    {
      return 443;
    }
    else if ("http" == protocol || "ws" == protocol)
    {
      return 80;
    }
    else if ("tcps" == protocol || "tcp" == protocol || "ssl" == protocol)
    {
      return 0;
    }
    else
    {
      throw std::invalid_argument{"port should be set, url: " + url};
    }
  }

  void Parse(const std::string& addr)
  {
    std::regex url_pattern1{"^([a-z]+)://([^/:]+):([0-9]+)(/.*)?$"};
    std::regex url_pattern2{"^([^/:]+):([0-9]+)(/.*)?$"};
    std::regex url_pattern3{"^([a-z]+)://([^/:]+)(/.*)?$"};
    std::smatch sm;

    if (std::regex_match(addr, sm, url_pattern1))
    {
      protocol = sm[1];
      host     = sm[2];
      port     = boost::lexical_cast<unsigned short>(sm[3]);
      path     = "/";
      url      = addr;

      if (sm.size() > 4 && !sm[4].str().empty())
      {
        path = sm[4];
      }
    }
    else if (std::regex_match(addr, sm, url_pattern2))
    {
      protocol = "tcp";
      host     = sm[1];
      port     = boost::lexical_cast<unsigned short>(sm[2]);
      path     = "/";
      url      = addr;

      if (sm.size() > 3 && !sm[3].str().empty())
      {
        path = sm[3];
      }
    }
    else if (std::regex_match(addr, sm, url_pattern3))
    {
      protocol = sm[1];
      host     = sm[2];
      path     = "/";
      url      = addr;
      port     = GetPort(protocol, url);

      if (sm.size() > 3 && !sm[3].str().empty())
      {
        path = sm[3];
      }
    }
    else
    {
      throw std::invalid_argument{"invalid socket url: " + addr};
    }
  }

  inline void Path(const std::string& p)
  {
    path = p;
    url  = protocol + "://" + host + ":" + std::to_string(port) + path;
  }

  inline auto ToString(int32_t indent_chars = 0) const
  {
    std::ostringstream oss;
    PutHeader(oss, indent_chars, "address");
    PutLine(oss, indent_chars, "protocol", protocol);
    PutLine(oss, indent_chars, "host", host);
    PutLine(oss, indent_chars, "port", port);
    PutLine(oss, indent_chars, "path", path);
    return oss.str();
  }

  std::string protocol;
  std::string host;
  unsigned short port = 80;
  std::string path;
  std::string url;
};

} // namespace phemex::common::config
