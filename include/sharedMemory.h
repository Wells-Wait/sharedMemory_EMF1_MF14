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
#include "MF26/v2/control.pb.h"
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
    uint8_t readerCounterServerLast;
    uint8_t readerCounterScreenLast;
    VehicleStateSharedMemoryPacket* sharedMemory;
    MF26::v2::VehicleState *vehicleState;

};

// VCU shared memory stuff
struct VCUCommandSharedMemoryPacket {

    std::atomic<uint8_t> writerCounter=0;
    std::atomic<uint8_t> readerCounterServer=0;
    std::atomic<uint8_t> readerCounterScreen=0;

    
    ProtoPacket packetA;
    ProtoPacket packetB;
};
struct VCUCommandSharedMemory{
    uint8_t writerCounterLast;
    uint8_t readerCounterVCULast;
    uint8_t readerCounterScreenLast;
    VCUCommandSharedMemoryPacket* sharedMemory;
    MF26::v2::VCUCommand *vcuCommand;

};



class SharedMemoryManager
{
private:
    SharedMemoryManagerMode mode;
    std::unique_ptr<boost::interprocess::managed_shared_memory> segment;


    //general shared memory

    VehicleStateSharedMemory vehicleStateSharedMemory;


    //VCU shared memory

    VCUCommandSharedMemory vcuCommandSharedMemory;

    bool makeSharedMemory();
    bool connectSharedMemory();

    bool setVehicleState();
    bool setVCU();


    bool getVehicleState();
    bool getVCU();


 

public:
    SharedMemoryManager(MF26::v2::VehicleState *ptr_vehicleState,MF26::v2::VCUCommand *ptr_vcuCommand, SharedMemoryManagerMode sharedMemoryManagerMode);
    bool setData();
    bool getData();

};