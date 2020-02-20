#pragma once

#include <chrono>

#include <boost/date_time/posix_time/posix_time.hpp>

namespace phemex::common::chrono
{
class Time
{
 public:
  template <class... T>
  static inline auto Now()
  {
    return NowImpl(Type<T...>{});
  }

 private:
  template <class... T>
  struct Type
  {
  };

  template <class... T>
  static inline auto NowImpl(const Type<std::chrono::duration<T...>>&)
  {
    return std::chrono::duration_cast<std::chrono::duration<T...>>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

  template <class... T>
  static inline auto NowImpl(const Type<T...>&)
  {
    return NowImpl(Type<std::chrono::duration<T...>>{});
  }
};
} // namespace phemex::common::chrono

