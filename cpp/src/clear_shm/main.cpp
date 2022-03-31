#include <aRibeiroCore/aRibeiroCore.h>
#include <aRibeiroPlatform/aRibeiroPlatform.h>
using namespace aRibeiro;

void unlinkBuffer(const char* name) {
    std::string buffer_name = std::string("/") + std::string(name) + std::string("_abd");//aribeiro_buffer_data
    std::string semaphore_name = std::string("/") + std::string(name) + std::string("_abs");//aribeiro_buffer_semaphore
    shm_unlink(buffer_name.c_str());
    sem_unlink(semaphore_name.c_str());
}

void unlinkLowLatencyQueue(const char* name) {
    std::string header_name = std::string("/") + std::string(name) + std::string("_allqh");//aribeiro_ll_queue_header
    std::string buffer_name = std::string("/") + std::string(name) + std::string("_allqb");//aribeiro_ll_queue_buffer
    std::string semaphore_name = std::string("/") + std::string(name) + std::string("_allqs");//aribeiro_ll_queue_semaphore

    shm_unlink(buffer_name.c_str());
    shm_unlink(header_name.c_str());
    sem_unlink(semaphore_name.c_str());
    
    std::string semaphore_count_name = std::string(name) + std::string("_allqsc");//aribeiro_ll_queue_semaphore_count
    std::string sem_count_name = std::string("/") + semaphore_count_name;

    sem_unlink(sem_count_name.c_str());

}

int main(int argc, char* argv[]){
    unlinkBuffer("network-to-ipc");
    unlinkLowLatencyQueue( "aRibeiro Cam 01" );
    return 0;
}
