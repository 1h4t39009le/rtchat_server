#include "server.hpp"
#include <boost/url.hpp>
#include "room/room_member.hpp"
#include <iostream>
#include <memory>
#include <thread>

net::awaitable<void> Server::send_bad_response(
    beast::tcp_stream stream,
    const http::request<http::string_body> &req,
    http::status status,
    std::string body_text
    ){
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "RTCHATServer");
    res.set(http::field::content_type, "text/plain");
    res.keep_alive(false);
    res.body() = std::move(body_text);
    res.prepare_payload();
    co_await http::async_write(stream, res, net::use_awaitable);
    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_send, ec);
}

net::awaitable<void> Server::handle_create_route(
    beast::tcp_stream stream,
    http::request<http::string_body> req,
    std::string name
    ){
    auto room_member = std::make_shared<RoomMember>(
        name,
        websocket::stream<beast::tcp_stream>(std::move(stream)),
        m_room_manager.create_empty_room());
    co_await room_member->run(std::move(req));
}

net::awaitable<void> Server::handle_join_route(
    beast::tcp_stream stream,
    http::request<http::string_body> req,
    std::string name,
    std::string room_code
    ){
    if(auto room = m_room_manager.get_room(room_code)){
        auto room_member = std::make_shared<RoomMember>(
            name,
            websocket::stream<beast::tcp_stream>(std::move(stream)),
            std::move(*room));
        co_await room_member->run(std::move(req));
    } else co_await send_bad_response(std::move(stream), req, http::status::not_found, "unknown room");
}

net::awaitable<void> Server::run_session(tcp::socket socket) {
    beast::tcp_stream stream(std::move(socket));
    http::request<http::string_body> req;
    beast::flat_buffer buffer;

    co_await http::async_read(stream, buffer, req, net::use_awaitable);
    if(!websocket::is_upgrade(req)) {
        co_return co_await send_bad_response(std::move(stream), req, http::status::upgrade_required, "websocket upgrade required");
    }

    std::cout << req.target() << '\n';
    boost::url_view url(req.target());
    std::string response_body;
    auto params = url.params();
    auto name_it = params.find("name"), room_code_it = params.find("room");
    if(name_it != params.end()){
        auto segment = url.segments().front();
        std::string name = (*name_it).value;
        if(segment == "create"){
            co_return co_await handle_create_route(std::move(stream), std::move(req), std::move(name));
        }else if(room_code_it != params.end() && segment == "join"){
            std::string room_code = (*room_code_it).value;
            co_return co_await handle_join_route(std::move(stream), std::move(req), std::move(name), std::move(room_code));
        }
    }
    co_await send_bad_response(std::move(stream), req, http::status::bad_request, "Invalid request or params");
}

net::awaitable<void> Server::listener(net::ip::port_type port){
    std::cout << std::format("Server is listening port {}\n", port);
    auto executor = co_await net::this_coro::executor;
    tcp::acceptor acceptor(executor, tcp::endpoint(tcp::v4(), port));
    for(;;){
        net::co_spawn(
            executor,
            run_session(co_await acceptor.async_accept(net::make_strand(executor), net::use_awaitable)),
            LogOnCatch("run_session")
            );
    }
}
void Server::start(net::ip::port_type port, int num_threads){
    Server server;
    net::io_context ioc(num_threads);
    std::vector<std::jthread> runners;
    runners.reserve(num_threads-1);
    net::co_spawn(
        ioc,
        server.listener(port),
        LogOnCatch("listener"));

    for(int i{};i<num_threads-1;++i){
        runners.emplace_back([&ioc]{ioc.run();});
    }
    ioc.run();
}
