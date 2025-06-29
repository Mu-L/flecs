#include <entity_type.h>
#include <iostream>

struct TypeWithEntity {
    flecs::entity_t e; /* Only naked entity ids are supported at the moment */
};

int main(int, char *[]) {
    flecs::world ecs;

    // Using flecs::entity_t directly would resolve to a uint64_t datatype, so
    // use flecs::Entity instead.
    ecs.component<TypeWithEntity>()
        .member(flecs::Entity, "e");

    flecs::entity foo = ecs.entity("Foo");

    // Create entity with PortableType
    flecs::entity e = ecs.entity()
        .set<TypeWithEntity>({ foo });

    // Convert PortableType component to flecs expression string
    const TypeWithEntity& c = e.get<TypeWithEntity>();
    std::cout << ecs.to_expr(&c).c_str() << "\n"; // {e: Foo}
}
