#ifndef RADAR_JSON_RPC_H
#define RADAR_JSON_RPC_H

#include <sys/types.h>
#include <string>

struct pulse {
	int ts;
	float width;
	float ampl;
};
struct pulse_pattern {
	uint duration;
	uint num_pulses;
	int repeats;
	uint repeat_interval;
	struct pulse *pulses;
};

struct device_config {
	double freq;
	double rate;
	double gain;
};

#define RESULT_RESPONSE		"result"
#define RESULT_INFO		"info"
#define PULSE_PATTERN_CMD	"pulse_pattern"
#define USRP_CONFIG_CMD		"device_config"

enum json_cmd_id {
	CMD_PULSE_PATTERN,
	CMD_CONFIGURE,
};

struct json_cmd {
	enum json_cmd_id cmd_id;
	union {
		struct device_config config;
		struct pulse_pattern pattern;
	};
};

class RadarJsonRpc {
public:
	RadarJsonRpc();
	~RadarJsonRpc();

	// get the command corresponding to given request
	struct json_cmd *get_request(std::string& request);
	// get the result string for result code and optional info
	std::string& set_result(bool result, std::string info="");
	// get result code for given result string
	bool get_result(std::string& result);
	// get the command corresponding to request in given file
	struct json_cmd *get_request_from_file(const char *filename);

private:
	struct json_cmd request_cmd;
	struct pulse_pattern pulse_pattern;
	std::string request_buffer;
	bool parse_JSON_request(std::string& request);
	bool parse_pattern_request(struct json_object *j_cmd);
	bool parse_config_request(struct json_object *j_cmd);
	struct json_cmd *parse_request(struct json_object *j_cmd);
};

#endif /* RADAR_JSON_RPC_H */
