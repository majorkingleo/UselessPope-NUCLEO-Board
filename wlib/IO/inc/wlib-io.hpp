#pragma once
#ifndef WLIB_IO_HPP_INCLUDED
#define WLIB_IO_HPP_INCLUDED

namespace wlib::io
{
  class DigitalInput_Interface
  {
  public:
    virtual ~DigitalInput_Interface() noexcept = default;

    virtual bool get() const = 0;
  };

  class DigitalOutput_Interface: public DigitalInput_Interface
  {
  public:
    virtual ~DigitalOutput_Interface() noexcept = default;

    virtual void set_high() = 0;
    virtual void set_low()  = 0;
    virtual void toggle() = 0;

    virtual void set(bool value)
    {
      if (value)
        return this->set_high();
      return this->set_low();
    }
  };
}    // namespace wlib::io
#endif
