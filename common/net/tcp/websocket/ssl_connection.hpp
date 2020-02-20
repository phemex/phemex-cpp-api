#pragma once

#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/websocket/ssl.hpp>

#include "common/net/tcp/websocket/connection.hpp"

namespace phemex::common::net::tcp::websocket
{
class SSLConnection
  : public Connection<boost::beast::websocket::stream<
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>>
{
 public:
  using Ptr    = std::shared_ptr<SSLConnection>;
  using Buffer = Connection::Buffer;

  template <class Context, class... Args>
  SSLConnection(
      boost::asio::ip::tcp::socket&& socket, const std::string& url,
      Context& context, const Args&... args)
    : Connection{std::move(socket), url, context}
  {
  }

  template <class Context, class... Args>
  SSLConnection(
      boost::asio::ip::tcp::socket&& socket, Context& context,
      const Args&... args)
    : Connection{std::move(socket), context}
  {
  }

  template <class... Args>
  inline void Connect(Args&&... args)
  {
    boost::asio::async_connect(
        Connection::Socket().next_layer().next_layer(),
        std::forward<Args>(args)...);
  }

  inline void SetSNIHostname(
      const std::string& hostname, boost::system::error_code& ec) noexcept
  {
    if (!SSL_set_tlsext_host_name(
            Connection::Socket().next_layer().native_handle(),
            hostname.c_str()))
    {
      ec = boost::system::error_code{static_cast<int>(::ERR_get_error()),
                                     boost::asio::error::get_ssl_category()};
    }
  }

  template <class HandShakeType, class... Args>
  inline void SSLHandShake(HandShakeType type, Args&&... args)
  {
    Connection::Socket().next_layer().async_handshake(
        type, std::forward<Args>(args)...);
  }

  template <class HandShakeType, class... Args>
  inline void SSLHandShake(
      HandShakeType type, boost::asio::yield_context yield,
      boost::system::error_code& ec)
  {
    Connection::Socket().next_layer().async_handshake(type, yield[ec]);
  }
};
} // namespace phemex::common::net::tcp::websocket
