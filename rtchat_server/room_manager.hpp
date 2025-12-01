#pragma once
#include "room.hpp"
class RoomManager {
public:
    std::shared_ptr<Room> create_empty_room();
    std::optional<std::shared_ptr<Room>> get_room(RoomCode code);
private:
    std::mutex m_mutex;
    std::size_t m_id_counter = 1000;
    std::unordered_map<RoomCode, std::shared_ptr<Room>> m_rooms;
};
