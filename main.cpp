#include "TwitchIRC.h"
#include "MainWindow.h"

#include <Windows.h>

#include "Parser.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, char *, int)
{
	std::string server_url = "irc-ws.chat.twitch.tv";

	MainWindow::RegisterClass(hInstance);

	auto service = std::make_shared<boost::asio::io_service>();
	boost::asio::io_service::work work(*service);

	TwitchIRC twitch(server_url, service);

	MainWindow mainwindow(hInstance);
	mainwindow.SetChat(&twitch);

	std::thread thread([&]() { service->run(); });

	ThreadManager::Run();
	ThreadManager::Wait();
	thread.join();

	return 0;
}
