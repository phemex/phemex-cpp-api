#pragma once

#include <boost/asio/steady_timer.hpp>

#include "common/chrono/basic_timer.hpp"

namespace phemex::common::chrono
{
template <
    class IntervalType = std::chrono::milliseconds,
    class IOExecutor   = boost::asio::io_context,
    class Timer        = boost::asio::steady_timer>
class DurationTimer
  : public BasicTimer<
        IntervalType, IOExecutor, Timer, GetTimeType<Timer>, ExpireAfter>
{
 public:
  using TimePoint = GetTimeType<Timer>;
  using Base =
      BasicTimer<IntervalType, IOExecutor, Timer, TimePoint, ExpireAfter>;

  template <class Interval>
  DurationTimer(IOExecutor& ioe, Interval&& interval, bool recursive)
    : Base{ioe, TimePoint{}, std::forward<Interval>(interval), recursive}
  {
  }

  template <class Interval>
  DurationTimer(IOExecutor& ioe, Interval&& interval)
    : DurationTimer{ioe, std::forward<Interval>(interval), true}
  {
  }
};

} // namespace phemex::common::chrono
