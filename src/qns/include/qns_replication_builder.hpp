#pragma once
#include <map>
#include <string>
#include <cstdint>

namespace qns_core {
    enum class RPCMode { SERVER, CLIENT, MULTICAST };
    
    struct ReplicatedProperty {
        std::string name;
        std::string on_rep_func;
        bool has_on_rep = false;
    };
    
    struct RPCDefinition {
        std::string name;
        RPCMode mode;
        bool is_reliable;
    };

    class QNSReplicationBuilder {
    public:
        std::map<uint32_t, RPCDefinition> rpcs;
        std::map<uint32_t, ReplicatedProperty> properties;
        
        void add_rpc(uint32_t hash, const char* name, RPCMode mode, bool reliable) {
            rpcs[hash] = { std::string(name), mode, reliable };
        }
        
        void add_property(uint32_t hash, const char* name, const char* on_rep = "") {
            ReplicatedProperty prop;
            prop.name = std::string(name);
            if (on_rep[0] != '\0') {
                prop.on_rep_func = std::string(on_rep);
                prop.has_on_rep = true;
            }
            properties[hash] = prop;
        }
    };
}