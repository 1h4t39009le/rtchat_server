add_library(RoomLib
    room.hpp
    room.cpp
    room_member.hpp
    room_member.cpp
)
if (MSVC)
    target_compile_options(RoomLib PRIVATE /bigobj)
endif (MSVC)
target_include_directories(RoomLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(RoomLib PUBLIC CommonLib)
