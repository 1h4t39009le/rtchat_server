#pragma once
#include "common.hpp"
#include "room_manager.hpp"
class RoomMember;

class Server{
public:
    using tcp = net::ip::tcp;
    using WebSocket = beast::websocket::stream<beast::tcp_stream>;

    net::awaitable<void> send_bad_response(
        beast::tcp_stream stream,
        const http::request<http::string_body> &req,
        http::status status,
        std::string body_text
    );
    net::awaitable<void> handle_create_route(beast::tcp_stream stream,http::request<http::string_body> req,std::string name);
    net::awaitable<void> handle_join_route(
        beast::tcp_stream stream,
        http::request<http::string_body> req,
        std::string name,
        std::string room_code
    );

    net::awaitable<void> run_session(tcp::socket socket);
    net::awaitable<void> listener(net::ip::port_type port);

    static void start(net::ip::port_type port, std::size_t num_threads);
private:
    Server() = default;
    RoomManager m_room_manager;
};
