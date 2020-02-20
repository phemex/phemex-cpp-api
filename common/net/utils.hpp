#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>

namespace phemex::common::net
{
template <class DurationType = std::chrono::milliseconds>
inline void AsyncWait(
    boost::asio::io_context& ioc, boost::asio::yield_context yield,
    const DurationType& duration = std::chrono::milliseconds{200})
{
  boost::system::error_code ec;
  boost::asio::steady_timer t(
      ioc, std::chrono::duration_cast<std::chrono::milliseconds>(duration));
  t.async_wait(yield[ec]);
}

template <class DurationType = std::chrono::milliseconds>
inline void AsyncWait(
    boost::asio::io_context::strand& strand, boost::asio::yield_context yield,
    const DurationType& duration = std::chrono::milliseconds{200})
{
  boost::system::error_code ec;
  boost::asio::steady_timer t(
      strand.context(),
      std::chrono::duration_cast<std::chrono::milliseconds>(duration));
  t.async_wait(yield[ec]);
}

} // namespace phemex::common::net
