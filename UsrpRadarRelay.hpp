#ifndef USRP_RADAR_RELAY_HPP
#define USRP_RADAR_RELAY_HPP
#include <uhd/usrp/multi_usrp.hpp>

#include "RadarJsonRpc.hpp"

class UsrpRadarRelay {
public:
	UsrpRadarRelay();
	~UsrpRadarRelay();
	virtual bool
		setup_tx(double freq=5505, double rate=10, double gain=20) = 0;
	virtual bool set_pulse_pattern(struct pulse_pattern *p) = 0;
	virtual bool send_sequence(void) = 0;

protected:
	bool configure_tx(double freq, double rate, double gain);
	bool create_pulse_pattern(struct pulse_pattern *p);
	uhd::usrp::multi_usrp::sptr usrp;
	uhd::tx_streamer::sptr tx_stream;
	uhd::tx_metadata_t md;
	std::vector<std::complex<float> > pattern_buff;
	size_t tx_samples;
	size_t num_channels;
	double operating_freq;
	double operating_rate;
	double operating_gain;
	int pattern_repeats;
};

class UsrpRadarRelayOnDemand : public UsrpRadarRelay
{
public:
	bool setup_tx(double freq=5505, double rate=10.0, double gain=20.0);
	bool set_pulse_pattern(struct pulse_pattern *p);
	bool send_sequence(void);
};

class UsrpRadarRelayContinuous : public UsrpRadarRelay
{
public:
	bool setup_tx(double freq=5505, double rate=10.0, double gain=20.0);
	bool set_pulse_pattern(struct pulse_pattern *p);
	bool send_sequence(void);

private:
	std::vector<std::complex<float> > low_buff;
	size_t lo_tx_samples;
};


#endif /* USRP_RADAR_RELAY_HPP */
