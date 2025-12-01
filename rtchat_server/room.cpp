#include "json/json_impl.hpp"
#include "room_member.hpp"
#include "room.hpp"

Room::Room(RoomCode code, std::function<void(RoomCode)> on_leave)
    : m_on_leave(std::move(on_leave)), m_code(code) {
}
RoomCode Room::get_code() {
    return m_code;
}
void Room::sending(std::size_t id, const std::string &message){
    broadcast_message(ServerRoomMessage{ServerRoomAction::Sended, id, message});
}
void Room::leaving(std::size_t id){
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
std::optional<std::size_t> Room::add_session(std::shared_ptr<RoomMember> session) {
    std::size_t id;
    {
        std::lock_guard lock(m_mutex);
        if(m_is_dead) return std::nullopt;
        id = m_id_counter++;
        m_sessions.emplace(id, std::move(session));
    }
    broadcast_message(ServerRoomMessage{ServerRoomAction::Joined, id, session->get_name()}, id);
    return id;
}
