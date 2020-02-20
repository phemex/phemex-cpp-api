#include <iostream>

#include <nlohmann/json.hpp>

#include "client.hpp"
#include "common/config/log.hpp"
#include "common/log.hpp"

int main(int argc, char** argv)
{
  boost::asio::io_context ioc;

  try
  {
    using namespace phemex::common;

    // init log
    auto& logger = Log::Get(config::Log{}, argv[0]);
    auto client =
        std::make_shared<phemex::Client>(ioc, [](const auto& message) {
          const auto j = nlohmann::json::parse(message);
          // handle orderbook messages
          if (j.count("orderbook"))
          {
          }
          // handle kline messages
          else if (j.count("kline"))
          {
          }
          // handle trade messages
          else if (j.count("trade"))
          {
          }
          // handle reply messages
          else
          {
          }
        });
    client->SubscribeOrderBook("BTCUSD");
    client->SubscribeKline("BTCUSD", 60);
    client->SubscribeTrade("BTCUSD");

    ioc.run();
    logger.Flush();
  }
  catch (std::exception& e)
  {
    std::cerr << "Error: failed to start phemex api, reason: " << e.what()
              << std::endl;
    return 1;
  }

  return 0;
}
