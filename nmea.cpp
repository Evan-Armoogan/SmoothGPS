#include "nmea.h"
#include <iomanip>
#include <sstream>
#include <cstdint>

NMEAParser::NMEAParser()
{
}

NMEAParser::NmeaResult NMEAParser::ProcessMsg(const std::string& data)
{
	Type type = Type::None;
	Result res = Result::None;

	if (data[0] != '$')
		return { Result::InvalidStart, Type::None };

	std::string type_str = data.substr(1, data.find(',') - 1);

	if (type_str == "GPGGA")
		type = Type::GGA;
	else if (type_str == "GPGLL")
		type = Type::GLL;
	else if (type_str == "GPGSA")
		type = Type::GSA;
	else
		return { Result::InvalidType, Type::None };

	if (!isChecksumGood(data))
		return { Result::BadChecksum, type };

	NmeaMsg* msg = nullptr;
	// Message parsing will fail if the end delimeter comes earlier than expected
	// Catch the error thrown by the string class and handle it
	try
	{
		switch (type)
		{
		case Type::GGA:
			res = ProcessGGA(data);
			msg = &gga_msg;
			break;
		case Type::GLL:
			res = ProcessGLL(data);
			msg = &gll_msg;
			break;
		case Type::GSA:
			res = ProcessGSA(data);
			msg = &gsa_msg;
			break;
		}
	}
	catch (std::invalid_argument ex)
	{
		res = Result::InvalidFormat;
	}

	if (msg)
	{
		using namespace std::chrono;
		msg->timestamp = system_clock::to_time_t(system_clock::now());
	}

	return { res, type };
}


NMEAParser::ChecksumResult NMEAParser::calculateChecksum(const std::string& data) const
{
	uint8_t checksum = 0;
	int i;

	// skip first character because it is the start delimeter
	for (i = 1; i < data.length(); i++)
	{
		if (data[i] == '*')
			break; // reached the end delimeter
		checksum ^= data[i];
	}

	return { checksum, i };
}

bool NMEAParser::isChecksumGood(const std::string& data) const
{
	NMEAParser::ChecksumResult result = calculateChecksum(data);

	std::ostringstream ss;
	ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << result.checksum;

	std::string msg_checksum = data.substr(result.end_delimeter_idx + 1, 2);
	
	return msg_checksum == ss.str();
}

std::string NMEAParser::getParam(std::istringstream& ss, bool last) const
{
	std::string param;
	if (!last)
		std::getline(ss, param, ',');
	else
	{
		std::getline(ss, param, '*');

		// Check if the string we parsed was valid
		if (param.find(",") != std::string::npos // No comma should be found in last parameter
			|| ss.eof()) // EOF should not be set if end delimeter was found
			return "InvalidNmeaFormat";
	}

	return param;
}

float NMEAParser::getTime(const std::string& param) const
{
	unsigned int hour = std::stoi(param.substr(0, 2));
	unsigned int min = std::stoi(param.substr(2, 2));
	unsigned int sec = std::stoi(param.substr(4, 2));
	unsigned int ms = std::stoi(param.substr(7, 3));

	float time = hour * 60 * 60 + min * 60 + sec + ms * 0.001f;
	return time;
}

double NMEAParser::getLatLon(const std::string& param, char dir, bool lon) const
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
	std::istringstream stream(data);
	std::string param;
	std::string direction;

	/**
	* Create a temporary structure here and copy it to the class member
	* at the end once we've verified the message is in the valid NMEA
	* format.
	*/
	GGAMsg msg;

	// discard first param as it indicates the msg type, already been parsed
	std::getline(stream, param, ',');

	param = getParam(stream); // gets time
	msg.time = getTime(param);

	param = getParam(stream); // gets latitude
	direction = getParam(stream); // gets direction
	msg.latitude = getLatLon(param, direction[0], false);

	param = getParam(stream); // gets longitude
	direction = getParam(stream); // gets direction
	msg.longitude = getLatLon(param, direction[0], true);

	param = getParam(stream); // get fix indicator
	msg.fix_indicator = static_cast<GGAMsg::PositionFixIndicator>(std::stoi(param));

	param = getParam(stream); // get sats used
	msg.satellites_used = std::stoi(param);

	param = getParam(stream); // get hdop
	msg.hdop = std::stof(param);

	param = getParam(stream); // get MSL alt
	msg.alt_msl = std::stof(param);

	param = getParam(stream); // discard unit

	param = getParam(stream); // get MSL alt
	msg.geoid_separation = std::stof(param);

	param = getParam(stream); // discard unit

	param = getParam(stream); // get age of diff data
	if (param == "")
		msg.age_of_diff_data = -1;
	else
		msg.age_of_diff_data = std::stoi(param);

	param = getParam(stream, true); // discard diff ref station id

	if (param == "InvalidNmeaFormat")
		return Result::InvalidFormat;

	gga_msg = msg;
	return Result::Success;
}

NMEAParser::Result NMEAParser::ProcessGLL(const std::string& data)
{
	std::istringstream stream(data);
	std::string param;
	std::string direction;

	/**
	* Create a temporary structure here and copy it to the class member
	* at the end once we've verified the message is in the valid NMEA
	* format.
	*/
	GLLMsg msg;

	// discard first param as it indicates the msg type, already been parsed
	getParam(stream);

	param = getParam(stream); // gets latitude
	direction = getParam(stream); // gets direction
	msg.latitude = getLatLon(param, direction[0], false);

	param = getParam(stream); // gets longitude
	direction = getParam(stream); // gets direction
	msg.longitude = getLatLon(param, direction[0], true);

	param = getParam(stream); // gets time
	msg.time = getTime(param);

	param = getParam(stream); // gets status
	msg.valid = param == "A";

	// TODO: v3.00 and later
	param = getParam(stream, true);
	if (param == "A")
		msg.mode = GLLMsg::Mode::Autonomous;
	else if (param == "D")
		msg.mode = GLLMsg::Mode::DGPS;
	else if (param == "E")
		msg.mode = GLLMsg::Mode::DR;
	else
		return Result::InvalidFormat;

	gll_msg = msg;
	return Result::Success;
}

NMEAParser::Result NMEAParser::ProcessGSA(const std::string& data)
{
	std::istringstream stream(data);
	std::string param;
	std::string direction;

	/**
	* Create a temporary structure here and copy it to the class member
	* at the end once we've verified the message is in the valid NMEA
	* format.
	*/
	GSAMsg msg;

	// discard first param as it indicates the msg type, already been parsed
	getParam(stream);

	param = getParam(stream); // gets Mode 1
	msg.mode1 = param == "M" ? GSAMsg::Mode1::Manual : GSAMsg::Mode1::Automatic;

	param = getParam(stream); // gets Mode 2
	if (param == "1")
		msg.mode2 = GSAMsg::Mode2::FixUnavailable;
	else if (param == "2")
		msg.mode2 = GSAMsg::Mode2::Fix2D;
	else
		msg.mode2 = GSAMsg::Mode2::Fix3D;

	param = getParam(stream); // gets Satelite Used Channel 1
	msg.SatCh1 = param == "" ? -1 : std::stoi(param);

	param = getParam(stream); // gets Satelite Used Channel 2
	msg.SatCh2 = param == "" ? -1 : std::stoi(param);

	param = getParam(stream); // gets Satelite Used Channel 3
	msg.SatCh3 = param == "" ? -1 : std::stoi(param);

	param = getParam(stream); // gets Satelite Used Channel 4
	msg.SatCh4 = param == "" ? -1 : std::stoi(param);

	param = getParam(stream); // gets Satelite Used Channel 5
	msg.SatCh5 = param == "" ? -1 : std::stoi(param);

	param = getParam(stream); // gets Satelite Used Channel 6
	msg.SatCh6 = param == "" ? -1 : std::stoi(param);

	param = getParam(stream); // gets Satelite Used Channel 7
	msg.SatCh7 = param == "" ? -1 : std::stoi(param);

	param = getParam(stream); // gets Satelite Used Channel 8
	msg.SatCh8 = param == "" ? -1 : std::stoi(param);

	param = getParam(stream); // gets Satelite Used Channel 9
	msg.SatCh9 = param == "" ? -1 : std::stoi(param);

	param = getParam(stream); // gets Satelite Used Channel 10
	msg.SatCh10 = param == "" ? -1 : std::stoi(param);

	param = getParam(stream); // gets Satelite Used Channel 11
	msg.SatCh11 = param == "" ? -1 : std::stoi(param);

	param = getParam(stream); // gets Satelite Used Channel 12
	msg.SatCh12 = param == "" ? -1 : std::stoi(param);

	param = getParam(stream); // gets PDOP
	msg.pdop = std::stof(param);

	param = getParam(stream); // gets HDOP
	msg.hdop = std::stof(param);

	param = getParam(stream, true); // gets VDOP
	if (param != "InvalidNmeaFormat")
		msg.vdop = std::stof(param);
	else
		return Result::InvalidFormat;

	gsa_msg = msg;
	return Result::Success;
}