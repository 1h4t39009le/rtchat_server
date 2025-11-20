#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include "json/json_impl.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;

class InChatSession : public std::enable_shared_from_this<InChatSession> {
    using Connection = beast::websocket::stream<beast::tcp_stream>;
    InChatSession(Connection connection, Room &room) : m_connection(std::move(connection)), m_room(room) {}
    
    InChatSession(const InChatSession&) = delete;
    InChatSession& operator=(const InChatSession&) = delete;

    InChatSession(InChatSession&&) = delete;
    InChatSession& operator=(InChatSession&&) = delete;

private:
    Room& m_room; // room outlives session
    Connection m_connection;
};

class Room {
private:
    friend class RoomManager;
    void add_session(std::shared_ptr<InChatSession> session) {
        std::lock_guard lock(m_mutex);
        m_sessions.push_back(std::move(session));
    }
    std::mutex m_mutex;
    std::vector<std::weak_ptr<InChatSession>> m_sessions{};
};

class RoomManager {
public:
    using RoomCode = std::size_t;
    void create_room(std::shared_ptr<InChatSession> session) {
        std::lock_guard lock(m_mutex);
        auto &room = m_rooms.emplace(m_id_counter, Room()).first->second;
        room.add_session(std::move(session));
    }
    void join_room(std::shared_ptr<InChatSession>& session) {
        std::terminate();
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

int main() {
    auto session = std::make_shared<InChatSession>(std::move(ws));
    m_room_manager.create_room(session);

}