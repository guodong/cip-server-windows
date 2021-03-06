#include "stdafx.h"
#include <websocketpp/config/asio_no_tls.hpp>

#include <websocketpp/server.hpp>

#include <iostream>
#include <set>

#include "WebsocketServer.h"

using namespace std;

using websocketpp::lib::lock_guard;


void WebsocketServer::run(uint16_t port)
{
	// listen on specified port
	m_server.listen(port);

	// Start the server accept loop
	m_server.start_accept();

	// Start the ASIO io_service run loop
	try {
		m_server.run();
	}
	catch (const std::exception & e) {
		std::cout << e.what() << std::endl;
	}
}
void WebsocketServer::on_open(connection_hdl hdl) {
	{
		size = 9;
		lock_guard<mutex> guard(m_connection_lock);
		m_connections.insert(hdl);
		int s = m_connections.size();
		char str[10];
		sprintf(str, "%d", m_connections.size());
		m_server.send(hdl, str, websocketpp::frame::opcode::text);
	}
}

void WebsocketServer::on_close(connection_hdl hdl) {
	{
		lock_guard<mutex> guard(m_connection_lock);
		m_connections.erase(hdl);
	}
}

void WebsocketServer::on_message(connection_hdl hdl, server::message_ptr msg) {
	{
		lock_guard<mutex> guard(m_action_lock);
		m_actions.push(action(MESSAGE, hdl, msg));
	}
}

void WebsocketServer::broadcast()
{
	FILE *f = fopen("C:\\debug.txt", "a");
	fprintf(f, "broadcast sz %d %d\n", m_connections.size(), size);
	fclose(f);
	for (auto it : m_connections) {
		MessageBox(NULL, L"f", L"a", MB_OK);
		std::cout << "look" << std::endl;
		m_server.send(it, "aa", websocketpp::frame::opcode::text);
	}
}