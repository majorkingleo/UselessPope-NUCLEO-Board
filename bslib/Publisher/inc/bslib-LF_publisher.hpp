#pragma once
#ifndef BSLIB_LOCKFREE_PUBLISHER_HPP_INCLUDED
#define BSLIB_LOCKFREE_PUBLISHER_HPP_INCLUDED

#include <atomic>
#include <concepts>
#include <cstddef>
#include <os.hpp>
#include <wlib.hpp>

namespace bslib::publisher
{
  using namespace wlib::publisher;

  template <typename T, std::size_t N>
    requires(N > 0)
  class LF_Publisher
      : public Publisher_Interface<T>
      , public Publisher_Interface<T>::Notifyable_Interface
  {
    using Notifyable_Interface = typename Publisher_Interface<T>::Notifyable_Interface;

  public:
    using payload_t = typename Publisher_Interface<T>::payload_t;

    static constexpr std::size_t max_number_of_subscribers = N;

  public:
    void notify(payload_t const& value) noexcept
    {
      this->m_notify_ongoing = true;
      for (auto& cur_entry : m_subscriber_list)
      {
        Notifyable_Interface* subscriber = cur_entry.load(std::memory_order_acquire);
        if (subscriber != nullptr)
        {
          subscriber->notify(value);
        }
      }
      this->m_notify_ongoing = false;
    }

  private:
    bool try_add_subscriber(Notifyable_Interface& sub) override
    {
      for (auto& cur_entry : m_subscriber_list)
      {
        Notifyable_Interface* expected = nullptr;
        if (cur_entry.compare_exchange_strong(expected, &sub))
        {
          return true;
        }
      }
      return false;
    }

    void remove_subscriber(Notifyable_Interface& sub) override
    {
      for (auto& cur_entry : m_subscriber_list)
      {
        Notifyable_Interface* expected = &sub;
        cur_entry.compare_exchange_strong(expected, nullptr);
      }
      while (this->m_notify_ongoing)
      {
        os::this_thread::yield();
      }
    }

  private:
    std::atomic<Notifyable_Interface*> m_subscriber_list[max_number_of_subscribers] = {};
    std::atomic<bool>                  m_notify_ongoing                             = false;
  };

  template <std::size_t N>
    requires(N > 0)
  class LF_Publisher<void, N>
      : public Publisher_Interface<void>
      , public Publisher_Interface<void>::Notifyable_Interface
  {
    using Notifyable_Interface = typename Publisher_Interface<void>::Notifyable_Interface;

  public:
    using payload_t = typename Publisher_Interface<void>::payload_t;

    static constexpr std::size_t max_number_of_subscribers = N;

  public:
    void notify() noexcept
    {
      this->m_notify_ongoing = true;
      for (auto& cur_entry : m_subscriber_list)
      {
        Notifyable_Interface* subscriber = cur_entry.load(std::memory_order_acquire);
        if (subscriber != nullptr)
        {
          subscriber->notify();
        }
      }
      this->m_notify_ongoing = false;
    }

  private:
    bool try_add_subscriber(Notifyable_Interface& sub) override
    {
      for (auto& cur_entry : m_subscriber_list)
      {
        Notifyable_Interface* expected = nullptr;
        if (cur_entry.compare_exchange_strong(expected, &sub))
        {
          return true;
        }
      }
      return false;
    }

    void remove_subscriber(Notifyable_Interface& sub) override
    {
      for (auto& cur_entry : m_subscriber_list)
      {
        Notifyable_Interface* expected = &sub;
        cur_entry.compare_exchange_strong(expected, nullptr);
      }
      while (this->m_notify_ongoing)
      {
        os::this_thread::yield();
      }
    }

  private:
    std::atomic<Notifyable_Interface*> m_subscriber_list[max_number_of_subscribers] = {};
    std::atomic<bool>                  m_notify_ongoing                             = false;
  };

}    // namespace bslib::publisher

#endif
