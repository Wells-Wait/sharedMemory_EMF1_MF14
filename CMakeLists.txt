add_library(SharedMemory STATIC 
    src/sharedMemory.cpp
)
find_package(Boost CONFIG REQUIRED)
target_link_libraries(SharedMemory PRIVATE Boost::headers)


target_include_directories(SharedMemory PRIVATE 
    ${Boost_INCLUDE_DIRS}
)
target_link_libraries(SharedMemory PUBLIC ProtobufLib)
# This allows 'vcu_app' to see the headers in your module's include folder
target_include_directories(SharedMemory PUBLIC include)