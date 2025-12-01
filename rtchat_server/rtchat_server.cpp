#include <iostream>
#include <deque>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include "json/json_impl.hpp"
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;

class RoomMember;

class LogOnCatch{
public:
    LogOnCatch(std::string source)
        :m_source(std::move(source)){
    }
    void operator()(std::exception_ptr e){
        if(e) try{
            std::rethrow_exception(e);
        }catch(std::exception &e){
            std::cerr << std::format("Error in {}:{}", m_source, e.what());
        }
    }
private:
    std::string m_source;
};
bool session_ended(const boost::system::error_code& ec) {
    return
        ec == websocket::error::closed ||
        ec == net::error::connection_reset ||
        ec == net::error::eof
        ;
}

using RoomCode = std::size_t;
class Room {
public:
    Room(RoomCode code, std::function<void(RoomCode)> on_leave)
        : m_on_leave(std::move(on_leave)), m_code(code) {
    }
    RoomCode get_code() {
        return m_code;
    }
    void sending(std::size_t id, const std::string &message){
        broadcast_message(ServerRoomMessage{ServerRoomAction::Sended, id, message});
    }
    void leaving(std::size_t id){
        {
            std::lock_guard lock(m_mutex);
            m_sessions.erase(id);
            m_is_dead = m_sessions.empty();
        }
        if(m_is_dead){
            return m_on_leave(m_code);
        }
        broadcast_message(ServerRoomMessage{ServerRoomAction::Leaved, id});
    }
private:
    void broadcast_message(const nlohmann::json &data, std::optional<std::size_t> exclude_id = std::nullopt);
    friend class RoomManager;
    friend class RoomMember;
    std::optional<std::size_t> add_session(std::shared_ptr<RoomMember> session) {
        std::size_t id;
        {
            std::lock_guard lock(m_mutex);
            if(m_is_dead) return std::nullopt;
            id = m_id_counter++;
            m_sessions.emplace(id, std::move(session));
        }
        broadcast_message(ServerRoomMessage{ServerRoomAction::Joined, id}, id);
        return id;
    }
    std::mutex m_mutex;
    std::function<void(RoomCode)> m_on_leave;
    bool m_is_dead = false;
    RoomCode m_code;
    std::size_t m_id_counter{};
    std::unordered_map<std::size_t, std::weak_ptr<RoomMember>> m_sessions{};
};


class RoomMember : public std::enable_shared_from_this<RoomMember> {
public:
    using Connection = beast::websocket::stream<beast::tcp_stream>;
    RoomMember(Connection connection, std::shared_ptr<Room> room)
        : m_connection(std::move(connection)), m_room(room){
    }
    RoomMember(const RoomMember&) = delete;
    RoomMember& operator=(const RoomMember&) = delete;

    void deliver(const std::string &message){
        send_impl(message);
    }
    void close_with_message(const std::string &message) {
        send_impl(message, true);
    }

    void send_impl(const std::string &message, bool close_after = false){
        net::post(
            m_connection.get_executor(),
            [self = shared_from_this(), message = std::move(message), close_after]{
                self->m_write_queue.push_back(message);
                if(close_after){
                    self->m_should_close = true;
                }
                if(!self->m_is_writing){
                    self->m_is_writing = true;
                    net::co_spawn(
                        self->m_connection.get_executor(),
                        self->write_loop(std::move(self)),
                        LogOnCatch("session write_loop")
                        );
                }
            });
    }

    net::awaitable<void> run(){
        auto room = m_room.lock();
        if (!room) co_return;
        auto self = shared_from_this();
        auto room_code = room->get_code();
        auto id_opt = room->add_session(self);
        if(!id_opt) {
            close_with_message(nlohmann::json(ServerPrepareResponse{.error = ServerPrepareError::InvalidRoomCode}).dump());
            co_return;
        }
        m_id = *id_opt;
        // session starts here
        try {
            beast::flat_buffer buffer;
            nlohmann::json start_response = ServerPrepareResponse{.client_id = m_id, .room_code = room_code};

            deliver(start_response.dump());
            for (;;) {
                co_await m_connection.async_read(buffer, net::use_awaitable);
                room->sending(m_id, beast::buffers_to_string(buffer.data()));
                buffer.consume(buffer.size());
            }
        }
        catch (const boost::system::system_error& e) {
            if (session_ended(e.code())) {
                std::cout << std::format("Session ended\n");

            } else throw;
        }
        room->leaving(m_id);
    }
private:
    net::awaitable<void> write_loop(std::shared_ptr<RoomMember> self){
        try{
            while(!m_write_queue.empty()){
                const auto &msg = m_write_queue.front();
                co_await m_connection.async_write(net::buffer(msg), net::use_awaitable);
                m_write_queue.pop_front();
            }
            if (self->m_should_close) {
                co_await self->m_connection.async_close(websocket::close_code::policy_error, net::use_awaitable);
            }
        } catch(...) {}

        m_is_writing = false;
    }

    std::size_t m_id;
    Connection m_connection;
    std::weak_ptr<Room> m_room; // room outlives session

    bool m_should_close = false;
    bool m_is_writing = false;
    std::deque<std::string> m_write_queue;
};

void Room::broadcast_message(const nlohmann::json &data, std::optional<std::size_t> exclude_id){
    auto serialized = data.dump();
    std::lock_guard lock(m_mutex);
    for(const auto &[id, weak_session] : m_sessions){
        if(id == exclude_id) continue;
        if(auto session = weak_session.lock()){
            session->deliver(serialized);
        }
    }
}


class RoomManager {
public:

    std::shared_ptr<Room> create_empty_room() {
        std::lock_guard lock(m_mutex);
        auto code = m_id_counter++;
        auto destroyer = [this](RoomCode code){
            std::lock_guard lock(m_mutex);
            m_rooms.erase(code);
        };
        return m_rooms.emplace(code, std::make_shared<Room>(code, destroyer)).first->second;
    }
    std::optional<std::shared_ptr<Room>> get_room(RoomCode code) {
        std::lock_guard lock(m_mutex);
        auto it = m_rooms.find(code);
        if (it != m_rooms.end()) {
            return it->second;
        }
        return std::nullopt;
    }
private:
    std::mutex m_mutex;
    std::size_t m_id_counter = 1000;
    std::unordered_map<RoomCode, std::shared_ptr<Room>> m_rooms;
};





class Server{
public:
    using tcp = net::ip::tcp;
    using WebSocket = beast::websocket::stream<beast::tcp_stream>;

    auto prepare_session(WebSocket ws)
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
                    co_return std::make_shared<RoomMember>(std::move(ws), m_room_manager.create_empty_room());
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
                    co_return std::make_shared<RoomMember>(std::move(ws), std::move(*room));
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

    net::awaitable<void> run_session(WebSocket ws){
        auto session = co_await prepare_session(std::move(ws));
        if(session){
            co_await (*session)->run();
        }
    }

    net::awaitable<void> listener(net::ip::port_type port){
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
    static void start(net::ip::port_type port, std::size_t num_threads){
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
private:
    Server() = default;
    RoomManager m_room_manager;
};



int main() {
    Server::start(8888, 4);
}
