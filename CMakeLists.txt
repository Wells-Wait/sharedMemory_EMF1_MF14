add_library(SharedMemory STATIC 
    src/sharedMemory.cpp
)
target_link_libraries(SharedMemory PUBLIC ProtobufLib)
# This allows 'vcu_app' to see the headers in your module's include folder
target_include_directories(SharedMemory PUBLIC include)