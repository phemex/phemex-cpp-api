#pragma once

#include <chrono>

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/system_timer.hpp>

#include "common/chrono/expiry_policy.hpp"

namespace phemex::common::chrono
{
namespace impl
{
template <class Timer>
struct GetTimeType
{
  using type = typename Timer::time_point;
};

template <>
struct GetTimeType<boost::asio::deadline_timer>
{
  using type = typename boost::asio::deadline_timer::time_type;
};
} // namespace impl

template <class Timer>
using GetTimeType = typename impl::GetTimeType<Timer>::type;

template <
    class IntervalType                         = std::chrono::hours,
    class IOExecutor                           = boost::asio::io_context,
    class Timer                                = boost::asio::system_timer,
    class TimePoint                            = GetTimeType<Timer>,
    template <class, class> class ExpiryPolicy = ExpireAtPoint>
class BasicTimer : public ExpiryPolicy<IntervalType, TimePoint>
{
 public:
  using ExpiryMgr = ExpiryPolicy<IntervalType, TimePoint>;

  template <class TP, class Interval>
  BasicTimer(
      IOExecutor& ioe, TP&& expiry_time, Interval&& interval, bool recursive)
    : ExpiryMgr{std::forward<TP>(expiry_time),
                std::forward<Interval>(interval)},
      ioe_{ioe},
      timer_{GetIOContext(ioe)},
      recursive_{recursive}
  {
  }

  template <class TP>
  BasicTimer(IOExecutor& ioe, TP&& expiry_time)
    : BasicTimer{ioe, std::forward<TP>(expiry_time), 1, false}
  {
  }

  template <class TP, class Interval>
  BasicTimer(IOExecutor& ioe, TP&& expiry_time, Interval&& interval)
    : BasicTimer{ioe, std::forward<TP>(expiry_time),
                 std::forward<Interval>(interval), false}
  {
  }

  template <class TP>
  BasicTimer(IOExecutor& ioe, TP&& expiry_time, bool recursive)
    : BasicTimer{ioe, std::forward<TP>(expiry_time), 1, recursive}
  {
  }

  template <class TP, class Rep, class Period>
  BasicTimer(
      IOExecutor& ioe, TP&& expiry_time,
      const std::chrono::duration<Rep, Period>& interval)
    : BasicTimer{ioe, std::forward<TP>(expiry_time),
                 std::chrono::duration_cast<IntervalType>(interval), false}
  {
  }

  template <class TP, class Rep, class Period>
  BasicTimer(
      IOExecutor& ioe, TP&& expiry_time,
      const std::chrono::duration<Rep, Period>& interval, bool recursive)
    : ExpiryMgr{std::forward<TP>(expiry_time),
                std::chrono::duration_cast<IntervalType>(interval)},
      ioe_{ioe},
      timer_{GetIOContext(ioe)},
      recursive_{recursive}
  {
  }

  template <class F>
  inline void Start(F&& handler)
  {
    PutTimer(std::forward<F>(handler), ioe_);
  }

  inline void Recursive(bool type)
  {
    recursive_ = type;
  }

  inline void Stop()
  {
    timer_.cancel();
  }

  ~BasicTimer()
  {
    timer_.cancel();
  }

 protected:
  static inline boost::asio::io_context& GetIOContext(
      boost::asio::io_context& ioc)
  {
    return ioc;
  }

  static inline boost::asio::io_context& GetIOContext(
      boost::asio::io_context::strand& strand)
  {
    return strand.context();
  }

  template <class F>
  inline void OnTimeout(const boost::system::error_code& ec, F&& handler)
  {
    if (ec != boost::asio::error::operation_aborted)
    {
      handler();
      if (recursive_)
      {
        PutTimer(std::forward<F>(handler), ioe_);
      }
    }
  }

  template <class F>
  inline void PutTimer(F&& handler, const boost::asio::io_context&)
  {
    ExpiryMgr::SetExpireTime(timer_);
    timer_.async_wait([this, handler = std::forward<F>(handler)](
                          const boost::system::error_code& ec) {
      OnTimeout(ec, std::move(handler));
    });
  }

  template <class F>
  inline void PutTimer(F&& handler, const boost::asio::io_context::strand&)
  {
    ExpiryMgr::SetExpireTime(timer_);
    timer_.async_wait(ioe_.wrap([this, handler = std::forward<F>(handler)](
                                    const boost::system::error_code& ec) {
      OnTimeout(ec, std::move(handler));
    }));
  }

 private:
  IOExecutor& ioe_;
  Timer timer_;
  bool recursive_;
};

} // namespace phemex::common::chrono
