#include "json_impl.hpp"
#include "room_member.hpp"
#include "room.hpp"

Room::Room(RoomCode code, std::function<void(const RoomCode&)> on_leave)
    : m_on_leave(std::move(on_leave)), m_code(std::move(code)) {
}
RoomCode const& Room::get_code() {
    return m_code;
}
std::unordered_map<size_t, std::string> Room::get_client_names(){
    return m_client_names;
}
void Room::sending(std::size_t id, const std::string &message){
    broadcast_message(ServerRoomMessage{ServerRoomAction::Sended, id, message});
}
void Room::leaving(std::size_t id){
    {
        std::lock_guard lock(m_mutex);
        m_clients.erase(id);
        m_client_names.erase(id);
        m_is_dead = m_clients.empty();
    }
    if(m_is_dead){
        return m_on_leave(m_code);
    }
    broadcast_message(ServerRoomMessage{ServerRoomAction::Leaved, id});
}
void Room::broadcast_message(const nlohmann::json &data, std::optional<std::size_t> exclude_id){
    auto serialized = data.dump();
    std::lock_guard lock(m_mutex);
    for(const auto &[id, weak_session] : m_clients){
        if(id == exclude_id) continue;
        if(auto session = weak_session.lock()){
            session->deliver(serialized);
        }
    }
}
std::optional<std::size_t> Room::add_client(std::shared_ptr<RoomMember> client) {
    std::size_t id;
    {
        std::lock_guard lock(m_mutex);
        if(m_is_dead) return std::nullopt;
        id = m_id_counter++;
        m_client_names.emplace(id, client->get_name());
        m_clients.emplace(id, std::move(client));
    }
    broadcast_message(ServerRoomMessage{ServerRoomAction::Joined, id, client->get_name()}, id);
    return id;
}
