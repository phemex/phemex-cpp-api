#pragma once

namespace phemex::common::chrono
{
template <class IntervalType, class TimePoint>
class ExpireAtPoint
{
 public:
  template <class TP, class T>
  ExpireAtPoint(TP&& expiry_time, T&& interval)
    : expiry_time_{std::forward<TP>(expiry_time)},
      interval_{std::forward<T>(interval)}
  {
  }

  template <class TP>
  inline void ExpireAt(TP&& expiry_time)
  {
    expiry_time_ = std::forward<TP>(expiry_time);
  }

  template <class T>
  inline void Interval(T&& interval)
  {
    interval_ = std::forward<T>(interval);
  }

 protected:
  template <class T>
  inline void SetExpireTime(T& timer)
  {
    while (true)
    {
      try
      {
        timer.expires_at(expiry_time_);
        expiry_time_ += interval_;
        break;
      }
      catch (const std::exception& e)
      {
      }
    }
  }

 private:
  TimePoint expiry_time_;
  IntervalType interval_;
};

template <class IntervalType, class TimePoint>
class ExpireAfter
{
 public:
  template <class TP, class T>
  ExpireAfter(const TP&, T&& interval) : interval_{std::forward<T>(interval)}
  {
  }

  template <class T>
  inline void Interval(T&& interval)
  {
    interval_ = std::forward<T>(interval);
  }

 protected:
  template <class T>
  inline void SetExpireTime(T& timer)
  {
    while (true)
    {
      try
      {
        timer.expires_after(
            std::chrono::duration_cast<std::chrono::microseconds>(interval_));
        break;
      }
      catch (const std::exception& e)
      {
      }
    }
  }

 private:
  IntervalType interval_;
};

} // namespace phemex::common::chrono

