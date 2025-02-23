#pragma once
#ifndef BSLIB_LED_HPP_INCLUDED
#define BSLIB_LED_HPP_INCLUDED

namespace bslib
{
  class LED_Interface
  {
  public:
    virtual ~LED_Interface() noexcept = default;

    virtual bool is_on() const noexcept = 0;
    virtual void set(bool) noexcept     = 0;
    virtual void on() noexcept { return this->set(true); }
    virtual void off() noexcept { return this->set(false); }
    virtual void toggle() noexcept = 0;
  };
}    // namespace bslib::gpio
#endif
