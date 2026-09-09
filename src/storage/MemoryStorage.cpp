#include "storage/MemoryStorage.hpp"

#include <fstream>

    void MemoryStorage::saveMemories(const MemoryStore& memoryStore){
        std::ofstream file("data/memory.json");

        file << memoryStore.toJson().dump(4);
    }

    MemoryStore MemoryStorage::loadMemories() const{
        std::ifstream file("data/memory.json");

        MemoryStore memoryStore;
        
        if(!file){
            return memoryStore;
        }

        nlohmann::json data;

        file >> data;

        memoryStore.fromJson(data);

        return memoryStore;
    }