#include "nmea.h"
#include <iomanip>
#include <sstream>
#include <cstdint>

NMEAParser::NMEAParser() : gga_msg({0})
{
}


NMEAParser::ChecksumResult NMEAParser::calculateChecksum(const std::string& data)
{
	// skip first character because it is the $ sign
	uint8_t checksum = data[1] ^ data[2];

	int i;
	for (i = 3; true; i++)
	{
		if (data[i] == '*')
			break; // reached the end delimeter
		checksum ^= data[i];
	}

	return { checksum, i };
}

bool NMEAParser::isChecksumGood(const std::string& data)
{
	NMEAParser::ChecksumResult result = calculateChecksum(data);

	std::ostringstream ss;
	ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << result.checksum;

	std::string msg_checksum = data.substr(result.end_delimeter_idx + 1, 2);
	
	return msg_checksum == ss.str();
}

static float getTime(const std::string& param)
{
	unsigned int hour = std::stoi(param.substr(0, 2));
	unsigned int min = std::stoi(param.substr(2, 2));
	unsigned int sec = std::stoi(param.substr(4, 2));
	unsigned int ms = std::stoi(param.substr(7, 3));

	float time = hour * 60 * 60 + min * 60 + sec + ms * 0.001f;
	return time;
}

static double getLatLon(const std::string& param, char dir, bool lon)
{
	int minute_index = lon ? 3 : 2;

	unsigned int degrees = std::stoi(param.substr(0, minute_index));
	double minutes = std::stod(param.substr(minute_index, 7));

	double result = degrees + minutes / 60;

	if (dir == 'S' || dir == 'W')
		result *= -1;

	return result;
}

NMEAParser::Result NMEAParser::ProcessGGA(const std::string& data)
{
	if (!isChecksumGood(data))
		return Result::BadChecksum;

	std::istringstream stream(data);

	std::string param;

	// discard first param as it indicates the msg type
	std::getline(stream, param, ',');

	std::getline(stream, param, ','); // gets time
	gga_msg.time = getTime(param);

	std::string direction;
	std::getline(stream, param, ','); // gets latitude
	std::getline(stream, direction, ','); // gets direction
	gga_msg.latitude = getLatLon(param, direction[0], false);

	std::getline(stream, param, ','); // gets longitude
	std::getline(stream, direction, ','); // gets direction
	gga_msg.longitude = getLatLon(param, direction[0], true);

	std::getline(stream, param, ','); // get fix indicator
	gga_msg.fix_indicator = static_cast<GGAMsg::PositionFixIndicator>(std::stoi(param));

	std::getline(stream, param, ','); // get sats used
	gga_msg.satellites_used = std::stoi(param);

	std::getline(stream, param, ','); // get hdop
	gga_msg.hdop = std::stof(param);

	std::getline(stream, param, ','); // get MSL alt
	gga_msg.alt_msl = std::stof(param);

	std::getline(stream, param, ','); // discard unit

	std::getline(stream, param, ','); // get MSL alt
	gga_msg.geoid_separation = std::stof(param);

	std::getline(stream, param, ','); // discard unit

	std::getline(stream, param, ','); // discard unit
	if (param == "")
		gga_msg.age_of_diff_data = -1;
	else
		gga_msg.age_of_diff_data = std::stoi(param);

	return Result::Success;
}