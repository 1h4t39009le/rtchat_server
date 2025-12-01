#pragma once
#include <optional>
#include <string>
#include <nlohmann/json.hpp>

NLOHMANN_JSON_NAMESPACE_BEGIN
template <typename T>
struct adl_serializer<std::optional<T>> {
    static void to_json(json& j, const std::optional<T>& opt) {
        if (opt) j = *opt; else j = nullptr;
    }
    static void from_json(const json& j, std::optional<T>& opt) {
        if (j.is_null()) opt = std::nullopt; else opt = j.get<T>();
    }
};
NLOHMANN_JSON_NAMESPACE_END

using ClientNames = std::unordered_map<size_t, std::string>;

NLOHMANN_JSON_NAMESPACE_BEGIN
template<>
struct adl_serializer<ClientNames> {
    static void to_json(json& j, const ClientNames& client_names) {
        j = json::object();
        for (const auto& [key, value] : client_names) {
            j[std::to_string(key)] = value;  // Явное преобразование
        }
    }
    static void from_json(const json& j, ClientNames& client_names) {
        client_names.clear();
        for (auto& [key_str, value] : j.items()) {
            size_t key = std::stoull(key_str);
            client_names[key] = value.get<std::string>();
        }
    }
};
NLOHMANN_JSON_NAMESPACE_END

enum class ClientPrepareAction { Join, Create };

struct ClientPrepareMessage {
    ClientPrepareAction action;
    std::string name;
    std::optional<std::size_t> room_code;
};
enum class ServerPrepareError { InvalidJson, InvalidRoomCode};
struct ServerPrepareResponse {
    std::optional<size_t> client_id;
    std::optional<ClientNames> client_names;
    std::optional<size_t> room_code;
    std::optional<ServerPrepareError> error;
};
enum class ServerRoomAction { Sended, Joined, Leaved };
struct ServerRoomMessage {
    ServerRoomAction action;
    std::size_t client_id;
    std::optional<std::string> message_data;
};

NLOHMANN_JSON_SERIALIZE_ENUM(ClientPrepareAction, {
    {ClientPrepareAction::Join, 0},
    {ClientPrepareAction::Create, 1},
})
NLOHMANN_JSON_SERIALIZE_ENUM(ServerRoomAction, {
    {ServerRoomAction::Sended, 0},
    {ServerRoomAction::Joined, 1},
    {ServerRoomAction::Leaved, 2},
})

NLOHMANN_JSON_SERIALIZE_ENUM(ServerPrepareError, {
    {ServerPrepareError::InvalidJson, 0},
    {ServerPrepareError::InvalidRoomCode, 1},
})
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ClientPrepareMessage, action, name, room_code)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ServerPrepareResponse, client_id, client_names, room_code, error)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ServerRoomMessage, action, client_id, message_data)
