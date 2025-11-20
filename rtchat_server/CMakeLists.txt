find_package(Boost REQUIRED)
find_package(nlohmann_json 3.12.0 REQUIRED)





add_executable (rtchat_server "rtchat_server.cpp" )
if (MSVC)
    target_compile_options(rtchat_server PRIVATE /bigobj)
endif (MSVC)

target_precompile_headers(rtchat_server PRIVATE
    <boost/beast.hpp>
    <boost/asio.hpp>
    <nlohmann/json.hpp>
)

set_target_properties(rtchat_server PROPERTIES CXX_STANDARD 20)
target_link_libraries(rtchat_server PRIVATE Boost::boost  nlohmann_json::nlohmann_json)