#pragma once
#ifndef WLIB_PUBLISHER_HPP_INCLUDED
#define WLIB_PUBLISHER_HPP_INCLUDED

#include <wlib-Callback.hpp>

namespace wlib::publisher
{
  namespace error
  {
    void handle_subscription_exception();
  }

  template <typename T> class Publisher_Interface
  {
  public:
    using payload_t = T;

    class Notifyable_Interface
    {
    public:
      virtual ~Notifyable_Interface()       = default;
      virtual void notify(payload_t const&) = 0;
    };

    class Subscription_Interface: private Publisher_Interface::Notifyable_Interface
    {
    public:
      Subscription_Interface()                                         = default;
      Subscription_Interface(Subscription_Interface const&)            = delete;
      Subscription_Interface(Subscription_Interface&&)                 = delete;
      Subscription_Interface& operator=(Subscription_Interface const&) = delete;
      Subscription_Interface& operator=(Subscription_Interface&&)      = delete;
      virtual ~Subscription_Interface() { this->unsubscribe(); }

      void subscribe(Publisher_Interface& pub) &
      {
        if (this->is_subscribed() || !pub.try_add_subscriber(*this))
          return error::handle_subscription_exception();
        this->m_pub = &pub;
      }

      void unsubscribe()
      {
        if (this->m_pub == nullptr)
          return;

        this->m_pub->remove_subscriber(*this);
        this->m_pub = nullptr;
      }

      bool is_subscribed() const noexcept { return this->m_pub != nullptr; }

    private:
      Publisher_Interface<T>* m_pub = nullptr;
    };

    Publisher_Interface()                                      = default;
    Publisher_Interface(Publisher_Interface const&)            = delete;
    Publisher_Interface(Publisher_Interface&&)                 = delete;
    Publisher_Interface& operator=(Publisher_Interface const&) = delete;
    Publisher_Interface& operator=(Publisher_Interface&&)      = delete;
    virtual ~Publisher_Interface()                             = default;

  private:
    virtual bool try_add_subscriber(Notifyable_Interface& sub) = 0;
    virtual void remove_subscriber(Notifyable_Interface& sub)  = 0;
  };

  template <> class Publisher_Interface<void>
  {
  public:
    using payload_t = void;

  protected:
    class Notifyable_Interface
    {
    public:
      virtual ~Notifyable_Interface() = default;
      virtual void notify()           = 0;
    };

  public:
    class Subscription_Interface: private Publisher_Interface<void>::Notifyable_Interface
    {
    public:
      Subscription_Interface()                                         = default;
      Subscription_Interface(Subscription_Interface const&)            = delete;
      Subscription_Interface(Subscription_Interface&&)                 = delete;
      Subscription_Interface& operator=(Subscription_Interface const&) = delete;
      Subscription_Interface& operator=(Subscription_Interface&&)      = delete;
      virtual ~Subscription_Interface() { this->unsubscribe(); }

      void subscribe(Publisher_Interface& pub) &
      {
        if (this->is_subscribed() || !pub.try_add_subscriber(*this))
          return error::handle_subscription_exception();
        this->m_pub = &pub;
      }

      void unsubscribe()
      {
        if (this->m_pub == nullptr)
          return;

        this->m_pub->remove_subscriber(*this);
        this->m_pub = nullptr;
      }

      bool is_subscribed() const noexcept { return this->m_pub != nullptr; }

    private:
      Publisher_Interface<void>* m_pub = nullptr;
    };

    Publisher_Interface()                                      = default;
    Publisher_Interface(Publisher_Interface const&)            = delete;
    Publisher_Interface(Publisher_Interface&&)                 = delete;
    Publisher_Interface& operator=(Publisher_Interface const&) = delete;
    Publisher_Interface& operator=(Publisher_Interface&&)      = delete;
    virtual ~Publisher_Interface()                             = default;

  private:
    virtual bool try_add_subscriber(Notifyable_Interface& sub) = 0;
    virtual void remove_subscriber(Notifyable_Interface& sub)  = 0;
  };

  template <typename Tpub> class CallbackSubscriber: public Publisher_Interface<Tpub>::Subscription_Interface
  {
  public:
    CallbackSubscriber(wlib::Callback<void(Tpub const&)>& target_callback)
        : m_target_callback(target_callback)
    {
    }

  private:
    void                               notify(Tpub const& val) override { return this->m_target_callback(val); }
    wlib::Callback<void(Tpub const&)>& m_target_callback;
  };

  template <> class CallbackSubscriber<void>: public Publisher_Interface<void>::Subscription_Interface
  {
  public:
    CallbackSubscriber(wlib::Callback<void()>& target_callback)
        : m_target_callback(target_callback)
    {
    }

  private:
    void                    notify() override { return this->m_target_callback(); }
    wlib::Callback<void()>& m_target_callback;
  };

  template <typename T, typename Tpub> class Memberfunction_CallbackSubscriber: public Publisher_Interface<Tpub>::Subscription_Interface
  {
    using mem_fnc_t = void (T::*)(Tpub const&);

  public:
    constexpr Memberfunction_CallbackSubscriber(T& obj, mem_fnc_t mem_fuc)
        : m_obj(obj)
        , m_mem_fuc(mem_fuc)
    {
    }

  private:
    void notify(Tpub const& val) override { return (this->m_obj.*m_mem_fuc)(val); }

    T&        m_obj;
    mem_fnc_t m_mem_fuc;
  };

  template <typename T> class Memberfunction_CallbackSubscriber<T, void>: public Publisher_Interface<void>::Subscription_Interface
  {
    using mem_fnc_t = void (T::*)();

  public:
    constexpr Memberfunction_CallbackSubscriber(T& obj, mem_fnc_t mem_fuc)
        : m_obj(obj)
        , m_mem_fuc(mem_fuc)
    {
    }

  private:
    void notify() override { return (this->m_obj.*m_mem_fuc)(); }

    T&        m_obj;
    mem_fnc_t m_mem_fuc;
  };

  template <typename Tpub, typename Tsub> class TransformationSubscriber: public Publisher_Interface<Tpub>::Subscription_Interface
  {
  public:
    TransformationSubscriber(wlib::Callback<void(Tsub const&)>&                                     target_callback,
                             wlib::Callback<void(wlib::Callback<void(Tsub const&)>&, Tpub const&)>& transformer_callback)
        : m_target_callback(target_callback)
        , m_transformer_callback(transformer_callback)
    {
    }

  private:
    void                               notify(Tpub const& val) override { return this->m_transformer_callback(this->m_target_callback, val); }
    wlib::Callback<void(Tsub const&)>& m_target_callback;
    wlib::Callback<void(wlib::Callback<void(Tsub const&)>&, Tpub const&)>& m_transformer_callback;
  };

}    // namespace wlib::publisher

#endif
