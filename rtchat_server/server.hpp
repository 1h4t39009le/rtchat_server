#pragma once
#include "common.hpp"
#include "room_manager.hpp"
class RoomMember;

class Server{
public:
    using tcp = net::ip::tcp;
    using WebSocket = beast::websocket::stream<beast::tcp_stream>;

    auto prepare_session(WebSocket ws)
        -> net::awaitable<std::optional<std::shared_ptr<RoomMember>>>
    ;
    net::awaitable<void> run_session(WebSocket ws);
    net::awaitable<void> listener(net::ip::port_type port);

    static void start(net::ip::port_type port, std::size_t num_threads);
private:
    Server() = default;
    RoomManager m_room_manager;
};
