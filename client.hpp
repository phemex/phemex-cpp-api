#pragma once

#include <nlohmann/json.hpp>

#include "common/config/websocket_client.hpp"
#include "common/net/tcp/websocket/client.hpp"
#include "common/timer.hpp"

namespace phemex
{
class Client : public common::net::tcp::websocket::Client<Client>
{
  using WebsocketClient = common::net::tcp::websocket::Client<Client>;

 public:
  Client(
      boost::asio::io_context& ioc,
      std::function<void(std::string)> ws_msg_callback)
    : WebsocketClient{ioc, common::config::WebsocketClient{}},
      heartbeat_timer_{ioc, 5},
      ws_msg_callback_{ws_msg_callback}
  {
    heartbeat_timer_.Start([this]() { SendHearbeat(); });
  }

  inline void SendHearbeat()
  {
    if (WebsocketClient::IsAvailable())
    {
      WebsocketClient::Write(nlohmann::json{
          {"method", "server.ping"},
          {"params", {}},
          {"id", 0}}.dump());
    }
  }

  inline void SubscribeOrderBook(const std::string& symbol)
  {
    BOOST_LOG(client_lg) << "subscribe order book, symbol: " << symbol;
    subs_.push_back(nlohmann::json{
        {"method", "orderbook.subscribe"},
        {"params", {symbol}},
        {"id", 1}}.dump());
  }

  inline void SubscribeKline(const std::string& symbol, int32_t interval)
  {
    BOOST_LOG(client_lg) << "subscribe kline, symbol: " << symbol
                         << ", interval: " << interval;
    subs_.push_back(nlohmann::json{
        {"method", "kline.subscribe"},
        {"params", {symbol, interval}},
        {"id", 2}}.dump());
  }

  inline void SubscribeTrade(const std::string& symbol)
  {
    BOOST_LOG(client_lg) << "subscribe trade, symbol: " << symbol;
    subs_.push_back(nlohmann::json{
        {"method", "trade.subscribe"},
        {"params", {symbol}},
        {"id", 3}}.dump());
  }

  inline void UnsubscribeOrderBook()
  {
    BOOST_LOG(client_lg) << "unsubscribe all order book";
    subs_.push_back(nlohmann::json{
        {"method", "orderbook.unsubscribe"},
        {"params", {}},
        {"id", 4}}.dump());
  }

  inline void UnsubscribeKline()
  {
    BOOST_LOG(client_lg) << "unsubscribe all kline";
    subs_.push_back(nlohmann::json{
        {"method", "kline.unsubscribe"},
        {"params", {}},
        {"id", 5}}.dump());
  }

  inline void UnsubscribeTrade()
  {
    BOOST_LOG(client_lg) << "unsubscribe all trade";
    subs_.push_back(nlohmann::json{
        {"method", "trade.unsubscribe"},
        {"params", {}},
        {"id", 6}}.dump());
  }

  inline void Parse(std::string_view remote_url, std::string&& message)
  {
    BOOST_LOG(client_lg) << "received message from " << remote_url
                         << ", message: " << message;
    ws_msg_callback_(message);
  }

  inline void OnConnected()
  {
    BOOST_LOG(client_lg) << "websocket connected, server: "
                         << WebsocketClient::RemoteUrl();
    for (const auto& sub : subs_)
    {
      BOOST_LOG_SEV(client_lg, debug)
          << "send subscription message, message: " << sub;
      WebsocketClient::Write(sub);
    }
  }

  inline void OnClose()
  {
    BOOST_LOG(client_lg) << "websocket closed, server: "
                         << WebsocketClient::RemoteUrl();
  }

 private:
  common::DurationTimer<std::chrono::seconds> heartbeat_timer_;
  std::function<void(std::string)> ws_msg_callback_;
  std::vector<std::string> subs_;
};
} // namespace phemex
