find_package(Boost REQUIRED COMPONENTS url)
find_package(nlohmann_json 3.11.3)

add_executable (rtchat_server
    "main.cpp"
    "common.hpp"
    "common.cpp"
    "room.hpp"
    "room.cpp"
    "room_member.hpp"
    "room_member.cpp"
    "room_manager.hpp"
    "room_manager.cpp"
    "json/json_impl.hpp"
    "server.hpp"
    "server.cpp"
)

if (MSVC)
    target_compile_options(rtchat_server PRIVATE /bigobj)
endif (MSVC)

target_precompile_headers(rtchat_server PRIVATE
    <boost/asio.hpp>
    <boost/beast.hpp>
    <nlohmann/json.hpp>
)

set_target_properties(rtchat_server PROPERTIES CXX_STANDARD 20)
target_link_libraries(rtchat_server PRIVATE
    Boost::disable_autolinking
    Boost::boost Boost::url
    nlohmann_json::nlohmann_json
)
