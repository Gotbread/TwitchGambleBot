#pragma once

#pragma warning(push)
#pragma warning(disable : 4309)
#include <client_wss.hpp>
#pragma warning(pop)

class TwitchIRC
{
public:
	using io_service = boost::asio::io_service;
	using WssClient = SimpleWeb::SocketClient<SimpleWeb::WSS>;

	TwitchIRC(std::string server, std::shared_ptr<io_service> service = nullptr);

	template<typename T>
	void Post(T &&handler)
	{
		client.io_service->post(handler);
	}

	void Connect();
	void Disconnect();

	void Exit();

	void SetUser(std::string username);
	void SetPass(std::string oauth_token);

	void SendMessage(std::string message);

	bool IsConnected() const;
private:
	void OnOpen(std::shared_ptr<WssClient::Connection> connection);
	void OnClose(std::shared_ptr<WssClient::Connection> /*connection*/, int status, const std::string & /*reason*/);
	void OnError(std::shared_ptr<WssClient::Connection> /*connection*/, const SimpleWeb::error_code &ec);
	void OnMessage(std::shared_ptr<WssClient::Connection> connection, std::shared_ptr<WssClient::InMessage> in_message);

	std::string username, oauth_token;

	WssClient client;
	std::shared_ptr<WssClient::Connection> client_connection;
};