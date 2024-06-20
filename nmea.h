#pragma once

#include <string>
#include <memory>

class NMEAParser
{
public:
	enum class Result
	{
		Success = 0,
		BadChecksum,
		None,
	};

	enum class Type
	{
		GGA = 0,
		GLL,
		GSA,
		GSV,
		MSS,
		RMC,
		VTG,
		None,
	};

	struct MsgResult
	{
		Result result;
		Type type;
	};

	struct GGAMsg
	{
		float time;
		double latitude;
		double longitude;

		enum class PositionFixIndicator
		{
			Invalid = 0,
			Fix,
			DiffFix,
			Unsupported,
			DeadReckoning,
		} fix_indicator;

		unsigned int satellites_used;
		float hdop;
		float alt_msl;
		float geoid_separation;
		int age_of_diff_data;
	};

public:
	NMEAParser();
	
	MsgResult ProcessMsg(const std::string& data)
	{
		Type type = Type::None;
		Result res = Result::None;

		std::string type_str = data.substr(1, data.find(',') - 1);

		if (type_str == "GPGGA")
			type = Type::GGA;

		switch (type)
		{
		case Type::GGA:
			res = ProcessGGA(data);
			break;
		}

		return { res, type };
	}

	inline GGAMsg GetGGA() const { return gga_msg; }

private:
	struct ChecksumResult
	{
		int checksum;
		int end_delimeter_idx;
	};

	ChecksumResult calculateChecksum(const std::string& data);
	bool isChecksumGood(const std::string& data);

	Result ProcessGGA(const std::string& data);

	GGAMsg gga_msg;
};