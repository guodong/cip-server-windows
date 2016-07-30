#include "stdafx.h"
#include <websocketpp/config/asio_no_tls.hpp>

#include <websocketpp/server.hpp>
#include <set>

#include "WebsocketServer.h"
#include "cip_protocol.h"
#include "cip_window.h"

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
	} catch (const std::exception & e) {
		std::cout << e.what() << std::endl;
	}
}

extern map<int, cip_window_t*> windows;

void WebsocketServer::on_open(connection_hdl hdl) {

	lock_guard<mutex> guard(m_connection_lock);
	m_connections.insert(hdl);
	/*map<int, cip_window_t*>::iterator it;
	for (it = windows.begin(); it != windows.end(); it++) {
		cip_event_window_create_t *cewc = (cip_event_window_create_t*)malloc(sizeof(cip_event_window_create_t));
		cewc->type = CIP_EVENT_WINDOW_CREATE;
		cewc->wid = it->second->wid;
		cewc->x = it->second->x;
		cewc->y = it->second->y;
		cewc->width = it->second->width;
		cewc->height = it->second->height;
		cewc->bare = 1;
		m_server.send(hdl, cewc, sizeof(cip_event_window_create_t), websocketpp::frame::opcode::value::binary);
		if (it->second->visible) {
			cip_event_window_show_t *cews = (cip_event_window_show_t*)malloc(sizeof(cip_event_window_show_t));
			cews->type = CIP_EVENT_WINDOW_SHOW;

			cews->wid = it->second->wid;
			cews->bare = 1;
			m_server.send(hdl, cews, sizeof(cip_event_window_show_t), websocketpp::frame::opcode::value::binary);
			cip_window_frame_send(it->second, 1);
		}

	}*/
}

void WebsocketServer::on_close(connection_hdl hdl) {
	lock_guard<mutex> guard(m_connection_lock);
	m_connections.erase(hdl);
}

void WebsocketServer::on_message(connection_hdl hdl, server::message_ptr msg) {
	lock_guard<mutex> guard(m_action_lock);
	const char *data = msg->get_payload().data();
	switch (data[0]) {
	case CIP_EVENT_MOUSE_MOVE: {
		cip_event_mouse_move_t *cemm = (cip_event_mouse_move_t*)data;
		SetCursorPos(cemm->x, cemm->y);
		break;
	}
	case CIP_EVENT_MOUSE_DOWN: {
		cip_event_mouse_down_t *cemd = (cip_event_mouse_down_t*)data;
		INPUT input;
		input.type = INPUT_MOUSE;
		if (cemd->code == 1) {
			input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		} else if (cemd->code == 3) {
			input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
		}
		SendInput(1, &input, sizeof(input));
		break;
	}
	case CIP_EVENT_MOUSE_UP: {
		cip_event_mouse_up_t *cemd = (cip_event_mouse_up_t*)data;
		INPUT input;
		input.type = INPUT_MOUSE;
		if (cemd->code == 1) {
			input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
		} else if (cemd->code == 3) {
			input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
		}
		SendInput(1, &input, sizeof(input));
		break;
	}
	default:
		break;
	}
}

void WebsocketServer::broadcast(std::string const & payload, int op)
{
	/*FILE *f = fopen("C:\\debug.txt", "a");
	fprintf(f, "broadcast sz %d %d\n", m_connections.size(), size);
	fclose(f);*/
	for (auto it : m_connections) {
		//MessageBox(NULL, L"f", L"a", MB_OK);
		//std::cout << "look" << std::endl;
		m_server.send(it, payload, (websocketpp::frame::opcode::value)op);
	}
}

void WebsocketServer::broadcast(void const *payload, size_t len, int op)
{
	for (auto it : m_connections) {
		//MessageBox(NULL, L"f", L"a", MB_OK);
		//std::cout << "look" << std::endl;
		m_server.send(it, payload, len, (websocketpp::frame::opcode::value)op);
	}
}

void WebsocketServer::quit()
{
	delete &m_server;
}