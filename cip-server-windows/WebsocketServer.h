#pragma once
#include <set>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using websocketpp::lib::mutex;
using websocketpp::lib::condition_variable;

typedef websocketpp::server<websocketpp::config::asio> server;

enum action_type {
	SUBSCRIBE,
	UNSUBSCRIBE,
	MESSAGE
};

struct action {
	action(action_type t, connection_hdl h) : type(t), hdl(h) {}
	action(action_type t, connection_hdl h, server::message_ptr m)
		: type(t), hdl(h), msg(m) {}

	action_type type;
	websocketpp::connection_hdl hdl;
	server::message_ptr msg;
};

class WebsocketServer
{
public:
	WebsocketServer() {
		// Initialize Asio Transport
		m_server.init_asio();

		// Register handler callbacks
		m_server.set_open_handler(bind(&WebsocketServer::on_open, this, ::_1));
		m_server.set_close_handler(bind(&WebsocketServer::on_close, this, ::_1));
		m_server.set_message_handler(bind(&WebsocketServer::on_message, this, ::_1, ::_2));
	};
	void run(uint16_t);
	void on_open(connection_hdl hdl);
	void on_close(connection_hdl hdl);
	void on_message(connection_hdl hdl, server::message_ptr);
	void broadcast();
private:
	typedef std::set<connection_hdl, std::owner_less<connection_hdl> > con_list;

	server m_server;
	con_list m_connections;
	std::queue<action> m_actions;
	mutex m_connection_lock;
	mutex m_action_lock;
	int size;
};