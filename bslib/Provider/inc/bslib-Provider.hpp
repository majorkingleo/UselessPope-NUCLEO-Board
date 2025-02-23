#pragma once
#ifndef BSLIB_PROVIDER_HPP_INCLUDED
#define BSLIB_PROVIDER_HPP_INCLUDED

#include <cstdint>
#include <os.hpp>
#include <wlib.hpp>

namespace bslib
{
  class Shared_Memory_Provider: public wlib::Shared_Memory_Provider_Interface
  {
  public:
    Shared_Memory_Provider(resource_t res)
        : Shared_Memory_Provider_Interface(res)
    {
    }

  private:
    virtual void lock() override { this->m_tex.lock(); }
    virtual void unlock() override { this->m_tex.unlock(); };
    os::mutex    m_tex;
  };
}    // namespace bslib
#endif
