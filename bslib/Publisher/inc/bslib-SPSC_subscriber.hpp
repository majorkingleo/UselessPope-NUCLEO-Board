#pragma once
#ifndef BSLIB_SPSC_SUBSCRIBER_HPP
#define BSLIB_SPSC_SUBSCRIBER_HPP

#include <bslib-Container.hpp>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <os.hpp>
#include <wlib.hpp>

namespace wlib::publisher
{
  template <typename T, std::size_t N>
    requires(N > 0)
  class SPSC_Subscriber final: public Publisher_Interface<T>::Subscription_Interface
  {
  public:
    using payload_t = typename wlib::publisher::Publisher_Interface<T>::payload_t;

    SPSC_Subscriber()  = default;
    ~SPSC_Subscriber() = default;

    payload_t get_value()
    {
      this->m_sem.acquire();
      return this->m_buffer.pop_front().value();
    }

    std::optional<payload_t> try_get_value()
    {
      if (!this->m_sem.try_acquire())
        return std::nullopt;
      return this->m_buffer.pop_front();
    }

    template <class Rep, class Period> std::optional<payload_t> try_get_value_for(const std::chrono::duration<Rep, Period>& rel_time)
    {
      if (!this->m_sem.try_acquire_for(rel_time))
        return std::nullopt;
      return this->m_buffer.pop_front();
    }

    template <class Clock, class Duration> std::optional<payload_t> try_get_value_until(const std::chrono::time_point<Clock, Duration>& abs_time)
    {
      if (!this->m_sem.try_acquire_until(abs_time))
        return std::nullopt;
      return this->m_buffer.pop_front();
    }

  private:
    void notify(payload_t const& val) noexcept override
    {
      if (this->m_buffer.push_back(val))
        this->m_sem.release();
    }

  private:
    os::counting_semaphore<N>            m_sem{ 0 };
    bslib::container::SPSC<payload_t, N> m_buffer;
  };
}    // namespace wlib::publisher

#endif
