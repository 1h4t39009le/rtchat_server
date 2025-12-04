#include "server.hpp"
int main() {
    Server::start(8888, 4);
}
/*
 * global_storage_manager
 * |-local_storage:Room1
 * |-local_storage:Room2
 * |-local_storage:Room3
 *
 * Situation
 * Room1 gets FILE1 from client_id:1
 * Room1.save_file(id, data) -> Room1.local_storage.save("/{id}/",data)
 * Room1.local_storage.save(path, data) {
 *      write_file data at m_storage_id/path
 * }
 * global_storage.read_file(storage_id, path)
*/
