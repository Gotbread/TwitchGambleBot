#include "MainWindow.h"

#include <CommCtrl.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <nlohmann/json.hpp>
#include <boost/algorithm/string.hpp>

#include "Parser.h"
#include "Utility.h"

using namespace std::literals;

MainWindow::MainWindow(HINSTANCE hInstance) : BaseWindow(hInstance)
{
}
bool MainWindow::OnInit()
{
	DWORD style = WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
	RECT rect;
	SetRect(&rect, 0, 0, WindowWidth, WindowHeight);
	AdjustWindowRect(&rect, style, 0);

	hWnd = CreateWindow(classname, "Twitch Gamble Bot - by Gotbread", style,
		CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, 0, 0, hInstance, this);

	if (!hWnd)
		return false;

	edit_font = CreateFont(25, 0, 0, 0, 600, 0, 0, 0, 0, 0, 0, 0, 0, "Times New Roman");
	white_background = CreateSolidBrush(WindowColor);

	auto child = WS_CHILD | WS_VISIBLE;
	auto es = child | WS_BORDER | ES_AUTOVSCROLL;
	auto log = es | WS_TABSTOP | ES_READONLY | ES_MULTILINE | WS_VSCROLL;
	hUsername = CreateWindow(WC_EDIT, "", es | WS_TABSTOP, 10, 10, 200, 30, hWnd, 0, hInstance, 0);
	hToken = CreateWindow(WC_EDIT, "", es | WS_TABSTOP | ES_PASSWORD, 10, 50, 200, 30, hWnd, 0, hInstance, 0);
	hChannel = CreateWindow(WC_EDIT, "", es | WS_TABSTOP, 10, 90, 200, 30, hWnd, 0, hInstance, 0);
	hConnect = CreateWindow(WC_BUTTON, "Connect", child | WS_TABSTOP, 10, 130, 200, 40, hWnd, 0, hInstance, 0);
	hStart = CreateWindow(WC_BUTTON, "Start", child | WS_TABSTOP, 390, 10, 200, 40, hWnd, 0, hInstance, 0);
	hDebtLabel = CreateWindow(WC_STATIC, "Debt:", child, 390, 60, 200, 20, hWnd, 0, hInstance, 0);
	hDebt = CreateWindow(WC_EDIT, "", log, 220, 80, 370, 110, hWnd, 0, hInstance, 0);
	hChatLabel = CreateWindow(WC_STATIC, "Chat:", child, 10, 180, 200, 20, hWnd, 0, hInstance, 0);
	hChat = CreateWindow(WC_EDIT, "", log, 10, 200, 580, 290, hWnd, 0, hInstance, 0);
	SetRect(&TimerRect, 220, 5, 380, 60);

	SendMessage(hUsername, EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(L"Username"));
	SendMessage(hToken, EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(L"Token"));
	SendMessage(hChannel, EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(L"Channel"));

	auto f = reinterpret_cast<WPARAM>(edit_font.GetFont());
	SendMessage(hUsername, WM_SETFONT, f, TRUE);
	SendMessage(hToken, WM_SETFONT, f, TRUE);
	SendMessage(hChannel, WM_SETFONT, f, TRUE);
	SendMessage(hConnect, WM_SETFONT, f, TRUE);
	SendMessage(hStart, WM_SETFONT, f, TRUE);
	//SendMessage(hDebt, WM_SETFONT, f, TRUE);
	//SendMessage(hChat, WM_SETFONT, f, TRUE);

	LoadConfigFile();

	initial_ticks = ticks_remaining = (2 * 60 + 10) * 1000;

	return true;
}
void MainWindow::OnStart()
{
	ShowWindow(hWnd, SW_SHOWNORMAL);

	SetTimer(hWnd, 0, 1000 / 30, 0);
	last_ticks = GetTickCount();
}
void MainWindow::OnExit()
{
	if (chat)
		chat->Exit();
}
void MainWindow::OnOtherEvent(Queue::Reason reason)
{
	if (reason == Queue::ReasonMessage)
	{
		MSG Msg;
		while (PeekMessage(&Msg, 0, 0, 0, PM_REMOVE))
		{
			if (Msg.message == WM_QUIT)
			{
				ThreadManager::Exit();
			}
			else
			{
				if (!IsDialogMessage(hWnd, &Msg))
				{
					TranslateMessage(&Msg);
					DispatchMessage(&Msg);
				}
			}
		}
	}
}
void MainWindow::SetChat(TwitchIRC *chat)
{
	this->chat = chat;
}
void MainWindow::OnChatConnecting()
{
	SetWindowText(hConnect, "Connecting...");
}
void MainWindow::OnChatConnected()
{
	connected = true;
	SetWindowText(hConnect, "Disconnect");

	JoinChannel();

	SaveConfigFile();
}
void MainWindow::OnChatDisconnected()
{
	connected = false;
	running = false;
	SetWindowText(hConnect, "Connect");
	SetWindowText(hStart, "Start");
}
void MainWindow::OnChatMessage(std::string user, std::string message)
{
	if (StartsWith<ComparePolicy::CaseInsensitive>(message, "!gamble "s)) // check for gamble msg: !gamble 100 safe
	{
		boost::variant<TwitchSafeGamble, TwitchReverseGamble, TwitchGamble> out;
		bool res = ParseGamble(message, out);
		if (res)
		{
			if (TwitchSafeGamble *safe = boost::get<TwitchSafeGamble>(&out))
			{
				gamble_stakes[user] = { GetTickCount(), safe->amount };
			}
			else if (TwitchReverseGamble *reverse = boost::get<TwitchReverseGamble>(&out))
			{
			}
			else if (TwitchGamble *gamble = boost::get<TwitchGamble>(&out))
			{
			}
		}
		// !gamble amount safe
		// !gamble all safe
		// !gamble amount text
		// !gamble all text
		// evtl wörter nach safe zulassen
	}
	else if (boost::iequals(user, "KiddsBot"))
	{
		if (StartsWith<ComparePolicy::CaseInsensitive>(message, "Rolled "s)) // check for gamble msg:	Rolled 33. @Gotbread lost 100 Munies and now has 4393754 Munies
																			// or:						Rolled 100. @Gotbread won the jackpot of 83376 Munies and now has a total of 461347 Munies
		{
			float stake_multiplier = 1.1f;

			auto GetUserMaxStake = [&](auto &str)
			{
				auto iter = gamble_licenses.find(str);
				if (iter != gamble_licenses.end())
					return iter->second;

				return default_gamble_license;
			};

			boost::variant<GambleWinResult, GambleLooseResult, GambleJackpotResult> out;
			bool res = ParseGambleResult(message, out);
			if (res)
			{
				if (GambleWinResult *win = boost::get<GambleWinResult>(&out))
				{
					// check if user had a valid stake
					auto stake_iter = gamble_stakes.find(win->username);
					if (stake_iter != gamble_stakes.end())
					{
						// calculate stake
						unsigned stake = win->roll == 99 ? win->munies_changed / 3 : win->munies_changed / 2;
						// check if stake is in the license
						unsigned max_stake = GetUserMaxStake(win->username);
						if (stake <= max_stake)
						{
							// calculate share
							unsigned share = win->munies_changed - static_cast<unsigned>(stake * stake_multiplier);
							// put on debt
							auto &debt = gamble_debts[win->username];
							debt.amount += share;
							debt.update_ticks = GetTickCount();
							UpdateDebt();

							// post response
							std::string amount_str = std::to_string(debt.amount);
							std::string answer = "Congratz to the win! The B&B share is " + amount_str + " munies (!give @" + username + " " + amount_str + ")";
							SendChatMessage(answer);
						}
						else // over stake
						{
							SendChatMessage("Error: " + win->username + " your stake of " + std::to_string(stake) + " is bigger then your license allows (" + std::to_string(max_stake) + ")");
						}
					}
				}
				else if (GambleLooseResult *loss = boost::get<GambleLooseResult>(&out))
				{
					// check if user had a valid stake
					auto stake_iter = gamble_stakes.find(loss->username);
					if (stake_iter != gamble_stakes.end())
					{
						// calculate stake
						unsigned stake = loss->munies_changed;
						// check if stake is in the license
						unsigned max_stake = GetUserMaxStake(loss->username);
						if (stake <= max_stake)
						{
							if (gamble_debts.find(loss->username) == gamble_debts.end())
							{
								unsigned payout = static_cast<unsigned>(stake * stake_multiplier);
								std::string payout_str = std::to_string(payout);
								SendChatMessage("Thanks to B&B, you get " + payout_str + " munies!");
								SendChatMessage("!give @" + loss->username + " " + payout_str);
							}
							else
							{
								SendChatMessage("Error: still open debt for user " + loss->username);
							}
						}
						else // over stake
						{
							SendChatMessage("Error: " + loss->username + " your stake of " + std::to_string(stake) + " is bigger then your license allows (" + std::to_string(max_stake) + ")");
						}
					}
				}
				else if (GambleJackpotResult *jack = boost::get<GambleJackpotResult>(&out))
				{
					// check if user had a valid stake
					auto stake_iter = gamble_stakes.find(jack->username);
					if (stake_iter != gamble_stakes.end())
					{
						// calculate stake
						if (stake_iter->second.amount)
						{
							unsigned stake = *stake_iter->second.amount;
							// check if stake is in the license
							unsigned max_stake = GetUserMaxStake(jack->username);
							if (stake <= max_stake)
							{
								// calculate share
								unsigned share = jack->munies_changed - static_cast<unsigned>(stake * stake_multiplier);
								// put on debt
								auto &debt = gamble_debts[jack->username];
								debt.amount += share;
								debt.update_ticks = GetTickCount();
								UpdateDebt();

								// post response
								std::string amount_str = std::to_string(debt.amount);
								std::string answer = "Congratz to the win! The B&B share is " + amount_str + " munies (!give @" + username + " " + amount_str + ")";
								SendChatMessage(answer);
							}
							else // over stake
							{
								SendChatMessage("Error: " + jack->username + " your stake of " + std::to_string(stake) + " is bigger then your license allows (" + std::to_string(max_stake) + ")");
							}
						}
						else // a gamble all. thats a problem
						{
							SendChatMessage("PANIC! User " + jack->username + " won the jackpot with a \"gamble all\". No way of determining stake. Manual intervention needed!");
						}
					}
				}
			}

			// Rolled 33.
			// Rolled 100.

			// wenn stake größer als lizenz: "Your last gamble was over your insured amount of <x>. Invalid gamble"
			// wenn verlust und nicht auf der debt liste, auszahlen. "Even with a lost gamble, thanks to the safe gambling system you always win!"
			// wenn verlust und auf der debt liste: "Invalid gamble. User "<username>" still in debt
			// wenn gewinn: "looks like you won! Please send a share of <amount> to <username> (with !give)
		}
		else if (StartsWith<ComparePolicy::CaseInsensitive>(message, "@"s)) // check for timeout: @Gotbread you can't gamble, you are on a cooldown for 70 seconds!
		{
			GambleTimeout out;
			bool res = ParseGambleTimeout(message, out);
			if (res)
			{
				if (boost::iequals(out.username, username)) // myself
				{
					if (running)
					{
						ticks_remaining = 1000 * (out.timer + 3); // 3 seconds extra
					}
				}
				else
				{
					gamble_stakes.erase(out.username);
				}
			}
			// @user you can't gamble
			// für mich selber: retry in x sekunden
			// für andere: aus den stakes entfernen
		}
		else
		{
			GiveCommand out;
			bool res = ParseGiveCommand(message, out);
			if (res)
			{
				auto debt_iter = gamble_debts.find(out.from);
				if (debt_iter != gamble_debts.end() && boost::iequals(out.to, username))
				{
					if (debt_iter->second.amount > out.amount)
					{
						debt_iter->second.amount -= out.amount;
						debt_iter->second.update_ticks = GetTickCount();
						SendChatMessage(out.from + " payed " + std::to_string(out.amount) + " and still has to pay " + std::to_string(debt_iter->second.amount) + " munies");
					}
					else
					{
						gamble_debts.erase(debt_iter);
						SendChatMessage(out.from + " payed the share. Thanks for using the B&B system!");
					}
					UpdateDebt();
				}
			}
		}
	}
	else if (boost::iequals(user, username)) // message to ourself
	{
		boost::variant<BotCommandSetLicense, BotCommandSetDefaultLicense, BotCommandGetLicense, BotCommandGetAllLicenses> out;
		bool res = ParseBotCommand(message, out);
		if (res)
		{
			if (BotCommandSetLicense *license = boost::get<BotCommandSetLicense>(&out))
			{
				gamble_licenses[license->username] = license->amount;
				SaveConfigFile();
				SendChatMessage("license for user \"" + license->username + "\" has been set to " + std::to_string(license->amount) + " munies");
			}
			else if (BotCommandSetDefaultLicense *license = boost::get<BotCommandSetDefaultLicense>(&out))
			{
				default_gamble_license = license->amount;
				SendChatMessage("default license has been set to " + std::to_string(license->amount) + " munies");
			}
			else if (BotCommandGetLicense *license = boost::get<BotCommandGetLicense>(&out))
			{
				auto iter = gamble_licenses.find(license->username);
				if (iter != gamble_licenses.end())
				{
					SendChatMessage("user \"" + license->username + "\" has a license for " + std::to_string(iter->second) + " munies");
				}
				else
				{
					SendChatMessage("user \"" + license->username + "\" has no license yet!");
				}
			}
			else if (BotCommandGetAllLicenses *license = boost::get<BotCommandGetAllLicenses>(&out))
			{
				std::string str;
				for (auto &entry : gamble_licenses)
					str += entry.first + " -> " + std::to_string(entry.second) + ". ";

				str += "everyone else has a limit of " + std::to_string(default_gamble_license);

				SendChatMessage(str);
			}
		}
		// gamblebot set default license <limit>
		// gamblebot set license username <limit>
		// gamblebot get license unsername
		// gamblebot get all licenses
	}
	else // regular message
	{
		AddLogMessage(hChat, user + ": " + message, 10000);
	}
}
void MainWindow::RegisterClass(HINSTANCE hInstance)
{
	BaseWindow::RegisterClass(hInstance, classname, CreateSolidBrush(WindowColor), 0);
}
std::string FormatTime(unsigned ticks)
{
	std::stringstream stream;

	unsigned minutes = (ticks / 1000) / 60;
	unsigned seconds = (ticks / 1000) % 60;

	stream << std::right << std::setw(2) << std::setfill('0') << minutes << ":" << std::right << std::setw(2) << std::setfill('0') << seconds;

	return stream.str();
}
LRESULT MainWindow::WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_COMMAND:
		{
			HWND hCommand = reinterpret_cast<HWND>(lParam);
			if (hCommand == hConnect)
			{
				if (connected)
				{
					chat->Post([&] {chat->Disconnect(); });
					OnChatDisconnected();
				}
				else
				{
					username = GetWindowTextString(hUsername);
					oauth_token = GetWindowTextString(hToken);
					channel = GetWindowTextString(hChannel);

					chat->Post([&]
					{
						chat->SetUser(username);
						chat->SetPass(oauth_token);
						chat->Connect();
					});
					OnChatConnecting();
				}
			}
			else if (hCommand == hStart)
			{
				if (running)
				{
					running = false;
					refresh_once = true;
					ticks_remaining = initial_ticks;
					SetWindowText(hStart, "Start");
				}
				else if (connected)
				{
					running = true;
					ticks_remaining = initial_ticks;
					SetWindowText(hStart, "Stop");
				}
				else
				{
					MessageBox(hWnd, "Must be connected before starting", 0, MB_ICONERROR);
				}
			}
		}
		break;
	case WM_CTLCOLORSTATIC:
		{
			HWND hWnd = reinterpret_cast<HWND>(lParam);
			if (hWnd == hDebtLabel || hWnd == hChatLabel)
			{
				HDC hdc = reinterpret_cast<HDC>(wParam);
				SetBkMode(hdc, TRANSPARENT);
				return reinterpret_cast<LRESULT>(white_background.GetBrush());
			}
		}
		break;
	case WM_TIMER:
		OnTimer();
		break;
	case WM_PAINT:
		{
			HDCWrapper hdc = HDCWrapper::BeginPaint(hWnd);
			GDIObject font = CreateFont(65, 0, 0, 0, 500, 0, 0, 0, 0, 0, 0, 0, 0, "Consolas");
			hdc.Select(font);

			auto str = FormatTime(ticks_remaining);
			DrawText(hdc, str.c_str(), -1, &TimerRect, 0);
		}
		break;
	}
	return DefWindowProc(hWnd, Msg, wParam, lParam);
}
void MainWindow::LoadConfigFile()
{
	std::fstream file("config.tbc", std::ios::in);
	if (!file.is_open())
		return;

	nlohmann::json j;

	try
	{
		file >> j;
		username = j.at("username");
		oauth_token = j.at("oauth_token");
		channel = j.at("channel");
	}
	catch (std::exception &)
	{
		return;
	}

	SetWindowText(hUsername, username.c_str());
	SetWindowText(hToken, oauth_token.c_str());
	SetWindowText(hChannel, channel.c_str());

	file.close();
	file.open("licenses.tbl", std::ios::in);
	if (!file.is_open())
		return;

	try
	{
		file >> j;

		default_gamble_license = j.at("default_gamble_license");
		gamble_licenses = j.at("licenses").get<decltype(gamble_licenses)>();
	}
	catch (std::exception &)
	{
		return;
	}

	return;
}
void MainWindow::SaveConfigFile()
{
	std::fstream file("config.tbc", std::ios::out);
	if (!file.is_open())
		return;

	nlohmann::json j;
	j["username"] = username;
	j["oauth_token"] = oauth_token;
	j["channel"] = channel;

	try
	{
		file << j;
	}
	catch (std::exception &)
	{
		return;
	}

	file.close();
	file.open("licenses.tbl", std::ios::out);
	if (!file.is_open())
		return;

	j.clear();
	j["default_gamble_license"] = default_gamble_license;
	j["licenses"] = gamble_licenses;

	try
	{
		file << j;
	}
	catch (std::exception &)
	{
		return;
	}

	return;
}
void MainWindow::SendChatMessage(const std::string &str)
{
	if (chat)
	{
		chat->Post([this, str, ch = this->channel]
		{
			chat->SendMessage("PRIVMSG #" + ch + " :" + str);
		});
	}
}
void MainWindow::JoinChannel()
{
	if (chat)
	{
		chat->Post([this, ch = this->channel]
		{
			chat->SendMessage("JOIN #" + ch);
		});
	}
}
void MainWindow::OnTimer()
{
	unsigned current_ticks = GetTickCount();
	unsigned diff = current_ticks - last_ticks;
	last_ticks = current_ticks;

	if (connected && running)
	{
		if (ticks_remaining > diff)
			ticks_remaining -= diff;
		else
			ticks_remaining = 0;

		if (!ticks_remaining)
		{
			ticks_remaining = initial_ticks;
			SendChatMessage("!gamble 100");
		}
	}
	if (running || refresh_once)
	{
		InvalidateRect(hWnd, &TimerRect, 0);
		refresh_once = false;
	}
	// remove old stakes
	for (auto iter = gamble_stakes.begin(); iter != gamble_stakes.end();)
	{
		if (iter->second.creation_ticks + stake_lifetime < current_ticks)
		{
			iter = gamble_stakes.erase(iter);
		}
		else
		{
			++iter;
		}
	}
	unsigned reminder_ticks = 5 * 60 * 1000; // 5 mins
	for (auto &entry : gamble_debts)
	{
		if (entry.second.update_ticks + reminder_ticks < current_ticks)
		{
			entry.second.update_ticks += reminder_ticks;
			auto amount_str = std::to_string(entry.second.amount);
			SendChatMessage("Bot reminder: " + entry.first + " still has an open debt of " + amount_str + " munies! (!give " + username + " " + amount_str + ")");
		}
	}
}
void MainWindow::UpdateDebt()
{
	std::string str;
	for (auto &entry : gamble_debts)
	{
		if (!str.empty())
			str += "\r\n";
		str += entry.first + ": " + std::to_string(entry.second.amount);
	}
	SetWindowText(hDebt, str.c_str());
}

const char *MainWindow::classname = "MainWindow";