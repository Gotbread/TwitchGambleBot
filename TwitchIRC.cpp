#include "TwitchIRC.h"
#include "MainWindow.h"
#include "Parser.h"

#pragma comment(lib, "libcrypto.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "crypt32.lib")

using std::string;
template<class T>
using shared_ptr = std::shared_ptr<T>;

TwitchIRC::TwitchIRC(string server_url, std::shared_ptr<io_service> service) : client(std::move(server_url), false)
{
	if (service)
		client.io_service = service;

	using namespace std::placeholders;

	client.on_open = std::bind(&TwitchIRC::OnOpen, this, _1);
	client.on_close = std::bind(&TwitchIRC::OnClose, this, _1, _2, _3);
	client.on_error = std::bind(&TwitchIRC::OnError, this, _1, _2);
	client.on_message = std::bind(&TwitchIRC::OnMessage, this, _1, _2);
}
void TwitchIRC::Connect()
{
	client.start();
}
void TwitchIRC::Disconnect()
{
	if (client_connection)
		client_connection->send_close(1000);

	client.stop();
}
void TwitchIRC::Exit()
{
	client.io_service->stop();
}
void TwitchIRC::SetUser(string username)
{
	this->username = std::move(username);
}
void TwitchIRC::SetPass(string oauth_token)
{
	this->oauth_token = std::move(oauth_token);
}
void TwitchIRC::SendMessage(string message)
{
	if (client_connection)
		client_connection->send(message);
}
bool TwitchIRC::IsConnected() const
{
	return client_connection != nullptr;
}
void TwitchIRC::OnOpen(shared_ptr<WssClient::Connection> connection)
{
	client_connection = connection;

	connection->send("CAP REQ :twitch.tv/tags twitch.tv/commands");
	connection->send(string("PASS ") + "oauth:" + oauth_token);
	connection->send(string("NICK ") + username);
	connection->send(string("USER ") + username + "8 * :" + username);

	MainWindow::RPC(&MainWindow::OnChatConnected);
}
void TwitchIRC::OnClose(shared_ptr<WssClient::Connection> /*connection*/, int status, const string & /*reason*/)
{
	client_connection.reset();

	MainWindow::RPC(&MainWindow::OnChatDisconnected);
}
void TwitchIRC::OnError(shared_ptr<WssClient::Connection> /*connection*/, const SimpleWeb::error_code &ec)
{
	client_connection.reset();

	MainWindow::RPC(&MainWindow::OnChatDisconnected);
}
std::string GetProperty(const std::vector<TwitchMessage::Property> &properties, const std::string &key)
{
	for (const auto &entry : properties)
	{
		if (entry.first == key)
			return entry.second;
	}
	return std::string();
}
void RemoveBackwards(std::string &str, const std::string &toremove)
{
	for (;;)
	{
		size_t pos = str.find_first_of(toremove);
		if (pos + toremove.length() == str.length())
			str.erase(pos);
		else
			break;
	}
}
void TwitchIRC::OnMessage(shared_ptr<WssClient::Connection> connection, shared_ptr<WssClient::InMessage> in_message)
{
	boost::variant<TwitchMessage, TwitchPing> out;

	auto str = in_message->string();
	RemoveBackwards(str, "\r\n");
	RemoveBackwards(str, "\n");
	//OutputDebugString((str + "\n\n").c_str());
	bool res = ParseMessage(str, out);
	if (res)
	{
		if (TwitchMessage *msg = boost::get<TwitchMessage>(&out))
		{
			auto displayname = GetProperty(msg->properties, "display-name");
			MainWindow::RPC(&MainWindow::OnChatMessage, displayname, msg->message);
		}
		else if (TwitchPing *ping = boost::get<TwitchPing>(&out))
		{
			SendMessage("PONG");
		}
	}
}