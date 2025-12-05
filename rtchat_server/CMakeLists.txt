set(Boost_USE_STATIC_LIBS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS url)
find_package(nlohmann_json 3.11.3)

add_subdirectory(common)
add_subdirectory(room)

add_executable (rtchat_server
    "main.cpp"
    "room_manager.hpp"
    "room_manager.cpp"
    "server.hpp"
    "server.cpp"
)
set_target_properties(rtchat_server PROPERTIES CXX_STANDARD 20)
target_link_libraries(rtchat_server PRIVATE
    RoomLib
    Boost::disable_autolinking
    Boost::url
)
