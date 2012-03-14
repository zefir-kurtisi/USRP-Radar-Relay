/*
 * Copyright (c) 2012 Neratec Solutions AG
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <iostream>
#include <complex>

#include "Debug.hpp"
#include "UsrpRadarRelay.hpp"
#include <uhd/utils/thread_priority.hpp>

bool UsrpRadarRelayContinuous::set_pulse_pattern(struct pulse_pattern *pattern)
{
	return create_pulse_pattern(pattern);
}

bool UsrpRadarRelayContinuous::setup_tx(double freq, double rate, double gain)
{
	if (!configure_tx(freq, rate, gain))
		return false;
	// 1ms silence
	lo_tx_samples = (size_t)(operating_rate * 1e3);
	DLOG	<< "Setting up lo_buffer with " << lo_tx_samples << " samples"
		<< "(or=" << operating_rate << ")" << std::endl;
	low_buff.assign(lo_tx_samples, std::complex<float>(0.0, 0.0));

	md.start_of_burst = true;
	md.end_of_burst = false;
	md.has_time_spec = false;
	// start sequence and update descriptor
	send_sequence();
	md.start_of_burst = false;
	return true;
}

bool UsrpRadarRelayContinuous::send_sequence(void)
{
	if (pattern_repeats == 0) {
		size_t num_tx_samps =
			tx_stream->send(&low_buff.front(), lo_tx_samples, md);
		return (lo_tx_samples == num_tx_samps);
	} else {
		size_t num_tx_samps =
			tx_stream->send(&pattern_buff.front(), tx_samples, md);
		if (pattern_repeats > 0)
			pattern_repeats--;
		return (tx_samples == num_tx_samps);
	}
	return false;
}
