#pragma once

#include "TwitchIRC.h"
#include "BaseWindow.h"
#include "ThreadCommunication.h"
#include "WindowsMessageQueue.h"
#include "GDIHelper.h"
#include "Parser.h"

#include <map>

class MainWindow : public BaseWindow, public BaseThread<MainWindow, WindowsMessageQueue<MainWindow>>
{
public:
	MainWindow(HINSTANCE hInstance);

	bool OnInit() override;
	void OnStart() override;
	void OnExit() override;
	void OnOtherEvent(Queue::Reason reason) override;

	void OnChatConnecting();
	void OnChatConnected();
	void OnChatDisconnected();

	void OnChatMessage(std::string username, std::string message);

	void SetChat(TwitchIRC *chat);

	static void RegisterClass(HINSTANCE hInstance);

	LRESULT WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) override;
private:
	void LoadConfigFile();
	void SaveConfigFile();
	void SendChatMessage(const std::string &str);
	void JoinChannel();
	void OnTimer();
	void UpdateDebt();

	HWND hUsername, hToken, hChannel;
	HWND hConnect;
	HWND hStart;
	HWND hDebt, hDebtLabel;
	HWND hChat, hChatLabel;
	RECT TimerRect;

	GDIObject edit_font;
	GDIObject white_background;

	std::string username, oauth_token, channel;

	bool connected = false;
	bool running = false;
	bool refresh_once = false;

	TwitchIRC *chat;

	struct Stake
	{
		unsigned creation_ticks; // when the stake was created
		GambleAmount amount;
	};
	struct Debt
	{
		unsigned update_ticks; // when the debt was changed
		unsigned amount = 0;
	};
	std::map<std::string, Stake> gamble_stakes;
	std::map<std::string, Debt> gamble_debts;
	std::map<std::string, unsigned> gamble_licenses;
	unsigned default_gamble_license = 5000;
	unsigned stake_lifetime = 10 * 1000; // 10 seconds

	unsigned initial_ticks;
	unsigned ticks_remaining;
	unsigned last_ticks;

	static constexpr unsigned WindowWidth = 600;
	static constexpr unsigned WindowHeight = 500;
	static constexpr unsigned WindowColor = RGB(255, 255, 255);

	static const char *classname;
};