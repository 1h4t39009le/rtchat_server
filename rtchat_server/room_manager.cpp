#include "room_manager.hpp"
RoomManager::RoomManager()
    : m_rand_eng(std::random_device{}()){
}

RoomCode RoomManager::generate_code(){
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const size_t max_index = (sizeof(charset) - 2);
    std::uniform_int_distribution<> dist(0, max_index);
    std::string str(4,0);
    std::generate_n( str.begin(), 4, [&,this]{return charset[dist(m_rand_eng)];} );
    return str;
}
std::shared_ptr<Room> RoomManager::create_empty_room() {
    while(true){
        auto code = generate_code();
        std::lock_guard lock(m_mutex);
        if(m_rooms.find(code) != m_rooms.end()){
            continue;
        }
        auto destroyer = [this](const RoomCode &code){
            std::lock_guard lock(m_mutex);
            m_rooms.erase(code);
        };
        auto room = std::make_shared<Room>(code, destroyer);
        return m_rooms.emplace(std::move(code), std::move(room)).first->second;
    }
}
std::optional<std::shared_ptr<Room>> RoomManager::get_room(const RoomCode &code) {
    std::lock_guard lock(m_mutex);
    auto it = m_rooms.find(code);
    if (it != m_rooms.end()) {
        return it->second;
    }
    return std::nullopt;
}
