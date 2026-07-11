#pragma once

#include <iostream>
#include <cstdint>
#include <thread> 
#include <memory>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include "MF26/v2/vehicle.pb.h"
enum class SharedMemoryManagerMode{
    VCU,
    SCREEN,
    SERVER
};


struct ProtoPacket{
    size_t dataSize=0;
    char buffer[1024];
};


// vehicle state stuff
struct VehicleStateSharedMemoryPacket {

    std::atomic<uint8_t> writerCounter=0;
    std::atomic<uint8_t> readerCounterVCU=0;
    std::atomic<uint8_t> readerCounterScreen=0;

    
    ProtoPacket packetA;
    ProtoPacket packetB;
};
struct VehicleStateSharedMemory{
    uint8_t writerCounterLast;
    uint8_t readerCounterVCULast;
    uint8_t readerCounterScreen;
    VehicleStateSharedMemoryPacket* sharedMemory;
    MF26::v2::VehicleState *vehicleState;

};

// VCU shared memory stuff

class SharedMemoryManager
{
private:
    SharedMemoryManagerMode mode;
    std::unique_ptr<boost::interprocess::managed_shared_memory> segment;


    //general shared memory

    VehicleStateSharedMemory vehicleStateSharedMemory;


    //VCU shared memory

    bool makeSharedMemory();
    bool connectSharedMemory();

    bool getVehicleState();
    bool getVCU();


 

public:
    SharedMemoryManager(MF26::v2::VehicleState *ptr_vehicleState,SharedMemoryManagerMode sharedMemoryManagerMode);
    bool getData();

};