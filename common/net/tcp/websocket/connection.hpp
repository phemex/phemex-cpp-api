#pragma once

#include <chrono>
#include <memory>

#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

namespace phemex::common::net::tcp::websocket
{
template <class S>
class Connection
{
 public:
  using Ptr    = std::shared_ptr<Connection<S>>;
  using Buffer = boost::beast::multi_buffer;

  template <class... Args>
  Connection(
      boost::asio::ip::tcp::socket&& socket, const std::string& url,
      Args&&... args)
    : remote_url_{url},
      ws_{std::move(socket), std::forward<Args>(args)...},
      last_update_{std::chrono::system_clock::now()}
  {
  }

  template <class... Args>
  Connection(boost::asio::ip::tcp::socket&& socket, Args&&... args)
    : remote_url_{GetRemoteUrl(socket)},
      ws_{std::move(socket), std::forward<Args>(args)...},
      last_update_{std::chrono::system_clock::now()}
  {
  }

  inline const auto& RemoteUrl() const
  {
    return remote_url_;
  }

  inline void RemoteUrl(const std::string& url)
  {
    remote_url_ = url;
  }

  inline bool IsOpen() const
  {
    return ws_.next_layer().lowest_layer().is_open();
  }

  inline bool IsMessageDone() const
  {
    return ws_.is_message_done();
  }

  template <class... Args>
  inline void Connect(Args&&... args)
  {
    boost::asio::async_connect(std::forward<Args>(args)...);
  }

  inline void Close(boost::system::error_code& ec)
  {
    ForceClose(ec);
  }

  inline void Close(
      boost::asio::yield_context yield, boost::system::error_code& ec)
  {
    ForceClose(ec);
  }

  static inline bool GracefullyClosed(const boost::system::error_code& ec)
  {
    if (boost::beast::websocket::error::closed != ec ||
        boost::asio::error::connection_reset != ec ||
        boost::asio::error::eof != ec)
    {
      return true;
    }
    return false;
  }

  template <class... Args>
  inline void Write(Args&&... args)
  {
    ws_.async_write(std::forward<Args>(args)...);
  }

  template <class T, class... Args>
  inline void Write(std::shared_ptr<T>& message, Args&&... args)
  {
    ws_.async_write(
        boost::asio::buffer(std::move(message->Data())),
        std::forward<Args>(args)...);
  }

  template <class... Args>
  inline void Read(Args&&... args)
  {
    last_update_ = std::chrono::system_clock::now();
    ws_.async_read(std::forward<Args>(args)...);
  }

  inline void Read(
      std::string& data, boost::asio::yield_context& yield,
      boost::system::error_code& ec)
  {
    Read(buffer_, yield[ec]);
    data = boost::beast::buffers_to_string(buffer_.data());
    buffer_.consume(buffer_.size());
  }

  inline void Read(std::string& data, boost::asio::yield_context& yield)
  {
    Read(buffer_, yield);
    data = boost::beast::buffers_to_string(buffer_.data());
    buffer_.consume(buffer_.size());
  }

  inline void SetSNIHostname(
      const std::string&, boost::system::error_code&) noexcept
  {
  }

  inline void HandShake(
      boost::asio::io_context::strand& strand, const std::string& host,
      const std::string& path, boost::asio::yield_context& yield,
      boost::system::error_code& ec)
  {
    last_update_ = std::chrono::system_clock::now();
    ws_.async_handshake(host, path, yield[ec]);
    if (!ec)
    {
      SetClientControlCallback(strand);
    }
  }

  template <class... Args>
  inline void SSLHandShake(Args&&... args)
  {
    last_update_ = std::chrono::system_clock::now();
  }

  // handshake at server side
  template <class F>
  void Accept(
      boost::asio::yield_context& yield, boost::system::error_code& ec,
      F&& callback)
  {
    AcceptImpl(yield, ec, std::forward<F>(callback));
  }

  // handshake at server side
  template <class T, class F>
  void Accept(
      boost::asio::yield_context& yield, boost::system::error_code& ec,
      F&& callback, const boost::beast::http::request<T>& request)
  {
    AcceptImpl(yield, ec, std::forward<F>(callback), request);
  }

  inline void Ping(
      boost::asio::yield_context& yield, boost::system::error_code& ec)
  {
    if (!ws_.is_open())
    {
      return;
    }

    ws_.async_ping("ping", yield[ec]);
    if (ec && ec != boost::asio::error::operation_aborted)
    {
      BOOST_LOG_SEV(client_lg, error)
          << "failed to send ping request, reason: " << ec.message()
          << ", remote url: " << remote_url_;
    }
  }

  inline void Monitor(boost::asio::yield_context& yield)
  {
    boost::system::error_code ec{};
    Ping(yield, ec);
  }

  inline auto& Socket()
  {
    return ws_;
  }

  inline auto& Socket() const
  {
    return ws_;
  }

  inline auto UseSSL() const
  {
    return false;
  }

  inline void UseSSL(bool)
  {
  }

  inline const auto& LastUpdate() const
  {
    return last_update_;
  }

  inline void SetLastUpdate(
      const std::chrono::system_clock::time_point& last_update =
          std::chrono::system_clock::now())
  {
    last_update_ = last_update;
  }

 protected:
  template <class T>
  static inline std::string GetRemoteUrl(const T& socket)
  {
    return socket.remote_endpoint().address().to_string() + ":" +
           std::to_string(socket.remote_endpoint().port());
  }

  inline void ForceClose(boost::system::error_code& ec)
  {
    if (!IsOpen())
    {
      return;
    }

    auto& lowest_layer = ws_.next_layer().lowest_layer();
    lowest_layer.cancel(ec);
    if (ec)
    {
      BOOST_LOG_SEV(client_lg, error)
          << "failed to cancel operations on tcp socket, remote url: "
          << remote_url_ << ", reason: " << ec.message();
    }

    lowest_layer.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    if (ec)
    {
      BOOST_LOG_SEV(client_lg, error)
          << "failed to force shutdown socket, remote url: " << remote_url_
          << ", reason: " << ec.message();
    }

    lowest_layer.close(ec);
    if (ec)
    {
      BOOST_LOG_SEV(client_lg, error)
          << "failed to force close tcp socket, remote url: " << remote_url_
          << ", reason: " << ec.message();
    }
  }

  template <class F, class... Args>
  void AcceptImpl(
      boost::asio::yield_context& yield, boost::system::error_code& ec,
      F&& callback, const Args&... args)
  {
    // Accept the websocket handshake
    ws_.async_accept(args..., yield[ec]);

    if (ec)
    {
      return;
    }

    last_update_ = std::chrono::system_clock::now();

    // set callback to respond to ping request
    SetSessionControlCallback(std::forward<F>(callback));
  }

  inline void SetClientControlCallback(boost::asio::io_context::strand& strand)
  {
    ws_.control_callback(
        strand.wrap([this](
                        boost::beast::websocket::frame_type kind,
                        boost::beast::string_view) {
          if (boost::beast::websocket::frame_type::pong == kind)
          {
            last_update_ = std::chrono::system_clock::now();
          }
        }));
  }

  template <class F>
  inline void SetSessionControlCallback(F&& callback)
  {
    ws_.control_callback(std::forward<F>(callback));
  }

 private:
  std::string remote_url_;
  S ws_;
  Buffer buffer_;
  std::chrono::system_clock::time_point last_update_;
};
} // namespace phemex::common::net::tcp::websocket
