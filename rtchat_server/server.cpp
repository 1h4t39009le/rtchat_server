#include "server.hpp"
#include "json/json_impl.hpp"
#include "room_member.hpp"
#include <iostream>
#include <memory>
#include <thread>

auto Server::prepare_session(WebSocket ws)
    -> net::awaitable<std::optional<std::shared_ptr<RoomMember>>>
{
    std::cout << std::format("Preparing session with {}\n", ws.next_layer().socket().remote_endpoint().address().to_string());
    auto send_error = [&](ServerPrepareError err_code) -> net::awaitable<void> {
        nlohmann::json error_msg = ServerPrepareResponse{.error = err_code};
        try {
            co_await ws.async_write(net::buffer(error_msg.dump()), net::use_awaitable);
            co_await ws.async_close(websocket::close_code::policy_error, net::use_awaitable);
        } catch(...) {}
    };
    try {
        co_await ws.async_accept(net::use_awaitable);
        beast::flat_buffer buffer;

        co_await ws.async_read(buffer, net::use_awaitable);
        std::optional<ClientPrepareMessage> client_msg;
        try {
            client_msg = nlohmann::json::parse(beast::buffers_to_string(buffer.data())).get<ClientPrepareMessage>();
            buffer.consume(buffer.size());
        }
        catch (...) {}

        if (!client_msg) {
            co_await send_error(ServerPrepareError::InvalidJson);
            co_return std::nullopt;
        }

        switch (client_msg->action) {
        case ClientPrepareAction::Create: {
            co_return std::make_shared<RoomMember>(client_msg->name, std::move(ws), m_room_manager.create_empty_room());
        }
        case ClientPrepareAction::Join: {
            if (!client_msg->room_code){
                co_await send_error(ServerPrepareError::InvalidJson);
                co_return std::nullopt;
            }
            auto room = m_room_manager.get_room(*client_msg->room_code);
            if(!room){
                co_await send_error(ServerPrepareError::InvalidRoomCode);
                co_return std::nullopt;
            }
            co_return std::make_shared<RoomMember>(client_msg->name, std::move(ws), std::move(*room));
        }
        }
    }
    catch (const boost::system::system_error &e) {
        if (!session_ended(e.code())) {
            std::cerr << std::format("Error when preparing session {}\n", e.what());
        }
    }
    try{
        co_await ws.async_close(websocket::close_code::internal_error, net::use_awaitable);
    } catch(...) {}
    co_return std::nullopt;
}

net::awaitable<void> Server::run_session(WebSocket ws){
    auto session = co_await prepare_session(std::move(ws));
    if(session){
        co_await (*session)->run();
    }
}

net::awaitable<void> Server::listener(net::ip::port_type port){
    std::cout << std::format("Server is listening port {}\n", port);
    auto executor = co_await net::this_coro::executor;
    tcp::acceptor acceptor(executor, tcp::endpoint(tcp::v4(), port));
    for(;;){
        WebSocket ws(co_await acceptor.async_accept(net::make_strand(executor), net::use_awaitable));
        net::co_spawn(
            executor,
            run_session(std::move(ws)),
            LogOnCatch("session")
            );
    }
}
void Server::start(net::ip::port_type port, std::size_t num_threads){
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
