#include <cpp.h>

namespace ns {
struct NestedNameSpaceType { };

class NestedModule {
public:
    NestedModule(flecs::world& world) {
        world.module<ns::NestedModule>();
        flecs::component<Velocity>(world, "Velocity");
    }
};

class SimpleModule {
public:
    SimpleModule(flecs::world& world) {
        world.module<ns::SimpleModule>();
        world.import<ns::NestedModule>();
        flecs::component<Position>(world, "Position");
    }
};

class NestedTypeModule {
public:
    struct NestedType { };

    NestedTypeModule(flecs::world& world) {
        world.module<NestedTypeModule>();
        world.component<NestedType>();
        world.component<NestedNameSpaceType>();
    }
};

class NamedModule {
public:
    NamedModule(flecs::world& world) {
        world.module<ns::NamedModule>("::my_scope::NamedModule");
        flecs::component<Position>(world, "Position");
    }
};

class ImplicitModule {
public:
    ImplicitModule(flecs::world& world) {
        world.component<Position>();
    }
};

class NamedModuleInRoot {
public:
    NamedModuleInRoot(flecs::world& world) {
        world.module<NamedModuleInRoot>("::NamedModuleInRoot");
        world.component<Position>();
    }
};

class ReparentModule {
public:
    ReparentModule(flecs::world& world) {
        flecs::entity m = world.module<ReparentModule>();
        m.child_of(world.entity("::parent"));

        flecs::entity other = world.entity("::ns::ReparentModule");
        test_assert(other != 0);
        test_assert(other != m);
    }
};

}

class ReparentRootModule {
public:
    ReparentRootModule(flecs::world& world) {
        world.module<ReparentRootModule>("ns::ReparentRootModule");
    }
};

namespace RenamedRootModule {
    struct Module {
        Module(flecs::world& world) {
            world.module<Module>("::MyModule");
            for(int i = 0; i < 5; ++i) {
                auto e = world.entity();
                test_assert(e.id() == (uint32_t)e.id());
            }
        }
    };
}


namespace ns_parent {
    struct NsType {
        float x;
    };

    struct ShorterParent {
        ShorterParent(flecs::world& world) {
            world.module<ShorterParent>("ns::ShorterParent");
            world.component<NsType>();
        }
    };

    struct LongerParent {
        LongerParent(flecs::world& world) {
            world.module<LongerParent>("ns_parent_namespace::LongerParent");
            world.component<NsType>();
        }
    };

    namespace ns_child {
        struct Nested {
            Nested(flecs::world& world) {
                world.module<Nested>("ns::child::Nested");
                world.component<NsType>();
            }
        };
    }
}

struct Module {
    Module(flecs::world& world) {
        world.module<Module>();
        world.component<Position>();
    }
};

void Module_import(void) {
    flecs::world world;
    auto m = world.import<ns::SimpleModule>();
    test_assert(m.id() != 0);
    test_str(m.path().c_str(), "::ns::SimpleModule");
    test_assert(m.has(flecs::Module));

    auto e = flecs::entity(world)
        .add<Position>();
    test_assert(e.id() != 0);
    test_assert(e.has<Position>());
}

void Module_lookup_from_scope(void) {
    flecs::world world;
    world.import<ns::SimpleModule>();

    auto ns_entity = world.lookup("ns");
    test_assert(ns_entity.id() != 0);

    auto module_entity = world.lookup("ns::SimpleModule");
    test_assert(module_entity.id() != 0);

    auto position_entity = world.lookup("ns::SimpleModule::Position");
    test_assert(position_entity.id() != 0);

    auto nested_module = ns_entity.lookup("SimpleModule");
    test_assert(module_entity.id() == nested_module.id());

    auto module_position = module_entity.lookup("Position");
    test_assert(position_entity.id() == module_position.id());

    auto ns_position = ns_entity.lookup("SimpleModule::Position");
    test_assert(position_entity.id() == ns_position.id());    
}

void Module_nested_module(void) {
    flecs::world world;
    world.import<ns::SimpleModule>();

    auto velocity = world.lookup("ns::NestedModule::Velocity");
    test_assert(velocity.id() != 0);

    test_str(velocity.path().c_str(), "::ns::NestedModule::Velocity");
}

void Module_nested_type_module(void) {
    flecs::world world;
    world.import<ns::NestedTypeModule>();

    auto ns_entity = world.lookup("ns");
    test_assert(ns_entity.id() != 0);

    auto module_entity = world.lookup("ns::NestedTypeModule");
    test_assert(module_entity.id() != 0);

    auto type_entity = world.lookup("ns::NestedTypeModule::NestedType");
    test_assert(type_entity.id() != 0);

    auto ns_type_entity = world.lookup("ns::NestedTypeModule::NestedNameSpaceType");
    test_assert(ns_type_entity.id() != 0);

    int32_t childof_count = 0;
    type_entity.each(flecs::ChildOf, [&](flecs::entity) {
        childof_count ++;
    });

    test_int(childof_count, 1);

    childof_count = 0;
    ns_type_entity.each(flecs::ChildOf, [&](flecs::entity) {
        childof_count ++;
    });

    test_int(childof_count, 1);    
}

void Module_component_redefinition_outside_module(void) {
    flecs::world world;

    world.import<ns::SimpleModule>();

    auto pos_comp = world.lookup("ns::SimpleModule::Position");
    test_assert(pos_comp.id() != 0);

    auto pos = world.component<Position>();
    test_assert(pos.id() != 0);
    test_assert(pos.id() == pos_comp.id());

    int32_t childof_count = 0;
    pos_comp.each(flecs::ChildOf, [&](flecs::entity) {
        childof_count ++;
    });

    test_int(childof_count, 1);
}

void Module_module_tag_on_namespace(void) {
    flecs::world world;

    auto mid = world.import<ns::NestedModule>();
    test_assert(mid.has(flecs::Module));

    auto nsid = world.lookup("ns");
    test_assert(nsid.has(flecs::Module));
}

static int module_ctor_invoked = 0;
static int module_dtor_invoked = 0;

class Module_w_dtor {
public:
    Module_w_dtor(flecs::world& world) {
        world.module<Module_w_dtor>();
        module_ctor_invoked ++;

        world.system<>().run([](flecs::iter& it) { });
    }

    ~Module_w_dtor() {
        module_dtor_invoked ++;
    }    
};

void Module_dtor_on_fini(void) {
    {
        flecs::world ecs;

        test_int(module_ctor_invoked, 0);
        test_int(module_dtor_invoked, 0);

        ecs.import<Module_w_dtor>();
        
        test_int(module_ctor_invoked, 1);
        test_int(module_dtor_invoked, 0);
    }

    test_int(module_dtor_invoked, 1);
}

void Module_register_w_root_name() {
    flecs::world ecs;

    auto m = ecs.import<ns::NamedModule>();

    auto m_lookup = ecs.lookup("::my_scope::NamedModule");
    test_assert(m != 0);
    test_assert(m == m_lookup);

    test_assert(ecs.lookup("::ns::NamedModule") == 0);
}

void Module_implicit_module(void) {
    flecs::world ecs;

    auto m = ecs.import<ns::ImplicitModule>();
    auto m_lookup = ecs.lookup("::ns::ImplicitModule");
    test_assert(m != 0);
    test_assert(m == m_lookup);

    auto p = ecs.component<Position>();
    auto p_lookup = ecs.lookup("::ns::ImplicitModule::Position");
    test_assert(p != 0);
    test_assert(p == p_lookup);
}

void Module_module_in_namespace_w_root_name(void) {
    flecs::world ecs;

    auto m = ecs.import<ns::NamedModuleInRoot>();
    auto m_lookup = ecs.lookup("::NamedModuleInRoot");
    test_assert(m != 0);
    test_assert(m == m_lookup);
    test_str(m.path(), "::NamedModuleInRoot");

    auto p = ecs.component<Position>();
    auto p_lookup = ecs.lookup("::NamedModuleInRoot::Position");
    test_assert(p != 0);
    test_assert(p == p_lookup);
}

void Module_module_as_entity(void) {
    flecs::world world;

    auto m = world.import<ns::SimpleModule>();
    test_assert(m != 0);

    auto e = world.entity<ns::SimpleModule>();
    test_assert(m == e);
}

void Module_module_as_component(void) {
    flecs::world world;

    auto m = world.import<ns::SimpleModule>();
    test_assert(m != 0);

    auto e = world.component<ns::SimpleModule>();
    test_assert(m == e);
}

void Module_module_with_core_name(void) {
    flecs::world world;

    flecs::entity m = world.import<Module>();
    test_assert(m != 0);
    test_str(m.path().c_str(), "::Module");

    flecs::entity pos = m.lookup("Position");
    test_assert(pos != 0);
    test_str(pos.path().c_str(), "::Module::Position");
    test_assert(pos == world.id<Position>());
}

void Module_import_addons_two_worlds(void) {
    flecs::world a;
    auto m1 = a.import<flecs::stats>();
    auto u1 = a.import<flecs::units>();

    flecs::world b;
    auto m2 = b.import<flecs::stats>();
    auto u2 = b.import<flecs::units>();

    test_assert(m1 == m2);
    test_assert(u1 == u2);
}

void Module_lookup_module_after_reparent(void) {
    flecs::world world;

    flecs::entity m = world.import<ns::NestedModule>();
    test_str(m.path().c_str(), "::ns::NestedModule");
    test_assert(world.lookup("::ns::NestedModule") == m);
    test_assert(ecs_lookup(world, "ns.NestedModule") == m);

    flecs::entity p = world.entity("p");
    m.child_of(p);
    test_str(m.path().c_str(), "::p::NestedModule");
    test_assert(world.lookup("::p::NestedModule") == m);
    test_assert(ecs_lookup(world, "p.NestedModule") == m);
    
    test_assert(world.lookup("::ns::NestedModule") == 0);
    test_assert(ecs_lookup(world, "ns.NestedModule") == 0);

    flecs::entity e = world.entity("::ns::NestedModule");
    test_assert(e != m);

    // Tests if symbol resolving (used by query DSL) interferes with getting the
    // correct object
    test_int(world.query_builder()
        .expr("(ChildOf, p.NestedModule)").build().count(), 1);
    test_int(world.query_builder()
        .expr("(ChildOf, ns.NestedModule)").build().count(), 0);
}

void Module_reparent_module_in_ctor(void) {
    flecs::world world;

    flecs::entity m = world.import<ns::ReparentModule>();
    test_str(m.path().c_str(), "::parent::ReparentModule");

    flecs::entity other = world.lookup("::ns::ReparentModule");
    test_assert(other != 0);
    test_assert(other != m);
}
namespace NamespaceLvl1 {
    namespace NamespaceLvl2 {
        struct StructLvl1 {
            struct StructLvl2_1 {};
            struct StructLvl2_2 {};
        };
    }
}
void Module_implicitly_add_module_to_scopes_component(void) {
    flecs::world ecs;

    using StructLvl2_1 = NamespaceLvl1::NamespaceLvl2::StructLvl1::StructLvl2_1;

    flecs::entity current = ecs.component<StructLvl2_1>();
    test_assert(current.id() != 0);
    test_assert(!current.has(flecs::Module));
    test_assert(current.has<flecs::Component>());
    test_assert(current.path() == "::NamespaceLvl1::NamespaceLvl2::StructLvl1::StructLvl2_1");

    current = current.parent();
    test_assert(current.id() != 0);
    test_assert(current.has(flecs::Module));
    test_assert(current.path() == "::NamespaceLvl1::NamespaceLvl2::StructLvl1");

    current = current.parent();
    test_assert(current.id() != 0);
    test_assert(current.has(flecs::Module));
    test_assert(current.path() == "::NamespaceLvl1::NamespaceLvl2");

    current = current.parent();
    test_assert(current.id() != 0);
    test_assert(current.has(flecs::Module));
    test_assert(current.path() == "::NamespaceLvl1");

    current = current.parent();
    test_assert(current.id() == 0);
}

void Module_implicitly_add_module_to_scopes_entity(void) {
    flecs::world ecs;

    using StructLvl2_2 = NamespaceLvl1::NamespaceLvl2::StructLvl1::StructLvl2_2;

    flecs::entity current = ecs.entity<StructLvl2_2>().set<flecs::Component>({});
    test_assert(current.id() != 0);
    test_assert(!current.has(flecs::Module));
    test_assert(current.has<flecs::Component>());
    test_assert(current.path() == "::NamespaceLvl1::NamespaceLvl2::StructLvl1::StructLvl2_2");

    current = current.parent();
    test_assert(current.id() != 0);
    test_assert(current.has(flecs::Module));
    test_assert(current.path() == "::NamespaceLvl1::NamespaceLvl2::StructLvl1");

    current = current.parent();
    test_assert(current.id() != 0);
    test_assert(current.has(flecs::Module));
    test_assert(current.path() == "::NamespaceLvl1::NamespaceLvl2");

    current = current.parent();
    test_assert(current.id() != 0);
    test_assert(current.has(flecs::Module));
    test_assert(current.path() == "::NamespaceLvl1");

    current = current.parent();
    test_assert(current.id() == 0);
}

void Module_rename_namespace_shorter(void) {
    flecs::world ecs;

    auto m = ecs.import<ns_parent::ShorterParent>();
    test_assert(m.has(flecs::Module));
    test_str(m.path(), "::ns::ShorterParent");
    test_assert(ecs.lookup("::ns_parent") == 0);
    test_assert(ecs.lookup("::ns_parent::ShorterParent") == 0);
    test_assert(ecs.lookup("::ns_parent::ShorterParent::NsType") == 0);
    test_assert(ecs.lookup("::ns::ShorterParent::NsType") != 0);

    auto ns = ecs.lookup("::ns");
    test_assert(ns != 0);
    test_assert(ns.has(flecs::Module));
}

void Module_rename_namespace_longer(void) {
    flecs::world ecs;

    auto m = ecs.import<ns_parent::LongerParent>();
    test_assert(m.has(flecs::Module));
    test_str(m.path(), "::ns_parent_namespace::LongerParent");
    test_assert(ecs.lookup("::ns_parent") == 0);
    test_assert(ecs.lookup("::ns_parent::LongerParent") == 0);
    test_assert(ecs.lookup("::ns_parent::LongerParent::NsType") == 0);
    test_assert(ecs.lookup("::ns_parent_namespace::LongerParent::NsType") != 0);

    auto ns = ecs.lookup("::ns_parent_namespace");
    test_assert(ns != 0);
    test_assert(ns.has(flecs::Module));
}

void Module_rename_namespace_nested(void) {
    flecs::world ecs;

    auto m = ecs.import<ns_parent::ns_child::Nested>();
    test_assert(m.has(flecs::Module));
    test_str(m.path(), "::ns::child::Nested");
    test_assert(ecs.lookup("::ns::child::Nested::NsType") != 0);
    test_assert(ecs.lookup("::ns_parent::ns_child::Nested::NsType") == 0);
    test_assert(ecs.lookup("::ns_parent::ns_child::Nested") == 0);
    test_assert(ecs.lookup("::ns_parent::ns_child") == 0);
    test_assert(ecs.lookup("::ns_parent") == 0);

    auto ns = ecs.lookup("::ns");
    test_assert(ns != 0);
    test_assert(ns.has(flecs::Module));

    auto ns_child = ecs.lookup("::ns::child");
    test_assert(ns_child != 0);
    test_assert(ns_child.has(flecs::Module));
}

void Module_rename_reparent_root_module(void) {
    flecs::world ecs;

    flecs::entity m = ecs.import<ReparentRootModule>();
    flecs::entity p = m.parent();
    test_assert(p != 0);
    test_str(p.name(), "ns");
    test_str(m.name(), "ReparentRootModule");
}

void Module_no_recycle_after_rename_reparent(void) {
    flecs::world ecs;

    flecs::entity m = ecs.import<RenamedRootModule::Module>();
    flecs::entity p = m.parent();
    test_assert(p == 0);
    test_str(m.name(), "MyModule");
}

void Module_reimport_after_delete(void) {
    flecs::world ecs;

    {
        auto m = ecs.import<Module>();
        test_assert(m.lookup("Position") == ecs.component<Position>());
        test_assert(m == ecs.entity<Module>());
    }

    ecs.entity<Module>().destruct();

    {
        auto m = ecs.import<Module>();
        test_assert(m.lookup("Position") == ecs.component<Position>());
        test_assert(m == ecs.entity<Module>());
    }
}

struct module_a_component { };

struct module_a {
    module_a(flecs::world &world) {
        world.component<module_a_component>();
    }
};

void Module_component_name_w_module_name(void) {
    flecs::world world;

    flecs::entity m = world.import<module_a>();
    test_assert(m != 0);
    flecs::entity c = world.lookup("module_a::module_a_component");
    test_assert(c != 0);
    test_str(c.name().c_str(), "module_a_component");
    test_str(c.parent().name().c_str(), "module_a");
}

struct SystemAndImplicitComponent {
    SystemAndImplicitComponent(flecs::world& world) {
        world.system("VelocitySys").with<Velocity>().each([]() {});
    }
};

void Module_delete_module_w_implicit_component_and_system(void) {
    flecs::world world;

    auto m = world.import<SystemAndImplicitComponent>();

    test_assert(m.lookup("Velocity") == 0);
    test_assert(world.lookup("Velocity") != 0);
    test_assert(m.lookup("VelocitySys") != 0);

    m.destruct();

    test_assert(true); // verify code doesn't crash
}

struct SystemAndExplicitComponent {
    SystemAndExplicitComponent(flecs::world& world) {
        world.component<Velocity>();
        world.system("VelocitySys").with<Velocity>().each([]() {});
    }
};

void Module_delete_module_w_explicit_component_and_system(void) {
    flecs::world world;

    auto m = world.import<SystemAndExplicitComponent>();

    test_assert(m.lookup("Velocity") != 0);
    test_assert(m.lookup("VelocitySys") != 0);

    m.destruct();

    test_assert(true); // verify code doesn't crash
}
