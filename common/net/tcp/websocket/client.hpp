#pragma once

#include <atomic>
#include <memory>
#include <queue>

#include <boost/asio/buffer.hpp>
#include <boost/beast/core.hpp>

#include "common/config/websocket_client.hpp"
#include "common/log.hpp"
#include "common/net/address_book.hpp"
#include "common/net/ssl_context.hpp"
#include "common/net/tcp/websocket/ssl_connection.hpp"
#include "common/net/utils.hpp"

namespace phemex::common::net::tcp::websocket
{
template <class Parser, class Connection = SSLConnection>
class Client : public SSLContext, public AddressBook
{
 public:
  using Self          = Client<Parser, Connection>;
  using ConnectionPtr = typename Connection::Ptr;
  using context       = boost::asio::ssl::context;
  using stream_base   = boost::asio::ssl::stream_base;

  Client(
      boost::asio::io_context& ioc, const config::WebsocketClient& conf,
      bool start = true)
    : SSLContext{context::sslv23_client},
      AddressBook{conf.addr},
      connection_{std::make_shared<Connection>(
          boost::asio::ip::tcp::socket{ioc}, AddressBook::Url(),
          SSLContext::Context(), AddressBook::UseSSL())},
      parser_{static_cast<Parser*>(this)},
      strand_{ioc},
      conf_{conf},
      writing_{false},
      closed_{false},
      reconnect_interval_{conf.reconnect_interval}
  {
    static_assert(
        std::is_base_of<Client, Parser>::value,
        "class Client should be derived by an message parser class");
    if (start)
    {
      Start();
    }
  }

  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;

  void Start()
  {
    boost::asio::spawn(strand_, [this](boost::asio::yield_context yield) {
      HandleRead(yield);
    });
    boost::asio::spawn(
        strand_, [this](boost::asio::yield_context yield) { Monitor(yield); });
  }

  inline auto UseSSL() const
  {
    return AddressBook::UseSSL();
  }

  inline const auto& RemoteHost() const
  {
    return AddressBook::Host();
  }

  inline auto RemotePort() const
  {
    return AddressBook::Port();
  }

  inline const auto& RemotePath() const
  {
    return AddressBook::Path();
  }

  inline const auto& RemoteUrl() const
  {
    return AddressBook::Url();
  }

  inline void UseNextAddress()
  {
    AddressBook::UseNext();
  }

  inline bool IsAvailable() const
  {
    if (!connection_->IsOpen())
    {
      return false;
    }
    return true;
  }

  inline void Stop()
  {
    BOOST_LOG(client_lg) << "stop websocket client connection to "
                         << RemoteUrl();
    closed_ = true;
  }

  void Write(std::string message)
  {
    if (closed_)
    {
      return;
    }

    // Write data
    boost::asio::post(strand_, [this, message = std::move(message)]() {
      writing_queue_.push(std::move(message));
      if (writing_)
      {
        return;
      }
      boost::asio::spawn(
          strand_, [this](boost::asio::yield_context yield) mutable {
            HandleWrite(yield);
          });
    });
  }

  inline auto LastUpdate() const
  {
    const auto connection = connection_;
    return connection->LastUpdate();
  }

  virtual ~Client()
  {
  }

 protected:
  void Close(boost::asio::yield_context yield)
  {
    BOOST_LOG(client_lg) << "close server connection, remote endpoint: "
                         << RemoteUrl();

    // call parser on-close callback
    parser_->OnClose();

    auto connection = connection_;
    while (connection->IsOpen())
    {
      boost::system::error_code ec;
      connection->Close(yield, ec);
      // close again if has error
      if (ec)
      {
        AsyncWait(strand_, yield);
      }
    }

    // reset socket & context
    connection_ = std::make_shared<Connection>(
        boost::asio::ip::tcp::socket{strand_.context()}, AddressBook::Url(),
        SSLContext::Context(), AddressBook::UseSSL());
  }

  void HandleWrite(boost::asio::yield_context yield)
  {
    if (writing_)
    {
      return;
    }
    writing_ = true;

    boost::system::error_code ec;

    while (!closed_)
    {
      auto connection = connection_;
      if (!connection->IsOpen() || writing_queue_.empty())
      {
        break;
      }

      auto message = writing_queue_.front();
      writing_queue_.pop();
      if (message.empty())
      {
        break;
      }

      connection->Write(boost::asio::buffer(message), yield[ec]);
      if (ec)
      {
        Fail(ec, "write");
        Close(yield);
        break;
      }
    }
    writing_ = false;
  }

  void Connect(boost::asio::yield_context yield)
  {
    using boost::asio::ip::tcp;
    boost::system::error_code ec;

    while (!closed_)
    {
      auto connection = connection_;
      if (connection->IsOpen())
      {
        return;
      }

      // assign next remote end point if available
      UseNextAddress();

      BOOST_LOG(client_lg) << "start to connect " << RemoteUrl();
      if (!SetSNIHostname(connection, yield, ec))
      {
        Fail(ec, "set_sni");
        Close(yield);
        AsyncWait(strand_, yield, reconnect_interval_);
        continue;
      }

      tcp::resolver resolver(strand_.context());

      tcp::resolver::query query(
          tcp::v4(), RemoteHost(), std::to_string(RemotePort()));

      tcp::resolver::iterator endpoint_iterator =
          resolver.async_resolve(query, yield[ec]);
      if (ec)
      {
        Fail(ec, "resolve");
        AsyncWait(strand_, yield, reconnect_interval_);
        continue;
      }

      connection->Connect(endpoint_iterator, yield[ec]);
      if (ec)
      {
        Fail(ec, "connect");
        AsyncWait(strand_, yield, reconnect_interval_);
        continue;
      }

      connection->SSLHandShake(
          boost::asio::ssl::stream_base::client, yield, ec);
      if (ec)
      {
        Fail(ec, "ssl handshake");
        AsyncWait(strand_, yield, reconnect_interval_);
        continue;
      }

      connection->HandShake(strand_, RemoteHost(), RemotePath(), yield, ec);
      if (ec)
      {
        Fail(ec, "handshake");
        AsyncWait(strand_, yield, reconnect_interval_);
        continue;
      }

      BOOST_LOG(client_lg) << "connected to " << RemoteUrl();
      // callback on connection established
      parser_->OnConnected();
      return;
    }
  }

  void HandleRead(boost::asio::yield_context yield)
  {
    boost::system::error_code ec;
    // Init connection
    Connect(yield);

    // Loop to read data from server
    while (!closed_)
    {
      auto connection = connection_;
      if (!connection->IsOpen())
      {
        AsyncWait(strand_, yield);
        Connect(yield);
        continue;
      }

      std::string data{};
      connection->Read(data, yield, ec);
      if (ec)
      {
        if (!connection->GracefullyClosed(ec))
        {
          Fail(ec, "read data from");
        }
        Close(yield);
        continue;
      }

      if (!connection->IsMessageDone())
      {
        BOOST_LOG_SEV(client_lg, warning)
            << "received oversized data from " << RemoteUrl();
        Close(yield);
        continue;
      }

      if (!data.empty())
      {
        connection->SetLastUpdate();
        parser_->Parse(RemoteUrl(), std::move(data));
      }
    }
  }

  void Monitor(boost::asio::yield_context yield)
  {
    if (0 == conf_.timeout)
    {
      return;
    }

    while (!closed_)
    {
      AsyncWait(strand_, yield, std::chrono::seconds{1});

      // Close session if no data update before session timed out
      auto connection = connection_;
      auto now        = std::chrono::system_clock::now();
      auto time_diff  = std::chrono::duration_cast<std::chrono::seconds>(
                           now - connection->LastUpdate())
                           .count();

      connection->Monitor(yield);

      if (connection->IsOpen() && time_diff > conf_.timeout)
      {
        BOOST_LOG_SEV(client_lg, error)
            << "connection timeout after " << conf_.timeout
            << "s, remote endpoint: " << RemoteUrl();
        Close(yield);
      }
    }
  }

  // Report a failure
  inline void Fail(const boost::system::error_code& ec, std::string_view what)
  {
    BOOST_LOG_SEV(client_lg, error)
        << "failed to " << what << " " << RemoteUrl()
        << ", error msg=" << ec.message();
  }

 protected:
  inline bool SetSNIHostname(
      const ConnectionPtr& connection, boost::asio::yield_context yield,
      boost::system::error_code& ec) noexcept
  {
    if (conf_.enable_sni)
    {
      connection->SetSNIHostname(RemoteHost(), ec);
      if (ec)
      {
        return false;
      }
    }

    return true;
  }

 private:
  ConnectionPtr connection_;
  Parser* parser_;
  boost::asio::io_context::strand strand_;
  config::WebsocketClient conf_;
  std::queue<std::string> writing_queue_;
  bool writing_;
  bool closed_;
  std::chrono::duration<double> reconnect_interval_;
};
} // namespace phemex::common::net::tcp::websocket
