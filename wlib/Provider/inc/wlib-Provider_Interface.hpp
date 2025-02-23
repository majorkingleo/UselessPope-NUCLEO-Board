#pragma once
#ifndef WLIB_PROVIDER_INTERFACE_HPP_INCLUDED
#define WLIB_PROVIDER_INTERFACE_HPP_INCLUDED

#include <cstddef>
#include <cstdint>
#include <span>

namespace wlib
{
  class Shared_Memory_Provider_Interface
  {
  public:
    using resource_t       = std::span<std::byte>;
    using const_resource_t = std::span<std::byte const>;

    class resource_token_t
    {
    public:
      resource_token_t(Shared_Memory_Provider_Interface& provider, resource_t resource)
          : m_pro(provider)
          , m_res(resource)
      {
        this->m_pro.lock();
      }
      resource_token_t(resource_token_t const&)            = delete;
      resource_token_t(resource_token_t&&)                 = delete;
      resource_token_t& operator=(resource_token_t const&) = delete;
      resource_token_t& operator=(resource_token_t&&)      = delete;

      ~resource_token_t() { this->m_pro.unlock(); }

      resource_t       get() & { return this->m_res; }
      const_resource_t get() const& { return this->m_res; }

    private:
      Shared_Memory_Provider_Interface& m_pro;
      resource_t                        m_res;
    };

    Shared_Memory_Provider_Interface(std::span<std::byte> memory)
        : m_memory{ memory }
    {
    }
    virtual ~Shared_Memory_Provider_Interface() = default;

    resource_token_t request() { return resource_token_t{ *this, this->m_memory }; }

  private:
    friend resource_token_t;
    virtual void         lock()   = 0;
    virtual void         unlock() = 0;
    std::span<std::byte> m_memory;
  };

  template <typename T> class Shared_Resource_Provider_Interface
  {
  public:
    using resource_t       = T;
    using const_resource_t = const T;

    class resource_token_t
    {
    public:
      resource_token_t(Shared_Resource_Provider_Interface& provider, resource_t& resource)
          : m_pro(provider)
          , m_res(resource)
      {
        this->m_pro.lock();
      }
      resource_token_t(resource_token_t const&)            = delete;
      resource_token_t(resource_token_t&&)                 = delete;
      resource_token_t& operator=(resource_token_t const&) = delete;
      resource_token_t& operator=(resource_token_t&&)      = delete;

      ~resource_token_t() { this->m_pro.unlock(); }

      resource_t&       get() & { return this->m_res; }
      const_resource_t& get() const& { return this->m_res; }

    private:
      Shared_Resource_Provider_Interface& m_pro;
      resource_t&                         m_res;
    };

    Shared_Resource_Provider_Interface(T& resource)
        : m_resource{ resource }
    {
    }
    virtual ~Shared_Resource_Provider_Interface() = default;

    resource_token_t request() { return resource_token_t{ *this, this->m_resource }; }

  private:
    friend resource_token_t;
    virtual void lock()   = 0;
    virtual void unlock() = 0;
    T&           m_resource;
  };

}    // namespace wlib

#endif
