#pragma once

#include <string>
#include <chrono>

class NMEAParser
{
public:
	enum class Result
	{
		Success = 0,
		BadChecksum,
		InvalidStart,
		InvalidFormat,
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

	struct NmeaResult
	{
		Result result;
		Type type;
	};

	struct NmeaMsg
	{
		time_t timestamp;
	};

	struct GGAMsg : public NmeaMsg
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
			DeadReckoning, // TODO: v2.3 and later
		} fix_indicator;

		unsigned int satellites_used;
		float hdop;
		float alt_msl;
		float geoid_separation;
		int age_of_diff_data;
	};

	struct GLLMsg : public NmeaMsg
	{
		double latitude;
		double longitude;
		float time;
		bool valid;

		enum class Mode
		{
			Autonomous = 0,
			DGPS,
			DR,
		} mode; // TODO: v3.00 and later
	};

	struct GSAMsg : public NmeaMsg
	{
		enum class Mode1
		{
			Manual = 0,
			Automatic,
		} mode1;

		enum class Mode2
		{
			FixUnavailable = 0,
			Fix2D,
			Fix3D,
		} mode2;

		int SatCh1;
		int SatCh2;
		int SatCh3;
		int SatCh4;
		int SatCh5;
		int SatCh6;
		int SatCh7;
		int SatCh8;
		int SatCh9;
		int SatCh10;
		int SatCh11;
		int SatCh12;

		float pdop;
		float hdop;
		float vdop;
	};

public:
	NMEAParser();
	
	NmeaResult ProcessMsg(const std::string& data);

	inline GGAMsg GetGGA() const { return gga_msg; }
	inline GLLMsg GetGLL() const { return gll_msg; }
	inline GSAMsg GetGSA() const { return gsa_msg; }

private:
	struct ChecksumResult
	{
		int checksum;
		int end_delimeter_idx;
	};

	Result ProcessGGA(const std::string& data);
	Result ProcessGLL(const std::string& data);
	Result ProcessGSA(const std::string& data);

	ChecksumResult calculateChecksum(const std::string& data) const;
	bool isChecksumGood(const std::string& data) const;
	std::string getParam(std::istringstream& ss, bool last = false) const;

	float getTime(const std::string& param) const;
	double getLatLon(const std::string& param, char dir, bool lon) const;

	GGAMsg gga_msg = {};
	GLLMsg gll_msg = {};
	GSAMsg gsa_msg = {};
};