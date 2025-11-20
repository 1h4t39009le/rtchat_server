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
private:
    friend class RoomManager;
    friend class InChatSession;
    void add_session(std::shared_ptr<InChatSession> session) {
        std::lock_guard lock(m_mutex);
        m_sessions.push_back(std::move(session));
    }
    std::mutex m_mutex;
    std::vector<std::weak_ptr<InChatSession>> m_sessions{};
};

class InChatSession : public std::enable_shared_from_this<InChatSession> {
public:
    using Connection = beast::websocket::stream<beast::tcp_stream>;
    InChatSession(Connection connection, Room &room)
        : m_connection(std::move(connection)), m_room(room)
    {
        m_room.add_session(shared_from_this());
    }

    InChatSession(const InChatSession&) = delete;
    InChatSession& operator=(const InChatSession&) = delete;

    InChatSession(InChatSession&&) = delete;
    InChatSession& operator=(InChatSession&&) = delete;

    net::awaitable<void> run(){
        co_return;
    }

private:
    Room& m_room; // room outlives session
    Connection m_connection;
};

class RoomManager {
public:
    using RoomCode = std::size_t;
    Room &create_empty_room() {
        std::lock_guard lock(m_mutex);
        return m_rooms.emplace(m_id_counter++, Room()).first->second;
    }
    std::optional<std::reference_wrapper<Room>> get_room(RoomCode code) {
        std::lock_guard guard(m_mutex);
        auto it = m_rooms.find(code);
        if (it != m_rooms.end()) {
            return it->second;
        }
        return std::nullopt;
    }
private:
    std::mutex m_mutex;
    std::size_t m_id_counter;
    std::map<RoomCode, Room> m_rooms;
};

class Server{
public:
    using tcp = net::ip::tcp;
    using WebSocket = beast::websocket::stream<beast::tcp_stream>;
    net::awaitable<std::pair<WebSocket, Room*>> prepare_session(WebSocket ws){
        // check client json messages
        // join or create here
        co_return std::pair(std::move(ws), &m_room_manager.create_empty_room());
    }
    net::awaitable<void> listener(net::ip::port_type port){
        auto executor = co_await net::this_coro::executor;
        tcp::acceptor acceptor(executor, tcp::endpoint(tcp::v4(), port));
        for(;;){
            auto [ws, room_ptr] = co_await prepare_session(
                WebSocket{co_await acceptor.async_accept(net::use_awaitable)}
            );
            auto in_chat_session = std::make_shared<InChatSession>(std::move(ws), *room_ptr);
            net::co_spawn(
                executor,
                in_chat_session->run(),
                net::detached
                );
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
            server.listener(port),
            net::detached
            );
        ioc.run();
    }
private:
    Server() = default;
    RoomManager m_room_manager;
};

int main() {
    Server::start(8888, 4);
}
