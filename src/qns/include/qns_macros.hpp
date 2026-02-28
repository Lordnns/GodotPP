#pragma once
#include "network_hash.hpp"

#define RPC_SERVER_RELIABLE(func_name) \
builder.add_rpc(godot::ct_hash(#func_name), #func_name, godot::RPCMode::SERVER, true)
#define RPC_SERVER_UNRELIABLE(func_name) \
builder.add_rpc(godot::ct_hash(#func_name), #func_name, godot::RPCMode::SERVER, false)

#define RPC_CLIENT_RELIABLE(func_name) \
builder.add_rpc(godot::ct_hash(#func_name), #func_name, godot::RPCMode::CLIENT, true)
#define RPC_CLIENT_UNRELIABLE(func_name) \
builder.add_rpc(godot::ct_hash(#func_name), #func_name, godot::RPCMode::CLIENT, false)

#define RPC_MULTICAST_RELIABLE(func_name) \
builder.add_rpc(godot::ct_hash(#func_name), #func_name, godot::RPCMode::MULTICAST, true)
#define RPC_MULTICAST_UNRELIABLE(func_name) \
builder.add_rpc(godot::ct_hash(#func_name), #func_name, godot::RPCMode::MULTICAST, false)

#define REPLICATED(prop_name) \
builder.add_property(godot::ct_hash(#prop_name), #prop_name)
#define REPLICATED_USING(prop_name, on_rep_func) \
builder.add_property(godot::ct_hash(#prop_name), #prop_name, #on_rep_func)