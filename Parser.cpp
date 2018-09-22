#include "Parser.h"

#include <boost/fusion/include/std_pair.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_lexeme.hpp>
#include <boost/spirit/include/qi_skip.hpp>
#include <boost/spirit/include/qi_no_skip.hpp>
#include <boost/spirit/include/qi_alternative.hpp>
#include <boost/spirit/include/qi_no_case.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/repository/include/qi_seek.hpp>
#include <boost/spirit/include/phoenix.hpp>

BOOST_FUSION_ADAPT_STRUCT(
	TwitchMessage,
	(std::vector<TwitchMessage::Property>, properties),
	(std::string, channel),
	(std::string, message));

BOOST_FUSION_ADAPT_STRUCT(
	TwitchPing,
	(std::string, server));

bool ParseMessage(const std::string &str, boost::variant<TwitchMessage, TwitchPing> &out)
{
	using namespace boost::spirit::qi;

	typedef std::string::const_iterator it;

	auto string_delim = [](char sep) -> rule<it, std::string()>
	{
		return *(char_ - sep) >> sep;
	};
	auto string_delim2 = [](const std::string &chars) -> rule<it, std::string()>
	{
		return *(char_ - char_(chars)) >> omit[char_(chars)];
	};
	rule<it, std::string()> property_key = string_delim('=');
	rule<it, std::string()> property_value = string_delim2("; ");
	rule<it, TwitchMessage::Property()> property_rule = property_key >> property_value;
	rule<it, std::string()> priv_msg_rule = lit("PRIVMSG");
	rule<it, std::string()> channel_rule = string_delim(' ');
	rule<it, std::string()> message_rule = *char_;
	rule<it, std::string()> skip_name_rule = string_delim(' ');

	rule<it, TwitchMessage()> message_parser = *property_rule >> omit[skip_name_rule] >> omit[priv_msg_rule] >> lit(" #") >> channel_rule >> lit(':') >> message_rule;

	rule<it, TwitchPing()> ping_parser = lit("PING") >> lit(" :") >> *char_;

	it iter = str.cbegin();
	bool res = parse(iter, str.cend(), message_parser | ping_parser, out);

	return res && iter == str.end();
}

BOOST_FUSION_ADAPT_STRUCT(
	TwitchGamble,
	(GambleAmount, amount));

BOOST_FUSION_ADAPT_STRUCT(
	TwitchSafeGamble,
	(GambleAmount, amount));

BOOST_FUSION_ADAPT_STRUCT(
	TwitchReverseGamble,
	(GambleAmount, amount));

bool ParseGamble(const std::string &str, boost::variant<TwitchSafeGamble, TwitchReverseGamble, TwitchGamble> &out)
{
	using namespace boost::spirit::qi;

	typedef std::string::const_iterator it;

	using boost::spirit::ascii::no_case;

	rule<it, unsigned()> number_rule = int_;
	rule<it> all_rule = no_case[lit("all")];
	rule<it> safe_rule = no_case[lit("safe")];
	rule<it> reverse_rule = no_case[lit("reverse")];
	rule<it, GambleAmount()> amount_rule = number_rule | all_rule;
	rule<it> gamble_rule = no_case[lit("!gamble")];

	rule<it, TwitchGamble()> regular_gambling = omit[gamble_rule] >> lit(" ") >> amount_rule;
	rule<it, TwitchSafeGamble()> safe_gambling = omit[gamble_rule] >> lit(" ") >> amount_rule >> lit(" ") >> safe_rule >> -omit[lit(" ") >> *char_];
	rule<it, TwitchSafeGamble()> reverse_gambling = omit[gamble_rule] >> lit(" ") >> amount_rule >> lit(" ") >> reverse_rule >> -omit[lit(" ") >> *char_];

	it iter = str.cbegin();
	bool res = parse(iter, str.cend(), safe_gambling | reverse_gambling | regular_gambling, out);

	return res && iter == str.end();
}

BOOST_FUSION_ADAPT_STRUCT(
	GambleWinResult,
	(unsigned, roll),
	(std::string, username),
	(unsigned, munies_changed),
	(unsigned, munies_total));

BOOST_FUSION_ADAPT_STRUCT(
	GambleLooseResult,
	(unsigned, roll),
	(std::string, username),
	(unsigned, munies_changed),
	(unsigned, munies_total));

BOOST_FUSION_ADAPT_STRUCT(
	GambleJackpotResult,
	(unsigned, roll),
	(std::string, username),
	(unsigned, munies_changed),
	(unsigned, munies_total));

bool ParseGambleResult(const std::string &str, boost::variant<GambleWinResult, GambleLooseResult, GambleJackpotResult> &out)
{
	using namespace boost::spirit::qi;

	typedef std::string::const_iterator it;

	// Rolled 100. @Gotbread won the jackpot of 83376 Munies and now has a total of 461347 Munies
	rule<it, unsigned()> roll_rule = int_;
	rule<it, unsigned()> amount_rule = int_;
	rule<it, std::string()> username_rule = *(char_ - space);

	rule<it, GambleLooseResult()> loose_rule = "Rolled " >> roll_rule >> ". @" >> username_rule >> " lost " >> amount_rule >> " Munies and now has " >> amount_rule >> " Munies";
	rule<it, GambleWinResult()> win_rule = "Rolled " >> roll_rule >> ". @" >> username_rule >> " won " >> amount_rule >> " Munies and now has " >> amount_rule >> " Munies";
	rule<it, GambleJackpotResult()> jackpot_rule = "Rolled " >> roll_rule >> ". @" >> username_rule >> " won the jackpot of " >> amount_rule >> " Munies and now has a total of " >> amount_rule >> " Munies";

	it iter = str.cbegin();
	bool res = parse(iter, str.cend(), loose_rule | win_rule | jackpot_rule, out);

	return res && iter == str.end();
}

BOOST_FUSION_ADAPT_STRUCT(
	GambleTimeout,
	(std::string, username),
	(unsigned, timer));

bool ParseGambleTimeout(const std::string &str, GambleTimeout &out)
{
	using namespace boost::spirit::qi;

	typedef std::string::const_iterator it;

	// @Gotbread you can't gamble, you are on a cooldown for 70 seconds!
	rule<it, std::string()> username_rule = *(char_ - space);
	rule<it, unsigned()> amount_rule = int_;

	rule<it, GambleTimeout()> timeout_rule = lit("@") >> username_rule >> " you can't gamble, you are on a cooldown for " >> amount_rule >> " seconds!";

	it iter = str.cbegin();
	bool res = parse(iter, str.cend(), timeout_rule, out);

	return res && iter == str.end();
}

BOOST_FUSION_ADAPT_STRUCT(
	BotCommandSetLicense,
	(std::string, username),
	(unsigned, amount));

BOOST_FUSION_ADAPT_STRUCT(
	BotCommandSetDefaultLicense,
	(unsigned, amount));

BOOST_FUSION_ADAPT_STRUCT(
	BotCommandGetLicense,
	(std::string, username));

BOOST_FUSION_ADAPT_STRUCT(
	BotCommandGetAllLicenses,
	(int, dummy));

bool ParseBotCommand(const std::string &str, boost::variant<BotCommandSetLicense, BotCommandSetDefaultLicense, BotCommandGetLicense, BotCommandGetAllLicenses> &out)
{
	using namespace boost::spirit::qi;

	typedef std::string::const_iterator it;

	// gamblebot set license username <limit>
	// gamblebot set default license <limit>
	// gamblebot get license unsername
	// gamblebot get all licenses
	rule<it, std::string()> username_rule = *(char_ - space - eol);
	rule<it, unsigned()> amount_rule = int_;

	rule<it, BotCommandSetDefaultLicense()> set_default_license_rule = lit("gamblebot set default license ") >> amount_rule;
	rule<it, BotCommandSetLicense()> set_license_rule = lit("gamblebot set license ") >> username_rule >> lit(" ") >> amount_rule;
	rule<it, BotCommandGetLicense()> get_license_rule = lit("gamblebot get license ") >> username_rule;
	rule<it, BotCommandGetAllLicenses()> get_all_license_rule = attr(0) >> lit("gamblebot get all licenses");

	it iter = str.cbegin();
	bool res = parse(iter, str.cend(), set_default_license_rule | set_license_rule | get_license_rule | get_all_license_rule, out);

	return res && iter == str.end();
}

BOOST_FUSION_ADAPT_STRUCT(
	GiveCommand,
	(std::string, from),
	(unsigned, amount),
	(std::string, to));

struct number_builder
{
	template<typename> struct result { typedef unsigned type; };
	template<typename Number, typename Ch> unsigned operator()(Number number, Ch ch) const
	{
		assert(ch >= '0' && ch <= '9');
		return number * 10 + (ch - '0');
	}
};
bool ParseGiveCommand(const std::string &str, GiveCommand &out)
{
	using namespace boost::spirit::qi;
	using namespace boost::spirit::qi::labels;

	typedef std::string::const_iterator it;

	boost::phoenix::function<number_builder> _number_builder;

	// Gotbread gave 11,000 Munies to tacotruck59
	rule<it, std::string()> username_rule = *(char_ - space - eol);
	rule<it, unsigned()> comma_amount_rule = eps[_val = 0] >> +(digit[_val = _number_builder(_val, _1)] | ',');

	rule<it, GiveCommand()> give_rule = username_rule >> " gave " >> comma_amount_rule >> " Munies to " >> username_rule;

	it iter = str.cbegin();
	bool res = parse(iter, str.cend(), give_rule, out);

	return res && iter == str.end();
}