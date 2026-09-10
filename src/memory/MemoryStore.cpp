#include "memory/MemoryStore.hpp"

#include <vector>

    void MemoryStore::addMemory(const Memory& memory){
        store.push_back(memory);
    }

    const std::vector<Memory>& MemoryStore::getMemories() const{
        return store;
    }

    nlohmann::json MemoryStore::toJson() const{
        nlohmann::json data;
        
        for(const auto& cell : store){
            data.push_back(cell.toJson());
        }

        return data;
    }

    void MemoryStore::fromJson(const nlohmann::json& data){
        store.clear();

        for(const auto& cell : data){
            store.push_back(Memory::fromJson(cell));
        }

    }

    void MemoryStore::clear(){
            store.clear();
    }

    std::size_t MemoryStore::size() const{
        return store.size();
    }

    std::vector<Memory> MemoryStore::getMemoriesByCategory(const std::string& category) const{
        std::vector<Memory> memory;

        for(const auto& cell : store){
            if(cell.category == category){
                memory.push_back(cell);
            }
        }

        return memory;
    }

    bool MemoryStore::removeMemory(std::size_t index){
        if(index >= store.size()){
            return false;
        }
        store.erase(store.begin() + index);
        return true;
    }