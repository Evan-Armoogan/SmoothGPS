#include <iostream>
#include "nmea.h"
#include <string>
#include <sstream>
#include <cassert>


int main()
{
	const std::string gga_data = "$GPGGA,002153.000,3342.6618,N,11751.3858,W,1,10,1.2,27.0,M,-34.2,M,20,0000*5C";
	const std::string gll_data = "$GPGLL,3723.2475,N,12158.3416,W,161229.487,A,A*41";
	NMEAParser parser;

	auto msg = parser.ProcessMsg(gll_data);

	std::cout << static_cast<int>(msg.result) << std::endl;
	std::cout << static_cast<int>(msg.type) << std::endl;

	assert(msg.result == NMEAParser::Result::Success);

	std::cout << std::endl;

	switch (msg.type)
	{
		case NMEAParser::Type::GGA:
		{
			std::cout << "GGA Message: " << std::endl;
			auto ret = parser.GetGGA();
			std::cout << ret.timestamp << std::endl;
			std::cout << ret.time << std::endl;
			std::cout << ret.latitude << std::endl;
			std::cout << ret.longitude << std::endl;
			std::cout << static_cast<int>(ret.fix_indicator) << std::endl;
			std::cout << ret.satellites_used << std::endl;
			std::cout << ret.hdop << std::endl;
			std::cout << ret.alt_msl << std::endl;
			std::cout << ret.geoid_separation << std::endl;
			std::cout << ret.age_of_diff_data << std::endl;
			break;
		}
		case NMEAParser::Type::GLL:
		{
			std::cout << "GLL Message: " << std::endl;
			auto ret = parser.GetGLL();
			std::cout << ret.timestamp << std::endl;
			std::cout << ret.latitude << std::endl;
			std::cout << ret.longitude << std::endl;
			std::cout << ret.time << std::endl;
			std::cout << ret.valid << std::endl;
			std::cout << static_cast<int>(ret.mode) << std::endl;
			break;
		}
	}

}