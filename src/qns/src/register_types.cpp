#include "register_types.h"

#include "quic_network_system.hpp"
#include "qns_networked_node.hpp"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_qns_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    // Register your QNS node here
    ClassDB::register_class<QuicNetworkSystem>();
    ClassDB::register_class<QNSNetworkedNode>();
    
    QuicNetworkSystem* qns_instance = memnew(QuicNetworkSystem);
    Engine::get_singleton()->register_singleton("QuicNetworkSystem", qns_instance);
}

void uninitialize_qns_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {
    // This name MUST match the 'entry_symbol' in your qns.gdextension file
    GDExtensionBool GDE_EXPORT qns_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
        godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_qns_module);
        init_obj.register_terminator(uninitialize_qns_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}