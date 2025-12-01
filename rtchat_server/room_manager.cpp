#include "room_manager.hpp"

std::shared_ptr<Room> RoomManager::create_empty_room() {
    std::lock_guard lock(m_mutex);
    auto code = m_id_counter++;
    auto destroyer = [this](RoomCode code){
        std::lock_guard lock(m_mutex);
        m_rooms.erase(code);
    };
    return m_rooms.emplace(code, std::make_shared<Room>(code, destroyer)).first->second;
}
std::optional<std::shared_ptr<Room>> RoomManager::get_room(RoomCode code) {
    std::lock_guard lock(m_mutex);
    auto it = m_rooms.find(code);
    if (it != m_rooms.end()) {
        return it->second;
    }
    return std::nullopt;
}
