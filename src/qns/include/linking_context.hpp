#pragma once
#include <map>

namespace qns_core {
    class LinkingContext {
        std::map<uint32_t, void*> net_to_entity;
    public:
        void register_entity(uint32_t net_id, void* entity_ptr) {
            net_to_entity[net_id] = entity_ptr;
        }

        void unregister_entity(uint32_t net_id) {
            net_to_entity.erase(net_id);
        }

        void* get_entity(uint32_t net_id) {
            if (!net_to_entity.count(net_id)) return nullptr;
            return net_to_entity[net_id];
        }

        const std::map<uint32_t, void*>& get_all_entities() const { 
            return net_to_entity; 
        }
    };
}