#include "bsp.hpp"
#include "os.hpp"

namespace {

class SimAnalogValues : public bslib::publisher::LF_Publisher<BSP::analog_values_t, 5>
{
private:
	os::Static_MemberfunctionCallbackTask<SimAnalogValues, 5120>  m_worker = { *this, &SimAnalogValues::generate_analog_values, "SimAnalogValues" };

public:
	~SimAnalogValues() {
	}

private:
	void generate_analog_values()
	{
		while( os::this_thread::keep_running() ) {
			std::this_thread::sleep_for( std::chrono::milliseconds(10) );
			this->notify( BSP::analog_values_t { 1 } );
		}
	}

};

} // namespace

bslib::publisher::Publisher_Interface<BSP::analog_values_t>& BSP::get_analog_value_publisher()
{
    static SimAnalogValues pub;
    return pub;
}
