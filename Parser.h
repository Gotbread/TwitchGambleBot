#pragma once

#include <string>
#include <vector>

#include <boost/variant.hpp>
#include <boost/optional.hpp>

struct TwitchMessage
{
	typedef std::pair<std::string, std::string> Property;

	std::vector<Property> properties;
	std::string channel;
	std::string message;
};
struct TwitchPing
{
	std::string server;
};

bool ParseMessage(const std::string &str, boost::variant<TwitchMessage, TwitchPing> &out);

typedef boost::optional<unsigned> GambleAmount;

struct TwitchGamble
{
	GambleAmount amount;
};
struct TwitchSafeGamble : public TwitchGamble
{
};
struct TwitchReverseGamble : public TwitchGamble
{
};

bool ParseGamble(const std::string &str, boost::variant<TwitchSafeGamble, TwitchReverseGamble, TwitchGamble> &out);

struct GambleResult
{
	unsigned roll;
	std::string username;
	unsigned munies_changed;
	unsigned munies_total;
};
struct GambleWinResult : public GambleResult
{
};
struct GambleLooseResult : public GambleResult
{
};
struct GambleJackpotResult : public GambleResult
{
};

bool ParseGambleResult(const std::string &str, boost::variant<GambleWinResult, GambleLooseResult, GambleJackpotResult> &out);

struct GambleTimeout
{
	std::string username;
	unsigned timer;
};

bool ParseGambleTimeout(const std::string &str, GambleTimeout &out);

struct BotCommandSetLicense
{
	std::string username;
	unsigned amount;
};
struct BotCommandSetDefaultLicense
{
	unsigned amount;
};
struct BotCommandGetLicense
{
	std::string username;
};
struct BotCommandGetAllLicenses
{
	int dummy;
};

bool ParseBotCommand(const std::string &str, boost::variant<BotCommandSetLicense, BotCommandSetDefaultLicense, BotCommandGetLicense, BotCommandGetAllLicenses> &out);

struct GiveCommand
{
	std::string from;
	unsigned amount;
	std::string to;
};

bool ParseGiveCommand(const std::string &str, GiveCommand &out);