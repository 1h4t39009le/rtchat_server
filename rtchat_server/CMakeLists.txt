find_package(Boost 1.89.0 REQUIRED)

add_executable (rtchat_server "rtchat_server.cpp" "rtchat_server.h")

set_target_properties(rtchat_server PROPERTIES CXX_STANDARD 20)
target_include_directories(rtchat_server PRIVATE ${Boost_INCLUDE_DIRS})
