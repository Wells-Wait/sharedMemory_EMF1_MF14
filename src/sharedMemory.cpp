#include "sharedMemory.h"
#include <atomic>



SharedMemoryManager::SharedMemoryManager(MF26::v2::VehicleState *ptr_vehicleState,SharedMemoryManagerMode sharedMemoryManagerMode){
        this->mode = sharedMemoryManagerMode;
        this->vehicleStateSharedMemory.vehicleState = ptr_vehicleState;

                //keep seeing if shared memory is there yet

        if(mode==SharedMemoryManagerMode::SERVER){
            //make shared memory
            while(!(this->makeSharedMemory())){
                std::cout << "Failed to make memory trying again"<< std::endl;   
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }

        }else{
            //connect to exsisting memory
            while(!(this->connectSharedMemory())){
                std::cout << "Failed to connect memory trying again"<< std::endl;   
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }


                
                


}




bool SharedMemoryManager::makeSharedMemory() {
    try {
        // 1. Always remove old remnants first
        boost::interprocess::shared_memory_object::remove("SharedMemory");

        // 2. Use make_unique instead of 'new' to be consistent with connectSharedMemory()
        segment = std::make_unique<boost::interprocess::managed_shared_memory>(
            boost::interprocess::create_only, 
            "SharedMemory", 
            65536
        );

        // 3. Construct the object in the segment
        // The parentheses () call the constructor for your packet
        auto* ptr = segment->construct<VehicleStateSharedMemoryPacket>("VehicleState")();
        if (ptr == nullptr) return false;
        this->vehicleStateSharedMemory.sharedMemory = ptr;

        auto* ptr2 = segment->construct<VCUCommandSharedMemoryPacket>("VCUCommand")();
        if (ptr2 == nullptr) return false;
        this->vcuCommandSharedMemory.sharedMemory = ptr2;

        return true;
    }
    catch (...) {
        // Log error: e.what()
        return false;
    }
}
bool SharedMemoryManager::connectSharedMemory(){
    try
    {
        segment = std::make_unique<
            boost::interprocess::managed_shared_memory
        >(
            boost::interprocess::open_only,
            "SharedMemory"
        );

        auto result = segment->find<VehicleStateSharedMemoryPacket>("Telemetry");
        if (result.first == nullptr) {
            return false;
        }

        this->vehicleStateSharedMemory.sharedMemory = result.first;
        return true;

    }
    catch (...)
    {
        return false;
    }
}


bool SharedMemoryManager::setData(){
    


    

    

     // Serialize to a string
    std::string serialized;
    if (!vehicleStateSharedMemory.vehicleState->SerializeToString(&serialized)) {
        std::cerr << "Failed to serialize EV message!\n";
        return false;
    }

    
    
    size_t size=serialized.size();
    
    if (size > 1024){
        std::cout << "too big" << std::endl;
        return false;
    } // Basic bounds check

    
    uint8_t writerCounter = vehicleStateSharedMemory.sharedMemory->writerCounter.load(std::memory_order_acquire);
    ProtoPacket* packetPtr = nullptr;
    if (writerCounter%2 == 0){
        packetPtr = &vehicleStateSharedMemory.sharedMemory->packetA;
    }else{
        packetPtr = &vehicleStateSharedMemory.sharedMemory->packetB;
    }
    if(vehicleStateSharedMemory.sharedMemory->readerCounterVCU.load(std::memory_order_acquire)%2 == 0){
    packetPtr->dataSize = size;
    std::memcpy(packetPtr->buffer, serialized.data(), size);
    

    
    vehicleStateSharedMemory.sharedMemory->writerCounter.fetch_add(1, std::memory_order_release);
    // Lock automatically releases here when function returns
    return true;
    }
    
    


}

bool SharedMemoryManager::getData(){
    bool state = true;
    if(mode==SharedMemoryManagerMode::VCU || mode==SharedMemoryManagerMode::SCREEN){
        state &= getVehicleState();
    }
    if(mode==SharedMemoryManagerMode::SERVER || mode==SharedMemoryManagerMode::SCREEN){
        state &= getVCU();
    }
    

    return state;
}

bool SharedMemoryManager::getVehicleState(){

    bool state = false;
    
    //check if new data this also lets writer have room to update
    uint8_t writerCounter = vehicleStateSharedMemory.sharedMemory->writerCounter.load(std::memory_order_acquire);

    if (writerCounter != this->vehicleStateSharedMemory.writerCounterLast){

        //lock writer
        uint8_t old_val;
        switch(mode) {

            case SharedMemoryManagerMode::VCU:
                old_val=vehicleStateSharedMemory.sharedMemory->readerCounterVCU.fetch_add(1, std::memory_order_release);
                if(old_val % 2 !=0){
                    std::cout << "VCU Shared memory counter Error resetting and reading counter set to 1"<< std::endl;
                    vehicleStateSharedMemory.sharedMemory->readerCounterVCU.store(1, std::memory_order_release);
                }
            break;
            case SharedMemoryManagerMode::SCREEN:
                old_val=vehicleStateSharedMemory.sharedMemory->readerCounterScreen.fetch_add(1, std::memory_order_release);
                if(old_val % 2 !=0){
                    std::cout << "VCU Shared memory counter Error resetting and reading counter set to 1"<< std::endl;
                    vehicleStateSharedMemory.sharedMemory->readerCounterScreen.store(1, std::memory_order_release);
                }
    
            break;

    

        }





        //Check to make sure writer has not moved if it has this will make sure nothing breaks
        uint8_t writerCounter = vehicleStateSharedMemory.sharedMemory->writerCounter.load(std::memory_order_acquire);

        this->vehicleStateSharedMemory.writerCounterLast = writerCounter;//update last writer as this is going to be the writer counter for the read
         
        //select memory packet
        ProtoPacket* packetPtr = nullptr;
        if (vehicleStateSharedMemory.sharedMemory->writerCounter%2 == 0){
            //even means writer is on or will be on A
            
            packetPtr = &vehicleStateSharedMemory.sharedMemory->packetB;

        }else{
            //odd means writer is on or will be on B

            packetPtr = &vehicleStateSharedMemory.sharedMemory->packetA;
        }
        

        
        //exstract the data
        if (packetPtr->dataSize > 0) {
            if (this->vehicleStateSharedMemory.vehicleState->ParseFromArray(packetPtr->buffer, packetPtr->dataSize)) {
                //std::cout << "Inverter Temp: "<< this->vehicleState->ev().inverter().invertertemp()<< std::endl;
                state=true;
            } else {
                    std::cout << "Failed to parse protobuf message."
                            << std::endl;
                    state=false;     
            }
        }
       
        //set to not reading(even number)
        switch(mode) {

            case SharedMemoryManagerMode::VCU:
                vehicleStateSharedMemory.sharedMemory->readerCounterVCU.fetch_add(1, std::memory_order_release);
            break;
            case SharedMemoryManagerMode::SCREEN:
                vehicleStateSharedMemory.sharedMemory->readerCounterScreen.fetch_add(1, std::memory_order_release);
    
            break;

    

        }
        
        
        

        
        
    
        //std::cout << "update reader status finsished"<< std::endl;
        
    }else{
    //std::cout << "Data has not changed"<< std::endl;
    return false;
    
    }
    //make even done reading
    return state;



}

bool SharedMemoryManager::getVCU(){
    
    bool state = false;
    
    //check if new data this also lets writer have room to update
    uint8_t writerCounter = vcuCommandSharedMemory.sharedMemory->writerCounter.load(std::memory_order_acquire);

    if (writerCounter != this->vcuCommandSharedMemory.writerCounterLast){

        //lock writer
        uint8_t old_val;
        switch(mode) {

            case SharedMemoryManagerMode::SERVER:
                old_val=vcuCommandSharedMemory.sharedMemory->readerCounterServer.fetch_add(1, std::memory_order_release);
                if(old_val % 2 !=0){
                    std::cout << "VCU Shared memory counter Error resetting and reading counter set to 1"<< std::endl;
                    vcuCommandSharedMemory.sharedMemory->readerCounterServer.store(1, std::memory_order_release);
                }
            break;
            case SharedMemoryManagerMode::SCREEN:
                old_val=vcuCommandSharedMemory.sharedMemory->readerCounterScreen.fetch_add(1, std::memory_order_release);
                if(old_val % 2 !=0){
                    std::cout << "VCU Shared memory counter Error resetting and reading counter set to 1"<< std::endl;
                    vcuCommandSharedMemory.sharedMemory->readerCounterScreen.store(1, std::memory_order_release);
                }
    
            break;

    

        }





        //Check to make sure writer has not moved if it has this will make sure nothing breaks
        uint8_t writerCounter = vcuCommandSharedMemory.sharedMemory->writerCounter.load(std::memory_order_acquire);

        this->vcuCommandSharedMemory.writerCounterLast = writerCounter;//update last writer as this is going to be the writer counter for the read
         
        //select memory packet
        ProtoPacket* packetPtr = nullptr;
        if (vcuCommandSharedMemory.sharedMemory->writerCounter%2 == 0){
            //even means writer is on or will be on A
            
            packetPtr = &vcuCommandSharedMemory.sharedMemory->packetB;

        }else{
            //odd means writer is on or will be on B

            packetPtr = &vcuCommandSharedMemory.sharedMemory->packetA;
        }
        

        
        //exstract the data
        if (packetPtr->dataSize > 0) {
            if (this->vcuCommandSharedMemory.vcuComand->ParseFromArray(packetPtr->buffer, packetPtr->dataSize)) {
                //std::cout << "Inverter Temp: "<< this->vehicleState->ev().inverter().invertertemp()<< std::endl;
                state=true;
            } else {
                    std::cout << "Failed to parse protobuf message."
                            << std::endl;
                    state=false;     
            }
        }
       
        //set to not reading(even number)
        switch(mode) {

            case SharedMemoryManagerMode::SERVER:
                vcuCommandSharedMemory.sharedMemory->readerCounterServer.fetch_add(1, std::memory_order_release);
            break;
            case SharedMemoryManagerMode::SCREEN:
                vcuCommandSharedMemory.sharedMemory->readerCounterScreen.fetch_add(1, std::memory_order_release);
    
            break;

    

        }
        
        
        

        
        
    
        //std::cout << "update reader status finsished"<< std::endl;
        
    }else{
    //std::cout << "Data has not changed"<< std::endl;
    return false;
    
    }
    //make even done reading
    return state;

  
}