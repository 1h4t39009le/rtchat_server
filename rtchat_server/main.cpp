#include "server.hpp"

int main() {
    Server::start(8888, 4);
}

/*

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;

class RoomMember;


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
    std::optional<std::size_t> add_session(std::string name, std::shared_ptr<RoomMember> session) {
        std::size_t id;
        {
            std::lock_guard lock(m_mutex);
            if(m_is_dead) return std::nullopt;
            id = m_id_counter++;
            m_sessions.emplace(id, std::move(session));
        }
        broadcast_message(ServerRoomMessage{ServerRoomAction::Joined, id, std::move(name)}, id);
        return id;
    }
    std::mutex m_mutex;
    std::function<void(RoomCode)> m_on_leave;
    bool m_is_dead = false;
    RoomCode m_code;
    std::size_t m_id_counter{};
    std::unordered_map<std::size_t, std::weak_ptr<RoomMember>> m_sessions{};
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






*/
