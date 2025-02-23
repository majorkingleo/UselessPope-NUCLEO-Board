/*
 * sim_bsp_led.cpp
 *
 *  Created on: 10.10.2024
 *      Author: martin.oberzalek
 */
#include <bsp.hpp>

namespace {


class DummyOutPutPin : public bslib::gpio::DigitalOutput_Interface
{
	bool state = false;

public:
	//virtual ~DummyOutPutPin() noexcept = default;

	void set( bool state_ ) noexcept override {
		state = state_;
	}

	void toggle() noexcept override {
		state != state;
	}

	virtual bool get() const noexcept {
		return state;
	}
};

} // namespace

bslib::gpio::DigitalOutput_Interface& BSP::get_output_LED_green()
{
	static DummyOutPutPin pin;
	return pin;
}



