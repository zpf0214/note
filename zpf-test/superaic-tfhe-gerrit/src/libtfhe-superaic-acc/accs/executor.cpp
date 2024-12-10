#include <assert.h>
#include "tfhe_acc.h"

using namespace std;
namespace tfhe_superaic {
bool Executor::isBusy() {
    return busy;
}



Executor::Executor(Executor_Type type,string name)
    :executor_type(type), name(name){

}



Executor_FPGA::Executor_FPGA(string name):
    Executor(Executor::Executor_FPGA,name) {

}

Executor_List::~Executor_List() {
}



Executor_List::Executor_List(void) {
}

void Executor_List::append_exector (shared_ptr<Executor> executor) {
    unique_lock<std::mutex> lock(_executors_mtx);
    executor->busy = false;
    executors.push_back(executor);
}


shared_ptr<Executor>  Executor_List::aquire_executor(void) {
    unique_lock<std::mutex> lock(_executors_mtx);

    // 寻找一个可用的
    for(auto executor : executors) {
        if ( !executor->busy ) {
            executor->busy = true;
            return executor;

        }
    }

    // 找不到可用的，那么就等
    _notFree.wait(lock);
    
    // 唤醒以后立刻就持有lock，可以避免其他线程进入临界区
    for(auto executor : executors) {
        if ( !executor->busy ) {
            executor->busy = true;
            return executor;

        }
    }

    return nullptr;
  
}

void Executor_List::release_executor(shared_ptr<Executor> executor) {
    unique_lock<std::mutex> lock(_executors_mtx);
    executor->busy = false;
    _notFree.notify_one();
}



}