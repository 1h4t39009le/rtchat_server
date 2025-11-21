#include <iostream>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include "json/json_impl.hpp"

#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;

class InChatSession;

class Room {
public:
    using Code = std::size_t;
    Room(Code code) 
        : m_code(code) {
    }
    Code get_code() {
        return m_code;
    }
private:
    friend class RoomManager;
    friend class InChatSession;
    std::size_t add_session(std::shared_ptr<InChatSession> session) {
        std::lock_guard lock(m_mutex);
        auto id = m_id_counter++;
        m_sessions.emplace(id, std::move(session));
        return id;
    }
    std::mutex m_mutex;
    Code m_code;
    std::size_t m_id_counter{};
    std::unordered_map<std::size_t, std::weak_ptr<InChatSession>> m_sessions{};
};

bool session_ended(const boost::system::error_code& ec) {
    return
        ec == websocket::error::closed ||
        ec == net::error::connection_reset ||
        ec == net::error::eof
        ;
}

class InChatSession : public std::enable_shared_from_this<InChatSession> {
public:
    using Connection = beast::websocket::stream<beast::tcp_stream>;
    InChatSession(Connection connection, std::shared_ptr<Room> room)
        : m_connection(std::move(connection)), m_room(room){
    }
    InChatSession(const InChatSession&) = delete;
    InChatSession& operator=(const InChatSession&) = delete;

    net::awaitable<void> run(){
        auto room = m_room.lock();
        if (!room) co_return;
        auto self = shared_from_this();
        auto id = room->add_session(self);
        beast::flat_buffer buffer;
        try {
            for (;;) {
                co_await m_connection.async_write(net::buffer("Hi"), net::use_awaitable);
                co_await m_connection.async_read(buffer, net::use_awaitable);
                buffer.consume(buffer.size());
            }
        }
        catch (const boost::system::system_error& e) {
            if (session_ended(e.code())) {
                std::cout << std::format("Session ended\n");
            } else throw;
        }
    }
private:
    std::weak_ptr<Room> m_room; // room outlives session
    Connection m_connection;
};

class RoomManager {
public:
    
    std::shared_ptr<Room> create_empty_room() {
        std::lock_guard lock(m_mutex);
        auto code = m_id_counter++;
        return m_rooms.emplace(code, std::make_shared<Room>(code)).first->second;
    }
    std::optional<std::shared_ptr<Room>> get_room(Room::Code code) {
        std::lock_guard lock(m_mutex);
        auto it = m_rooms.find(code);
        if (it != m_rooms.end()) {
            return it->second;
        }
        return std::nullopt;
    }
private:
    std::mutex m_mutex;
    std::size_t m_id_counter;
    std::unordered_map<Room::Code, std::shared_ptr<Room>> m_rooms;
};





class Server{
public:
    using tcp = net::ip::tcp;
    using WebSocket = beast::websocket::stream<beast::tcp_stream>;

    auto prepare_session(WebSocket ws)
        -> net::awaitable<std::optional<std::shared_ptr<InChatSession>>>
    {
        std::cout << std::format("Preparing session with {}\n", ws.next_layer().socket().remote_endpoint().address().to_string());
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

            if (client_msg) {
                switch (client_msg->action) {
                    case ClientPrepareMessageAction::Create: {
                        co_return std::make_shared<InChatSession>(std::move(ws), m_room_manager.create_empty_room());
                    }
                    case ClientPrepareMessageAction::Join:
                        break;
                }
            }
            else {
                nlohmann::json error = { {"error", "Invalid message format"} };
                co_await ws.async_write(net::buffer(error.dump()), net::use_awaitable);
                co_await ws.async_close(websocket::close_code::unknown_data, net::use_awaitable);
            }
        }
        catch (const boost::system::system_error &e) {
            if (!session_ended(e.code())) {
                std::cerr << std::format("Error when preparing session {}\n", e.what());
            }
        }
        co_return std::nullopt;
    }

    net::awaitable<void> listener(net::ip::port_type port){
        std::cout << std::format("Server is listening port {}\n", port);
        auto executor = co_await net::this_coro::executor;
        tcp::acceptor acceptor(executor, tcp::endpoint(tcp::v4(), port));
        for(;;){
            if (auto in_chat_session = co_await prepare_session(WebSocket{ co_await acceptor.async_accept(net::use_awaitable) })) {
                net::co_spawn(
                    executor,
                    (*in_chat_session)->run(),
                [](std::exception_ptr e) {
                    if (e) {
                        try { std::rethrow_exception(e); }
                        catch (std::exception& e) {
                            std::cerr << "Error in session: " << e.what() << "\n";
                        }
                    }
                });
            }
        }
    }
    static void start(net::ip::port_type port, std::size_t num_threads){
        Server server;
        net::io_context ioc(num_threads);
        std::vector<std::jthread> runners;
        runners.reserve(num_threads-1);
        for(int i{};i<num_threads-1;++i){
            runners.emplace_back([&ioc]{ioc.run();});
        }
        net::co_spawn(
            ioc,
            server.listener(port), // SINGLE_LISTENER_SERVER 1
            [](std::exception_ptr e) {
                if (e) {
                    try { std::rethrow_exception(e); }
                    catch (std::exception& e) {
                        std::cerr << "Error in listener: " << e.what() << "\n";
                    }
                }
            });
        ioc.run();
    }
private:
    Server() = default;
    RoomManager m_room_manager;
};



int main() {
    Server::start(8888, 4);
}
