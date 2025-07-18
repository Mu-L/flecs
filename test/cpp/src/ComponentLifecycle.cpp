// Catch static asserts
#define flecs_static_assert(cond, str)\
    ecs_assert(cond, ECS_INVALID_OPERATION, str)

#include <cpp.h>

int Pod::ctor_invoked = 0;
int Pod::dtor_invoked = 0;
int Pod::copy_invoked = 0;
int Pod::move_invoked = 0;
int Pod::copy_ctor_invoked = 0;
int Pod::move_ctor_invoked = 0;

int CountNoDefaultCtor::ctor_invoked = 0;
int CountNoDefaultCtor::dtor_invoked = 0;
int CountNoDefaultCtor::copy_invoked = 0;
int CountNoDefaultCtor::move_invoked = 0;
int CountNoDefaultCtor::copy_ctor_invoked = 0;
int CountNoDefaultCtor::move_ctor_invoked = 0;

void ComponentLifecycle_ctor_on_add(void) {
    flecs::world world;

    world.component<Pod>();

    auto e = world.entity().add<Pod>();
    test_assert(e.id() != 0);
    test_assert(e.has<Pod>());

    const Pod *pod = e.try_get<Pod>();
    test_assert(pod != NULL);
    test_int(Pod::ctor_invoked, 1);
    test_int(Pod::dtor_invoked, 0);
    test_int(Pod::copy_invoked, 0);
    test_int(Pod::move_invoked, 0);
    
    test_int(pod->value, 10);
}

void ComponentLifecycle_dtor_on_remove(void) {
    flecs::world world;

    world.component<Pod>();

    auto e = world.entity().add<Pod>();
    test_assert(e.id() != 0);
    test_assert(e.has<Pod>());
    test_int(Pod::ctor_invoked, 1);

    e.remove<Pod>();
    test_assert(!e.has<Pod>());
    test_int(Pod::ctor_invoked, 1);
    test_int(Pod::dtor_invoked, 1);
    test_int(Pod::copy_invoked, 0);
    test_int(Pod::move_invoked, 0);
}

void ComponentLifecycle_move_on_add(void) {
    flecs::world world;

    world.component<Pod>();

    auto e = world.entity().add<Pod>();
    test_assert(e.id() != 0);
    test_assert(e.has<Pod>());

    const Pod *pod = e.try_get<Pod>();
    test_assert(pod != NULL);
    test_int(Pod::ctor_invoked, 1);
    test_int(Pod::dtor_invoked, 0);
    test_int(Pod::copy_invoked, 0);
    test_int(Pod::move_invoked, 0);
    test_int(Pod::copy_ctor_invoked, 0);
    test_int(Pod::move_ctor_invoked, 0);

    e.add<Position>();

    test_int(Pod::ctor_invoked, 1);
    test_int(Pod::dtor_invoked, 1);
    test_int(Pod::copy_invoked, 0);
    test_int(Pod::move_invoked, 0);
    test_int(Pod::copy_ctor_invoked, 0);
    test_int(Pod::move_ctor_invoked, 1);

    test_int(pod->value, 10);
}

void ComponentLifecycle_move_on_remove(void) {
    flecs::world world;

    world.component<Pod>();

    auto e = world.entity().add<Position>().add<Pod>();
    test_assert(e.id() != 0);
    test_assert(e.has<Pod>());

    const Pod *pod = e.try_get<Pod>();
    test_assert(pod != NULL);
    test_int(Pod::ctor_invoked, 1);
    test_int(Pod::dtor_invoked, 0);
    test_int(Pod::copy_invoked, 0);
    test_int(Pod::move_invoked, 0);
    test_int(Pod::copy_ctor_invoked, 0);
    test_int(Pod::move_ctor_invoked, 0);

    e.remove<Position>();

    test_int(Pod::ctor_invoked, 1);
    test_int(Pod::dtor_invoked, 1);
    test_int(Pod::copy_invoked, 0);
    test_int(Pod::move_invoked, 0);
    test_int(Pod::copy_ctor_invoked, 0);
    test_int(Pod::move_ctor_invoked, 1);

    test_int(pod->value, 10);
}

void ComponentLifecycle_copy_on_set(void) {
    flecs::world world;

    world.component<Pod>();

    auto e = world.entity();
    test_assert(e.id() != 0);

    e.set<Pod>({20});
    test_assert(e.has<Pod>());
    test_int(Pod::ctor_invoked, 2);
    test_int(Pod::dtor_invoked, 1);
    test_int(Pod::copy_invoked, 0);
    test_int(Pod::move_invoked, 1);
}

void ComponentLifecycle_copy_on_override(void) {
    flecs::world world;

    world.component<Pod>().add(flecs::OnInstantiate, flecs::Inherit);

    auto base = world.entity();
    test_assert(base.id() != 0);

    base.set<Pod>({10});
    test_int(Pod::ctor_invoked, 2);
    test_int(Pod::dtor_invoked, 1);
    test_int(Pod::copy_invoked, 0);
    test_int(Pod::copy_ctor_invoked, 0);
    Pod::ctor_invoked = 0;
    Pod::dtor_invoked = 0;
    Pod::copy_invoked = 0;
    Pod::move_invoked = 0;

    auto e = world.entity();
    test_assert(e.id() != 0);
    
    e.add(flecs::IsA, base);
    test_int(Pod::ctor_invoked, 0);

    e.add<Pod>();
    test_int(Pod::ctor_invoked, 0);
    test_int(Pod::dtor_invoked, 0);
    test_int(Pod::copy_invoked, 0);
    test_int(Pod::move_invoked, 0);
    test_int(Pod::copy_ctor_invoked, 1);

    const Pod *pod = e.try_get<Pod>();
    test_assert(pod != NULL);
    test_int(pod->value, 10);
}

void ComponentLifecycle_struct_w_string_add(void) {
    flecs::world world;

    auto e = world.entity().add<Struct_w_string>();
    test_assert(e.id() != 0);
    test_assert(e.has<Struct_w_string>());

    const Struct_w_string *str = e.try_get<Struct_w_string>();
    test_assert(str != NULL);
    test_assert(str->value == "");
}

void ComponentLifecycle_struct_w_string_remove(void) {
    flecs::world world;

    auto e = world.entity().add<Struct_w_string>();
    test_assert(e.id() != 0);
    test_assert(e.has<Struct_w_string>());

    e.remove<Struct_w_string>();
    test_assert(!e.has<Struct_w_string>());
}

void ComponentLifecycle_struct_w_string_set(void) {
    flecs::world world;

    auto e = world.entity()
        .set<Struct_w_string>({"Hello World"});
    test_assert(e.id() != 0);
    test_assert(e.has<Struct_w_string>());

    const Struct_w_string *str = e.try_get<Struct_w_string>();
    test_assert(str != NULL);
    test_assert(str->value == "Hello World");
}

void ComponentLifecycle_struct_w_string_override(void) {
    flecs::world world;

    auto base = world.entity();
    test_assert(base.id() != 0);

    base.set<Struct_w_string>({"Hello World"});

    auto e = world.entity();
    test_assert(e.id() != 0);
    
    e.add(flecs::IsA, base);

    e.add<Struct_w_string>();

    const Struct_w_string *str = e.try_get<Struct_w_string>();
    test_assert(str != NULL);
    test_assert(str->value == "Hello World");
}

void ComponentLifecycle_struct_w_string_add_2_remove(void) {
    flecs::world world;

    auto e1 = world.entity().add<Struct_w_string>();
    auto e2 = world.entity().add<Struct_w_string>();

    const Struct_w_string *str1 = e1.try_get<Struct_w_string>();
    test_assert(str1 != NULL);
    test_assert(str1->value == "");
    const Struct_w_string *str2 = e2.try_get<Struct_w_string>();
    test_assert(str2 != NULL);
    test_assert(str2->value == "");

    e1.remove<Struct_w_string>();
    str1 = e1.try_get<Struct_w_string>();
    test_assert(str1 == NULL);

    str2 = e2.try_get<Struct_w_string>();
    test_assert(str2 != NULL);
    test_assert(str2->value == "");

    e2.remove<Struct_w_string>();
    str2 = e2.try_get<Struct_w_string>();
    test_assert(str2 == NULL);
}

void ComponentLifecycle_struct_w_string_set_2_remove(void) {
    flecs::world world;

    auto e1 = world.entity().set<Struct_w_string>({"hello"});
    auto e2 = world.entity().set<Struct_w_string>({"world"});

    const Struct_w_string *str1 = e1.try_get<Struct_w_string>();
    test_assert(str1 != NULL);
    test_assert(str1->value == "hello");
    const Struct_w_string *str2 = e2.try_get<Struct_w_string>();
    test_assert(str2 != NULL);
    test_assert(str2->value == "world");

    e1.remove<Struct_w_string>();
    str1 = e1.try_get<Struct_w_string>();
    test_assert(str1 == NULL);

    str2 = e2.try_get<Struct_w_string>();
    test_assert(str2 != NULL);
    test_assert(str2->value == "world");

    e2.remove<Struct_w_string>();
    str2 = e2.try_get<Struct_w_string>();
    test_assert(str2 == NULL);
}

void ComponentLifecycle_struct_w_string_add_2_remove_w_tag(void) {
    flecs::world world;

    auto e1 = world.entity().add<Tag>().add<Struct_w_string>();
    auto e2 = world.entity().add<Tag>().add<Struct_w_string>();

    const Struct_w_string *str1 = e1.try_get<Struct_w_string>();
    test_assert(str1 != NULL);
    test_assert(str1->value == "");
    const Struct_w_string *str2 = e2.try_get<Struct_w_string>();
    test_assert(str2 != NULL);
    test_assert(str2->value == "");

    e1.remove<Struct_w_string>();
    str1 = e1.try_get<Struct_w_string>();
    test_assert(str1 == NULL);

    str2 = e2.try_get<Struct_w_string>();
    test_assert(str2 != NULL);
    test_assert(str2->value == "");

    e2.remove<Struct_w_string>();
    str2 = e2.try_get<Struct_w_string>();
    test_assert(str2 == NULL);
}

void ComponentLifecycle_struct_w_string_set_2_remove_w_tag(void) {
    flecs::world world;

    auto e1 = world.entity().add<Tag>().set<Struct_w_string>({"hello"});
    auto e2 = world.entity().add<Tag>().set<Struct_w_string>({"world"});

    const Struct_w_string *str1 = e1.try_get<Struct_w_string>();
    test_assert(str1 != NULL);
    test_assert(str1->value == "hello");
    const Struct_w_string *str2 = e2.try_get<Struct_w_string>();
    test_assert(str2 != NULL);
    test_assert(str2->value == "world");

    e1.remove<Struct_w_string>();
    str1 = e1.try_get<Struct_w_string>();
    test_assert(str1 == NULL);

    str2 = e2.try_get<Struct_w_string>();
    test_assert(str2 != NULL);
    test_assert(str2->value == "world");

    e2.remove<Struct_w_string>();
    str2 = e2.try_get<Struct_w_string>();
    test_assert(str2 == NULL);
}

void ComponentLifecycle_struct_w_vector_add(void) {
    flecs::world world;

    auto e = world.entity().add<Struct_w_vector>();
    test_assert(e.has<Struct_w_vector>());

    const Struct_w_vector *ptr = e.try_get<Struct_w_vector>();
    test_assert(ptr != NULL);
    test_int(ptr->value.size(), 0);
}

void ComponentLifecycle_struct_w_vector_remove(void) {
    flecs::world world;

    auto e = world.entity().add<Struct_w_vector>();
    test_assert(e.has<Struct_w_vector>());

    e.remove<Struct_w_vector>();
    test_assert(!e.has<Struct_w_vector>());
}

void ComponentLifecycle_struct_w_vector_set(void) {
    flecs::world world;

    auto e = world.entity().set<Struct_w_vector>({std::vector<int>{1, 2}});
    test_assert(e.has<Struct_w_vector>());

    const Struct_w_vector *ptr = e.try_get<Struct_w_vector>();
    test_assert(ptr != NULL);
    test_int(ptr->value.size(), 2);
    test_int(ptr->value.at(0), 1);
    test_int(ptr->value.at(1), 2);
}

void ComponentLifecycle_struct_w_vector_override(void) {
    flecs::world world;

    auto base = world.entity().set<Struct_w_vector>({std::vector<int>{1, 2}});
    test_assert(base.has<Struct_w_vector>());

    auto e = world.entity().is_a(base).add<Struct_w_vector>();
    test_assert(e.has<Struct_w_vector>());

    const Struct_w_vector *ptr = base.try_get<Struct_w_vector>();
    test_assert(ptr != NULL);
    test_int(ptr->value.size(), 2);
    test_int(ptr->value.at(0), 1);
    test_int(ptr->value.at(1), 2);

    ptr = e.try_get<Struct_w_vector>();
    test_assert(ptr != NULL);
    test_int(ptr->value.size(), 2);
    test_int(ptr->value.at(0), 1);
    test_int(ptr->value.at(1), 2);
}

void ComponentLifecycle_struct_w_vector_add_2_remove(void) {
    flecs::world world;

    auto e1 = world.entity().add<Struct_w_vector>();
    auto e2 = world.entity().add<Struct_w_vector>();

    const Struct_w_vector *ptr1 = e1.try_get<Struct_w_vector>();
    test_assert(ptr1 != NULL);
    test_int(ptr1->value.size(), 0);
    const Struct_w_vector *ptr2 = e2.try_get<Struct_w_vector>();
    test_assert(ptr2 != NULL);
    test_int(ptr2->value.size(), 0);

    e1.remove<Struct_w_vector>();
    ptr1 = e1.try_get<Struct_w_vector>();
    test_assert(ptr1 == NULL);

    ptr2 = e2.try_get<Struct_w_vector>();
    test_assert(ptr2 != NULL);
    test_int(ptr2->value.size(), 0);

    e2.remove<Struct_w_vector>();
    ptr2 = e2.try_get<Struct_w_vector>();
    test_assert(ptr2 == NULL);
}

void ComponentLifecycle_struct_w_vector_set_2_remove(void) {
    flecs::world world;

    auto e1 = world.entity().set<Struct_w_vector>({std::vector<int>{1, 2}});
    auto e2 = world.entity().set<Struct_w_vector>({std::vector<int>{3, 4}});

    const Struct_w_vector *ptr1 = e1.try_get<Struct_w_vector>();
    test_assert(ptr1 != NULL);
    test_int(ptr1->value.size(), 2);
    test_int(ptr1->value.at(0), 1);
    test_int(ptr1->value.at(1), 2);
    const Struct_w_vector *ptr2 = e2.try_get<Struct_w_vector>();
    test_assert(ptr2 != NULL);
    test_int(ptr2->value.size(), 2);
    test_int(ptr2->value.at(0), 3);
    test_int(ptr2->value.at(1), 4);

    e1.remove<Struct_w_vector>();
    ptr1 = e1.try_get<Struct_w_vector>();
    test_assert(ptr1 == NULL);

    ptr2 = e2.try_get<Struct_w_vector>();
    test_assert(ptr2 != NULL);
    test_int(ptr2->value.size(), 2);
    test_int(ptr2->value.at(0), 3);
    test_int(ptr2->value.at(1), 4);

    e2.remove<Struct_w_vector>();
    ptr2 = e2.try_get<Struct_w_vector>();
    test_assert(ptr2 == NULL);
}

void ComponentLifecycle_struct_w_vector_add_2_remove_w_tag(void) {
    flecs::world world;

    auto e1 = world.entity().add<Tag>().add<Struct_w_vector>();
    auto e2 = world.entity().add<Tag>().add<Struct_w_vector>();

    const Struct_w_vector *ptr1 = e1.try_get<Struct_w_vector>();
    test_assert(ptr1 != NULL);
    test_int(ptr1->value.size(), 0);
    const Struct_w_vector *ptr2 = e2.try_get<Struct_w_vector>();
    test_assert(ptr2 != NULL);
    test_int(ptr2->value.size(), 0);

    e1.remove<Struct_w_vector>();
    ptr1 = e1.try_get<Struct_w_vector>();
    test_assert(ptr1 == NULL);

    ptr2 = e2.try_get<Struct_w_vector>();
    test_assert(ptr2 != NULL);
    test_int(ptr2->value.size(), 0);

    e2.remove<Struct_w_vector>();
    ptr2 = e2.try_get<Struct_w_vector>();
    test_assert(ptr2 == NULL);
}

void ComponentLifecycle_struct_w_vector_set_2_remove_w_tag(void) {
    flecs::world world;

    auto e1 = world.entity().add<Tag>().set<Struct_w_vector>({std::vector<int>{1, 2}});
    auto e2 = world.entity().add<Tag>().set<Struct_w_vector>({std::vector<int>{3, 4}});

    const Struct_w_vector *ptr1 = e1.try_get<Struct_w_vector>();
    test_assert(ptr1 != NULL);
    test_int(ptr1->value.size(), 2);
    test_int(ptr1->value.at(0), 1);
    test_int(ptr1->value.at(1), 2);
    const Struct_w_vector *ptr2 = e2.try_get<Struct_w_vector>();
    test_assert(ptr2 != NULL);
    test_int(ptr2->value.size(), 2);
    test_int(ptr2->value.at(0), 3);
    test_int(ptr2->value.at(1), 4);

    e1.remove<Struct_w_vector>();
    ptr1 = e1.try_get<Struct_w_vector>();
    test_assert(ptr1 == NULL);

    ptr2 = e2.try_get<Struct_w_vector>();
    test_assert(ptr2 != NULL);
    test_int(ptr2->value.size(), 2);
    test_int(ptr2->value.at(0), 3);
    test_int(ptr2->value.at(1), 4);

    e2.remove<Struct_w_vector>();
    ptr2 = e2.try_get<Struct_w_vector>();
    test_assert(ptr2 == NULL);
}

void ComponentLifecycle_ensure_new(void) {
    flecs::world world;

    world.component<Pod>();

    auto e = world.entity();
    test_assert(e.id() != 0);

    e.ensure<Pod>();

    Pod::ctor_invoked = 1;
    Pod::dtor_invoked = 0;
    Pod::copy_invoked = 0;
    Pod::move_invoked = 0;

    e.modified<Pod>();

    Pod::ctor_invoked = 1;
    Pod::dtor_invoked = 0;
    Pod::copy_invoked = 0;
    Pod::move_invoked = 0;    
}

void ComponentLifecycle_ensure_existing(void) {
    flecs::world world;

    world.component<Pod>();

    auto e = world.entity();
    test_assert(e.id() != 0);

    e.ensure<Pod>();

    Pod::ctor_invoked = 1;
    Pod::dtor_invoked = 0;
    Pod::copy_invoked = 0;
    Pod::move_invoked = 0;

    e.ensure<Pod>();

    /* Repeated calls to ensure should not invoke constructor */
    Pod::ctor_invoked = 1;
    Pod::dtor_invoked = 0;
    Pod::copy_invoked = 0;
    Pod::move_invoked = 0;
}

void ComponentLifecycle_implicit_component(void) {
    flecs::world world;

    auto e = world.entity().add<Pod>();
    test_assert(e.id() != 0);
    test_assert(e.has<Pod>());
    test_int(Pod::ctor_invoked, 1);

    const Pod *pod = e.try_get<Pod>();
    test_assert(pod != NULL);

    test_int(pod->value, 10);

    test_int(Pod::ctor_invoked, 1);
    test_int(Pod::dtor_invoked, 0);
    test_int(Pod::copy_invoked, 0);
    test_int(Pod::move_invoked, 0);

    world.entity().add<Pod>();

    test_int(Pod::ctor_invoked, 2);
    test_int(Pod::move_ctor_invoked, 0);
    test_int(Pod::move_invoked, 0);

    world.entity().add<Pod>();

    test_int(Pod::ctor_invoked, 3);
    test_int(Pod::move_ctor_invoked, 2);
    test_int(Pod::move_invoked, 0);
}

void ComponentLifecycle_implicit_after_query(void) {
    flecs::world world;

    world.query<Pod>();

    auto e = world.entity().add<Pod>();
    test_assert(e.id() != 0);
    test_assert(e.has<Pod>());
    test_int(Pod::ctor_invoked, 1);

    const Pod *pod = e.try_get<Pod>();
    test_assert(pod != NULL);

    test_int(pod->value, 10);

    test_int(Pod::ctor_invoked, 1);
    test_int(Pod::dtor_invoked, 0);
    test_int(Pod::copy_invoked, 0);
    test_int(Pod::move_invoked, 0);

    world.entity().add<Pod>();

    test_int(Pod::ctor_invoked, 2);
    test_int(Pod::move_ctor_invoked, 0);
    test_int(Pod::move_invoked, 0); 

    world.entity().add<Pod>();

    test_int(Pod::ctor_invoked, 3);
    test_int(Pod::move_ctor_invoked, 2);
    test_int(Pod::move_invoked, 0); 
}

template <typename T>
static void try_add(flecs::world& ecs) {
    flecs::entity e = ecs.entity().add<T>();
    
    test_assert(e.has<T>());

    const T *ptr = e.try_get<T>();
    test_int(ptr->x_, 99);

    e.remove<T>();
    test_assert(!e.has<T>());
}

template <typename T>
static void try_add_relation(flecs::world& ecs) {
    flecs::entity obj = ecs.entity();

    flecs::entity e = ecs.entity().add<T>(obj);
    test_assert(e.has<T>());

    const T *ptr = e.try_get<T>();
    test_int(ptr->x_, 89);

    e.remove<T>();
    test_assert(!e.has<T>());
}

template <typename T>
static void try_add_second(flecs::world& ecs) {
    flecs::entity rel = ecs.entity();

    flecs::entity e = ecs.entity().add_second<T>(rel);
    test_assert(e.has<T>());

    const T *ptr = e.try_get<T>();
    test_int(ptr->x_, 89);

    e.remove<T>();
    test_assert(!e.has<T>());
}

template <typename T>
static void try_set(flecs::world& ecs) {
    flecs::entity e = ecs.entity().set<T>({10});

    const T *ptr = e.try_get<T>();
    test_int(ptr->x_, 10);
}

template <typename T>
static void try_emplace(flecs::world& ecs) {
    flecs::entity e = ecs.entity().emplace<T>(10);

    const T *ptr = e.try_get<T>();
    test_int(ptr->x_, 10);
}

template <typename T>
static void try_set_default(flecs::world& ecs) {
    flecs::entity e = ecs.entity().set(T());

    const T *ptr = e.try_get<T>();
    test_int(ptr->x_, 99);

    e.remove<T>();
}

void ComponentLifecycle_deleted_copy(void) {
    flecs::world ecs;

    ecs.component<NoCopy>();

    try_add<NoCopy>(ecs);

    try_set<NoCopy>(ecs);
}

void ComponentLifecycle_no_default_ctor_emplace(void) {
    flecs::world ecs;

    ecs.component<NoDefaultCtor>();

    try_emplace<NoDefaultCtor>(ecs);
}

void ComponentLifecycle_default_init(void) {
    flecs::world ecs;

    ecs.component<DefaultInit>();

    try_add<DefaultInit>(ecs);

    try_set<DefaultInit>(ecs);   
}

void ComponentLifecycle_no_default_ctor_add(void) {
    install_test_abort();

    flecs::world ecs;

    ecs.component<NoDefaultCtor>();

    test_expect_abort();

    try_add<NoDefaultCtor>(ecs);  
}

void ComponentLifecycle_no_default_ctor_add_relation(void) {
    install_test_abort();

    flecs::world ecs;

    ecs.component<NoDefaultCtor>();

    test_expect_abort();

    try_add_relation<NoDefaultCtor>(ecs); 
}

void ComponentLifecycle_no_default_ctor_add_second(void) {
    install_test_abort();

    flecs::world ecs;

    ecs.component<NoDefaultCtor>();

    test_expect_abort();

    try_add_second<NoDefaultCtor>(ecs);
}

void ComponentLifecycle_no_default_ctor_set(void) {
    install_test_abort();

    flecs::world ecs;

    ecs.component<NoDefaultCtor>();

    test_expect_abort();

    try_set<NoDefaultCtor>(ecs);  
}

void ComponentLifecycle_no_copy_ctor(void) {
    flecs::world ecs;

    ecs.component<NoCopyCtor>();

    try_add<NoCopyCtor>(ecs);

    try_set<NoCopyCtor>(ecs); 
}

void ComponentLifecycle_no_move(void) {
    flecs::world ecs;

    ecs.component<NoMove>();
    ecs.component<Position>();

    test_assert(ecs.component<NoMove>().has(flecs::Sparse));

    flecs::entity e = ecs.entity().add<NoMove>();
    const NoMove *ptr = e.try_get<NoMove>();
    test_assert(ptr != NULL);
    test_int(ptr->x_, 99);

    e.add<Position>();

    test_assert(ptr == e.try_get<NoMove>());
    test_int(ptr->x_, 99);
}

void ComponentLifecycle_no_move_ctor(void) {
    flecs::world ecs;

    ecs.component<NoMoveCtor>();
    ecs.component<Position>();

    test_assert(ecs.component<NoMoveCtor>().has(flecs::Sparse));

    flecs::entity e = ecs.entity().add<NoMoveCtor>();
    const NoMoveCtor *ptr = e.try_get<NoMoveCtor>();
    test_assert(ptr != NULL);
    test_int(ptr->x_, 99);

    e.add<Position>();

    test_assert(ptr == e.try_get<NoMoveCtor>());
    test_int(ptr->x_, 99);
}

void ComponentLifecycle_no_move_assign(void) {
    flecs::world ecs;

    ecs.component<NoMoveAssign>();
    ecs.component<Position>();

    test_assert(ecs.component<NoMoveAssign>().has(flecs::Sparse));

    flecs::entity e = ecs.entity().add<NoMoveAssign>();
    const NoMoveAssign *ptr = e.try_get<NoMoveAssign>();
    test_assert(ptr != NULL);
    test_int(ptr->x_, 99);

    e.add<Position>();

    test_assert(ptr == e.try_get<NoMoveAssign>());
    test_int(ptr->x_, 99);
}

void ComponentLifecycle_no_copy_assign(void) {
    flecs::world ecs;

    ecs.component<NoCopyAssign>();

    try_add<NoCopyAssign>(ecs);

    try_set<NoCopyAssign>(ecs);
}

void ComponentLifecycle_no_copy(void) {
    flecs::world ecs;

    ecs.component<NoCopy>();

    try_add<NoCopy>(ecs);

    try_set<NoCopy>(ecs);
}

void ComponentLifecycle_no_dtor(void) {
    install_test_abort();

    flecs::world ecs;

    test_expect_abort();

    ecs.component<NoDtor>();
}

void ComponentLifecycle_default_ctor_w_value_ctor(void) {
    flecs::world ecs;

    ecs.component<DefaultCtorValueCtor>();

    try_add<DefaultCtorValueCtor>(ecs);

    try_set<DefaultCtorValueCtor>(ecs);

    try_set_default<FlecsCtorDefaultCtor>(ecs);
}

void ComponentLifecycle_no_default_ctor_move_ctor_on_set(void) {
    flecs::world ecs;

    ecs.component<CountNoDefaultCtor>();

    // Emplace, construct
    auto e = ecs.entity().emplace<CountNoDefaultCtor>(10);
    test_assert(e.has<CountNoDefaultCtor>());

    const CountNoDefaultCtor* ptr = e.try_get<CountNoDefaultCtor>();
    test_assert(ptr != NULL);
    test_int(ptr->value, 10);

    test_int(CountNoDefaultCtor::ctor_invoked, 1);
    test_int(CountNoDefaultCtor::dtor_invoked, 0);
    test_int(CountNoDefaultCtor::copy_invoked, 0);
    test_int(CountNoDefaultCtor::move_invoked, 0);
    test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
    test_int(CountNoDefaultCtor::move_ctor_invoked, 0);

    // Set, move assign
    e.set<CountNoDefaultCtor>({10});

    test_int(CountNoDefaultCtor::ctor_invoked, 2);
    test_int(CountNoDefaultCtor::dtor_invoked, 1);
    test_int(CountNoDefaultCtor::copy_invoked, 0);
    test_int(CountNoDefaultCtor::move_invoked, 1);
    test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
    test_int(CountNoDefaultCtor::move_ctor_invoked, 0);    
}

void ComponentLifecycle_emplace_w_ctor(void) {
    flecs::world ecs;

    auto e = ecs.entity()
        .emplace<Pod>(10);

    test_int(Pod::ctor_invoked, 1);
    test_int(Pod::dtor_invoked, 0);

    const Pod *ptr = e.try_get<Pod>();
    test_assert(ptr != NULL);
    test_int(ptr->value, 10);

    test_int(Pod::ctor_invoked, 1);
    test_int(Pod::dtor_invoked, 0);    
}

void ComponentLifecycle_emplace_no_default_ctor(void) {
    flecs::world ecs;

    auto e = ecs.entity()
        .emplace<CountNoDefaultCtor>(10);

    test_int(CountNoDefaultCtor::ctor_invoked, 1);
    test_int(CountNoDefaultCtor::dtor_invoked, 0);

    const CountNoDefaultCtor *ptr = e.try_get<CountNoDefaultCtor>();
    test_assert(ptr != NULL);
    test_int(ptr->value, 10);

    test_int(CountNoDefaultCtor::ctor_invoked, 1);
    test_int(CountNoDefaultCtor::dtor_invoked, 0); 
}

void ComponentLifecycle_emplace_defer_use_move_ctor(void) {
    {
        flecs::world ecs;

        auto e = ecs.entity();

        ecs.defer_begin();
        e.emplace<CountNoDefaultCtor>(10);
        test_assert(!e.has<CountNoDefaultCtor>());
        test_int(CountNoDefaultCtor::ctor_invoked, 1);
        test_int(CountNoDefaultCtor::dtor_invoked, 0);
        test_int(CountNoDefaultCtor::move_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 0);
        ecs.defer_end();

        test_assert(e.has<CountNoDefaultCtor>());
        test_int(CountNoDefaultCtor::ctor_invoked, 1);
        test_int(CountNoDefaultCtor::dtor_invoked, 1);
        test_int(CountNoDefaultCtor::move_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 1);

        const CountNoDefaultCtor *ptr = e.try_get<CountNoDefaultCtor>();
        test_assert(ptr != NULL);
        test_int(ptr->value, 10);

        test_int(CountNoDefaultCtor::ctor_invoked, 1);
        test_int(CountNoDefaultCtor::dtor_invoked, 1);
        test_int(CountNoDefaultCtor::move_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 1);
    }

    test_int(CountNoDefaultCtor::ctor_invoked, 1);
    test_int(CountNoDefaultCtor::dtor_invoked, 2);
    test_int(CountNoDefaultCtor::move_invoked, 0);
    test_int(CountNoDefaultCtor::move_ctor_invoked, 1);
}

void ComponentLifecycle_emplace_existing(void) {
    install_test_abort();

    flecs::world ecs;

    auto e = ecs.entity()
        .emplace<Pod>(10);

    const Pod *ptr = e.try_get<Pod>();
    test_assert(ptr != NULL);
    test_int(ptr->value, 10);    

    test_int(Pod::ctor_invoked, 1);
    test_int(Pod::dtor_invoked, 0);

    test_expect_abort();
    e.emplace<Pod>(20);
}

void ComponentLifecycle_emplace_singleton(void) {
    flecs::world ecs;

    ecs.emplace<Pod>(10);

    test_int(Pod::ctor_invoked, 1);
    test_int(Pod::dtor_invoked, 0);

    const Pod *ptr = ecs.try_get<Pod>();
    test_assert(ptr != NULL);
    test_int(ptr->value, 10);

    test_int(Pod::ctor_invoked, 1);
    test_int(Pod::dtor_invoked, 0); 
}

class CtorDtorNonTrivial {
public:
    CtorDtorNonTrivial(int x) : x_(x) {
        ctor_invoked ++;
    }

    ~CtorDtorNonTrivial() {
        dtor_invoked ++;
        dtor_value = x_;
    }

    int x_;
    std::string str_;

    static int ctor_invoked;
    static int dtor_invoked;
    static int dtor_value;
};

int CtorDtorNonTrivial::ctor_invoked;
int CtorDtorNonTrivial::dtor_invoked;
int CtorDtorNonTrivial::dtor_value;

void ComponentLifecycle_dtor_w_non_trivial_implicit_move(void) {
    flecs::world ecs;

    test_bool(std::is_trivially_move_assignable<CtorDtorNonTrivial>::value, false);
    test_bool(std::is_move_assignable<CtorDtorNonTrivial>::value, true);

    auto e_1 = ecs.entity().emplace<CtorDtorNonTrivial>(10);
    auto e_2 = ecs.entity().emplace<CtorDtorNonTrivial>(20);

    const CtorDtorNonTrivial *ptr = e_1.try_get<CtorDtorNonTrivial>();
    test_assert(ptr != nullptr);
    test_int(ptr->x_, 10);

    ptr = e_2.try_get<CtorDtorNonTrivial>();
    test_assert(ptr != nullptr);
    test_int(ptr->x_, 20);

    test_int(CtorDtorNonTrivial::ctor_invoked, 2);

    // Moves e_2 to e_1
    e_1.destruct();

    test_int(CtorDtorNonTrivial::ctor_invoked, 2);
    test_int(CtorDtorNonTrivial::dtor_invoked, 1);

    // Counter intuitive but correct. The class is not trivially movable, so
    // the move assignment should take care of cleaning up e_1 (10). That still
    // leaves the original e_2 which was moved from, but not destructed.
    //
    // In a real application the class should probably implement its own move
    // assignment to ensure correct destructor behavior.
    test_int(CtorDtorNonTrivial::dtor_value, 20);
}

class CtorDtor_w_MoveAssign {
public:
    CtorDtor_w_MoveAssign(int x) : x_(x) {
        ctor_invoked ++;
    }

    ~CtorDtor_w_MoveAssign() {
        dtor_invoked ++;
        dtor_value = x_;
    }

    CtorDtor_w_MoveAssign(const CtorDtor_w_MoveAssign& obj) = default;
    CtorDtor_w_MoveAssign(CtorDtor_w_MoveAssign&& obj) = default;
    CtorDtor_w_MoveAssign& operator=(const CtorDtor_w_MoveAssign& obj) = default;

    CtorDtor_w_MoveAssign& operator=(CtorDtor_w_MoveAssign&& obj) noexcept {
        move_value = this->x_;

        this->x_ = obj.x_;
        obj.x_ = 0;
        return *this;
    }    

    int x_;
    std::string str_;

    static int ctor_invoked;
    static int dtor_invoked;
    static int dtor_value;
    static int move_value;
};

int CtorDtor_w_MoveAssign::ctor_invoked;
int CtorDtor_w_MoveAssign::dtor_invoked;
int CtorDtor_w_MoveAssign::dtor_value;
int CtorDtor_w_MoveAssign::move_value;

void ComponentLifecycle_dtor_w_non_trivial_explicit_move(void) {
    flecs::world ecs;

    test_bool(std::is_trivially_move_assignable<CtorDtor_w_MoveAssign>::value, false);
    test_bool(std::is_move_assignable<CtorDtor_w_MoveAssign>::value, true);

    auto e_1 = ecs.entity().emplace<CtorDtor_w_MoveAssign>(10);
    auto e_2 = ecs.entity().emplace<CtorDtor_w_MoveAssign>(20);

    const CtorDtor_w_MoveAssign *ptr = e_1.try_get<CtorDtor_w_MoveAssign>();
    test_assert(ptr != nullptr);
    test_int(ptr->x_, 10);

    ptr = e_2.try_get<CtorDtor_w_MoveAssign>();
    test_assert(ptr != nullptr);
    test_int(ptr->x_, 20);

    test_int(CtorDtor_w_MoveAssign::ctor_invoked, 2);

    // Moves e_2 to e_1
    e_1.destruct();

    test_int(CtorDtor_w_MoveAssign::ctor_invoked, 2);
    test_int(CtorDtor_w_MoveAssign::dtor_invoked, 1);

    test_int(CtorDtor_w_MoveAssign::move_value, 10);
    test_int(CtorDtor_w_MoveAssign::dtor_value, 0);
}

void ComponentLifecycle_grow_no_default_ctor(void) {
	{
        flecs::world world;

        world.component<CountNoDefaultCtor>();

        auto e1 = world.entity().emplace<CountNoDefaultCtor>(1);
        auto e2 = world.entity().emplace<CountNoDefaultCtor>(2);

        test_int(CountNoDefaultCtor::ctor_invoked, 2);
        test_int(CountNoDefaultCtor::dtor_invoked, 0);
        test_int(CountNoDefaultCtor::copy_invoked, 0);
        test_int(CountNoDefaultCtor::move_invoked, 0);
        test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 0);

        auto e3 = world.entity().emplace<CountNoDefaultCtor>(3);
        
        test_int(CountNoDefaultCtor::ctor_invoked, 3);
        test_int(CountNoDefaultCtor::dtor_invoked, 2);
        test_int(CountNoDefaultCtor::copy_invoked, 0);
        test_int(CountNoDefaultCtor::move_invoked, 0);
        test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 2);

        test_assert(e1.has<CountNoDefaultCtor>());
        test_assert(e2.has<CountNoDefaultCtor>());
        test_assert(e3.has<CountNoDefaultCtor>());

        test_int(e1.try_get<CountNoDefaultCtor>()->value, 1);
        test_int(e2.try_get<CountNoDefaultCtor>()->value, 2);
        test_int(e3.try_get<CountNoDefaultCtor>()->value, 3);
    }

    test_int(CountNoDefaultCtor::ctor_invoked, 3);
    test_int(CountNoDefaultCtor::dtor_invoked, 5);
    test_int(CountNoDefaultCtor::copy_invoked, 0);
    test_int(CountNoDefaultCtor::move_invoked, 0);
    test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
    test_int(CountNoDefaultCtor::move_ctor_invoked, 2);
}

void ComponentLifecycle_grow_no_default_ctor_move(void) {
    {
        flecs::world world;

        world.component<CountNoDefaultCtor>();
        world.component<Tag>();

        auto e1 = world.entity().emplace<CountNoDefaultCtor>(1);
        auto e2 = world.entity().emplace<CountNoDefaultCtor>(2);

        test_int(CountNoDefaultCtor::ctor_invoked, 2);
        test_int(CountNoDefaultCtor::dtor_invoked, 0);
        test_int(CountNoDefaultCtor::copy_invoked, 0);
        test_int(CountNoDefaultCtor::move_invoked, 0);
        test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 0);

        CountNoDefaultCtor::reset();
        auto e3 = world.entity().emplace<CountNoDefaultCtor>(3);

        test_int(CountNoDefaultCtor::ctor_invoked, 1);
        test_int(CountNoDefaultCtor::dtor_invoked, 2);
        test_int(CountNoDefaultCtor::copy_invoked, 0);
        test_int(CountNoDefaultCtor::move_invoked, 0);
        test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 2);

        test_assert(e1.has<CountNoDefaultCtor>());
        test_assert(e2.has<CountNoDefaultCtor>());
        test_assert(e3.has<CountNoDefaultCtor>());

        test_int(e1.try_get<CountNoDefaultCtor>()->value, 1);
        test_int(e2.try_get<CountNoDefaultCtor>()->value, 2);
        test_int(e3.try_get<CountNoDefaultCtor>()->value, 3);

        CountNoDefaultCtor::reset();
        e1.add<Tag>();

        test_int(CountNoDefaultCtor::ctor_invoked, 0);
        test_int(CountNoDefaultCtor::dtor_invoked, 1);
        test_int(CountNoDefaultCtor::copy_invoked, 0);
        test_int(CountNoDefaultCtor::move_invoked, 1);
        test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 1);

        CountNoDefaultCtor::reset();
        e2.add<Tag>();

        test_int(CountNoDefaultCtor::ctor_invoked, 0);
        test_int(CountNoDefaultCtor::dtor_invoked, 1);
        test_int(CountNoDefaultCtor::copy_invoked, 0);
        test_int(CountNoDefaultCtor::move_invoked, 0);
        test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 1);

        CountNoDefaultCtor::reset();
        e3.add<Tag>();

        test_int(CountNoDefaultCtor::ctor_invoked, 0);
        test_int(CountNoDefaultCtor::dtor_invoked, 3);
        test_int(CountNoDefaultCtor::copy_invoked, 0);
        test_int(CountNoDefaultCtor::move_invoked, 0);
        test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 3);

        CountNoDefaultCtor::reset();
    }

    test_int(CountNoDefaultCtor::ctor_invoked, 0);
    test_int(CountNoDefaultCtor::dtor_invoked, 3);
    test_int(CountNoDefaultCtor::copy_invoked, 0);
    test_int(CountNoDefaultCtor::move_invoked, 0);
    test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
    test_int(CountNoDefaultCtor::move_ctor_invoked, 0);
}

void ComponentLifecycle_grow_no_default_ctor_move_w_component(void) {
    {
        flecs::world world;

        world.component<CountNoDefaultCtor>();
        world.component<Position>();

        auto e1 = world.entity().emplace<CountNoDefaultCtor>(1);
        auto e2 = world.entity().emplace<CountNoDefaultCtor>(2);

        test_int(CountNoDefaultCtor::ctor_invoked, 2);
        test_int(CountNoDefaultCtor::dtor_invoked, 0);
        test_int(CountNoDefaultCtor::copy_invoked, 0);
        test_int(CountNoDefaultCtor::move_invoked, 0);
        test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 0);

        CountNoDefaultCtor::reset();
        auto e3 = world.entity().emplace<CountNoDefaultCtor>(3);

        test_int(CountNoDefaultCtor::ctor_invoked, 1);
        test_int(CountNoDefaultCtor::dtor_invoked, 2);
        test_int(CountNoDefaultCtor::copy_invoked, 0);
        test_int(CountNoDefaultCtor::move_invoked, 0);
        test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 2);

        test_assert(e1.has<CountNoDefaultCtor>());
        test_assert(e2.has<CountNoDefaultCtor>());
        test_assert(e3.has<CountNoDefaultCtor>());

        test_int(e1.try_get<CountNoDefaultCtor>()->value, 1);
        test_int(e2.try_get<CountNoDefaultCtor>()->value, 2);
        test_int(e3.try_get<CountNoDefaultCtor>()->value, 3);

        CountNoDefaultCtor::reset();
        e1.add<Position>();

        test_int(CountNoDefaultCtor::ctor_invoked, 0);
        test_int(CountNoDefaultCtor::dtor_invoked, 1);
        test_int(CountNoDefaultCtor::copy_invoked, 0);
        test_int(CountNoDefaultCtor::move_invoked, 1);
        test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 1);

        CountNoDefaultCtor::reset();
        e2.add<Position>(); // e2 is last element, e3 got moved to e1's location

        test_int(CountNoDefaultCtor::ctor_invoked, 0);
        test_int(CountNoDefaultCtor::dtor_invoked, 1);
        test_int(CountNoDefaultCtor::copy_invoked, 0);
        test_int(CountNoDefaultCtor::move_invoked, 0);
        test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 1);

        CountNoDefaultCtor::reset();
        e3.add<Position>();

        test_int(CountNoDefaultCtor::ctor_invoked, 0);
        test_int(CountNoDefaultCtor::dtor_invoked, 3); /* 2 (e1,e2) for resize, 1 (e3) for moved away storage */
        test_int(CountNoDefaultCtor::copy_invoked, 0);
        test_int(CountNoDefaultCtor::move_invoked, 0);
        test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 3); /* resize */

        CountNoDefaultCtor::reset();
    }

    test_int(CountNoDefaultCtor::ctor_invoked, 0);
    test_int(CountNoDefaultCtor::dtor_invoked, 3);
    test_int(CountNoDefaultCtor::copy_invoked, 0);
    test_int(CountNoDefaultCtor::move_invoked, 0);
    test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
    test_int(CountNoDefaultCtor::move_ctor_invoked, 0);
}

void ComponentLifecycle_delete_no_default_ctor(void) {
	{
        flecs::world world;

        world.component<CountNoDefaultCtor>();

        auto e1 = world.entity().emplace<CountNoDefaultCtor>(1);
        auto e2 = world.entity().emplace<CountNoDefaultCtor>(2);
        auto e3 = world.entity().emplace<CountNoDefaultCtor>(3);
        
        test_int(CountNoDefaultCtor::ctor_invoked, 3);
        test_int(CountNoDefaultCtor::dtor_invoked, 2);
        test_int(CountNoDefaultCtor::copy_invoked, 0);
        test_int(CountNoDefaultCtor::move_invoked, 0);
        test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 2);

        test_assert(e1.has<CountNoDefaultCtor>());
        test_assert(e2.has<CountNoDefaultCtor>());
        test_assert(e3.has<CountNoDefaultCtor>());

        test_int(e1.try_get<CountNoDefaultCtor>()->value, 1);
        test_int(e2.try_get<CountNoDefaultCtor>()->value, 2);
        test_int(e3.try_get<CountNoDefaultCtor>()->value, 3);

        e2.destruct();

        test_int(CountNoDefaultCtor::ctor_invoked, 3);
        test_int(CountNoDefaultCtor::dtor_invoked, 3);
        test_int(CountNoDefaultCtor::copy_invoked, 0);
        test_int(CountNoDefaultCtor::move_invoked, 1);
        test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 2);
    }

    test_int(CountNoDefaultCtor::ctor_invoked, 3);
    test_int(CountNoDefaultCtor::dtor_invoked, 5);
    test_int(CountNoDefaultCtor::copy_invoked, 0);
    test_int(CountNoDefaultCtor::move_invoked, 1);
    test_int(CountNoDefaultCtor::copy_ctor_invoked, 0);
    test_int(CountNoDefaultCtor::move_ctor_invoked, 2);
}

void ComponentLifecycle_on_add_hook(void) {
    int count = 0;

    {
        flecs::world ecs;

        ecs.component<Position>().on_add([&](Position& p) {
            count ++;
        });

        test_int(0, count);

        auto e = ecs.entity().add<Position>();
        test_int(1, count);

        e.add<Position>();
        test_int(1, count);
    }

    test_int(1, count);
}

void ComponentLifecycle_on_remove_hook(void) {
    int count = 0;
    
    {
        flecs::world ecs;

        ecs.component<Position>().on_remove([&](Position& p) {
            count ++;
        });

        test_int(0, count);

        auto e1 = ecs.entity().add<Position>();
        ecs.entity().add<Position>();
        test_int(0, count);

        e1.remove<Position>();
        test_int(1, count);
    }

    test_int(2, count);
}

void ComponentLifecycle_on_set_hook(void) {
    int count = 0;
    Position v = {0};

    {
        flecs::world ecs;

        ecs.component<Position>().on_set([&](Position& p) {
            count ++;
            v = p;
        });

        test_int(0, count);

        auto e1 = ecs.entity().add<Position>();
        test_int(0, count);

        e1.set<Position>({10, 20});
        test_int(1, count);
        test_int(10, v.x);
        test_int(20, v.y);

        ecs.entity().set<Position>({30, 40});
        test_int(2, count);
        test_int(30, v.x);
        test_int(40, v.y);
    }

    test_int(2, count);
}

void ComponentLifecycle_on_add_hook_w_entity(void) {
    int count = 0;
    flecs::entity e_arg;

    {
        flecs::world ecs;

        ecs.component<Position>().on_add([&](flecs::entity arg, Position& p) {
            e_arg = arg;
            count ++;
        });

        test_int(0, count);
        test_assert(e_arg == 0);

        auto e1 = ecs.entity().add<Position>();
        test_int(1, count);
        test_assert(e_arg == e1);

        e1.add<Position>();
        test_int(1, count);

        auto e2 = ecs.entity().add<Position>();
        test_int(2, count);
        test_assert(e_arg == e2);
    }

    test_int(2, count);
}

void ComponentLifecycle_on_remove_hook_w_entity(void) {
    int count = 0;
    flecs::entity e_arg;
    flecs::entity e2;
    
    {
        flecs::world ecs;

        ecs.component<Position>().on_remove([&](flecs::entity arg, Position& p){
            e_arg = arg;
            count ++;
        });

        test_int(0, count);
        test_assert(e_arg == 0);

        auto e1 = ecs.entity().add<Position>();
        e2 = ecs.entity().add<Position>();
        test_int(0, count);

        e1.remove<Position>();
        test_int(1, count);
        test_assert(e_arg == e1);
    }

    test_int(2, count);
    test_assert(e_arg == e2);
}

void ComponentLifecycle_on_set_hook_w_entity(void) {
    int count = 0;
    Position v = {0};
    flecs::entity e_arg;

    {
        flecs::world ecs;

        ecs.component<Position>().on_set([&](flecs::entity arg, Position& p) {
            count ++;
            v = p;
            e_arg = arg;
        });

        test_int(0, count);

        auto e1 = ecs.entity().add<Position>();
        test_int(0, count);

        e1.set<Position>({10, 20});
        test_int(1, count);
        test_assert(e_arg == e1);
        test_int(10, v.x);
        test_int(20, v.y);

        auto e2 = ecs.entity().set<Position>({30, 40});
        test_int(2, count);
        test_assert(e_arg == e2);
        test_int(30, v.x);
        test_int(40, v.y);
    }

    test_int(2, count);
}

void ComponentLifecycle_on_add_hook_sparse(void) {
    int count = 0;

    {
        flecs::world ecs;

        ecs.component<Position>().add(flecs::Sparse);
        ecs.component<Position>().on_add([&](Position& p) {
            count ++;
        });

        test_int(0, count);

        auto e = ecs.entity().add<Position>();
        test_int(1, count);

        e.add<Position>();
        test_int(1, count);
    }

    test_int(1, count);
}

void ComponentLifecycle_on_remove_hook_sparse(void) {
    int count = 0;
    
    {
        flecs::world ecs;

        ecs.component<Position>().add(flecs::Sparse);
        ecs.component<Position>().on_remove([&](Position& p) {
            count ++;
        });

        test_int(0, count);

        auto e1 = ecs.entity().add<Position>();
        ecs.entity().add<Position>();
        test_int(0, count);

        e1.remove<Position>();
        test_int(1, count);
    }

    test_int(2, count);
}

void ComponentLifecycle_on_set_hook_sparse(void) {
    int count = 0;
    Position v = {0};

    {
        flecs::world ecs;

        ecs.component<Position>().add(flecs::Sparse);
        ecs.component<Position>().on_set([&](Position& p) {
            count ++;
            v = p;
        });

        test_int(0, count);

        auto e1 = ecs.entity().add<Position>();
        test_int(0, count);

        e1.set<Position>({10, 20});
        test_int(1, count);
        test_int(10, v.x);
        test_int(20, v.y);

        ecs.entity().set<Position>({30, 40});
        test_int(2, count);
        test_int(30, v.x);
        test_int(40, v.y);
    }

    test_int(2, count);
}

void ComponentLifecycle_on_add_hook_sparse_w_entity(void) {
    int count = 0;
    flecs::entity e_arg;

    {
        flecs::world ecs;

        ecs.component<Position>().add(flecs::Sparse);
        ecs.component<Position>().on_add([&](flecs::entity arg, Position& p) {
            e_arg = arg;
            count ++;
        });

        test_int(0, count);
        test_assert(e_arg == 0);

        auto e1 = ecs.entity().add<Position>();
        test_int(1, count);
        test_assert(e_arg == e1);

        e1.add<Position>();
        test_int(1, count);

        auto e2 = ecs.entity().add<Position>();
        test_int(2, count);
        test_assert(e_arg == e2);
    }

    test_int(2, count);
}

void ComponentLifecycle_on_remove_hook_sparse_w_entity(void) {
    int count = 0;
    flecs::entity e_arg;
    flecs::entity e2;
    
    {
        flecs::world ecs;

        ecs.component<Position>().add(flecs::Sparse);
        ecs.component<Position>().on_remove([&](flecs::entity arg, Position& p){
            e_arg = arg;
            count ++;
        });

        test_int(0, count);
        test_assert(e_arg == 0);

        auto e1 = ecs.entity().add<Position>();
        e2 = ecs.entity().add<Position>();
        test_int(0, count);

        e1.remove<Position>();
        test_int(1, count);
        test_assert(e_arg == e1);
    }

    test_int(2, count);
    test_assert(e_arg == e2);
}

void ComponentLifecycle_on_set_hook_sparse_w_entity(void) {
    int count = 0;
    Position v = {0};
    flecs::entity e_arg;

    {
        flecs::world ecs;

        ecs.component<Position>().add(flecs::Sparse);
        ecs.component<Position>().on_set([&](flecs::entity arg, Position& p) {
            count ++;
            v = p;
            e_arg = arg;
        });

        test_int(0, count);

        auto e1 = ecs.entity().add<Position>();
        test_int(0, count);

        e1.set<Position>({10, 20});
        test_int(1, count);
        test_assert(e_arg == e1);
        test_int(10, v.x);
        test_int(20, v.y);

        auto e2 = ecs.entity().set<Position>({30, 40});
        test_int(2, count);
        test_assert(e_arg == e2);
        test_int(30, v.x);
        test_int(40, v.y);
    }

    test_int(2, count);
}

void ComponentLifecycle_on_add_hook_sparse_w_iter(void) {
    int count = 0;
    flecs::entity e_arg;

    {
        flecs::world ecs;

        ecs.component<Position>().add(flecs::Sparse);
        ecs.component<Position>()
            .on_add([&](flecs::iter& it, size_t row, Position& p) {
                e_arg = it.entity(row);
                count ++;
            });

        test_int(0, count);
        test_assert(e_arg == 0);

        auto e1 = ecs.entity().add<Position>();
        test_int(1, count);
        test_assert(e_arg == e1);

        e1.add<Position>();
        test_int(1, count);

        auto e2 = ecs.entity().add<Position>();
        test_int(2, count);
        test_assert(e_arg == e2);
    }

    test_int(2, count);
}

void ComponentLifecycle_on_remove_hook_sparse_w_iter(void) {
    int count = 0;
    flecs::entity e_arg;
    flecs::entity e2;
    
    {
        flecs::world ecs;

        ecs.component<Position>().add(flecs::Sparse);
        ecs.component<Position>()
            .on_remove([&](flecs::iter& it, size_t row, Position& p){
                e_arg = it.entity(row);
                count ++;
            });

        test_int(0, count);
        test_assert(e_arg == 0);

        auto e1 = ecs.entity().add<Position>();
        e2 = ecs.entity().add<Position>();
        test_int(0, count);

        e1.remove<Position>();
        test_int(1, count);
        test_assert(e_arg == e1);
    }

    test_int(2, count);
    test_assert(e_arg == e2);
}

void ComponentLifecycle_on_set_hook_sparse_w_iter(void) {
    int count = 0;
    Position v = {0};
    flecs::entity e_arg;

    {
        flecs::world ecs;

        ecs.component<Position>().add(flecs::Sparse);
        ecs.component<Position>()
            .on_set([&](flecs::iter& it, size_t row, Position& p) {
                count ++;
                v = p;
                e_arg = it.entity(row);
            });

        test_int(0, count);

        auto e1 = ecs.entity().add<Position>();
        test_int(0, count);

        e1.set<Position>({10, 20});
        test_int(1, count);
        test_assert(e_arg == e1);
        test_int(10, v.x);
        test_int(20, v.y);

        auto e2 = ecs.entity().set<Position>({30, 40});
        test_int(2, count);
        test_assert(e_arg == e2);
        test_int(30, v.x);
        test_int(40, v.y);
    }

    test_int(2, count);
}

void ComponentLifecycle_chained_hooks(void) {
    flecs::world ecs;

    int32_t add_count = 0;
    int32_t remove_count = 0;
    int32_t set_count = 0;

    ecs.component<Position>()
        .on_add([&](Position& p){
            add_count ++;
        })
        .on_set([&](Position& p){
            set_count ++;
        })
        .on_remove([&](Position& p){
            remove_count ++;
        });

    auto e = ecs.entity();
    test_int(0, add_count);
    test_int(0, set_count);
    test_int(0, remove_count);

    e.add<Position>();
    test_int(1, add_count);
    test_int(0, set_count);
    test_int(0, remove_count);

    e.set<Position>({10, 20});
    test_int(1, add_count);
    test_int(1, set_count);
    test_int(0, remove_count);

    e.remove<Position>();
    test_int(1, add_count);
    test_int(1, set_count);
    test_int(1, remove_count);
}

void ComponentLifecycle_ctor_w_2_worlds(void) {
    {
        flecs::world ecs;

        test_int(Pod::ctor_invoked, 0);

        ecs.entity().add<Pod>();
        test_int(Pod::ctor_invoked, 1);
    }

    Pod::ctor_invoked = 0;

    {
        flecs::world ecs;

        test_int(Pod::ctor_invoked, 0);

        ecs.entity().add<Pod>();
        test_int(Pod::ctor_invoked, 1);
    }
}

void ComponentLifecycle_ctor_w_2_worlds_explicit_registration(void) {
    {
        flecs::world ecs;

        ecs.component<Pod>();
        test_int(Pod::ctor_invoked, 0);

        ecs.entity().add<Pod>();
        test_int(Pod::ctor_invoked, 1);
    }

    Pod::ctor_invoked = 0;

    {
        flecs::world ecs;

        ecs.component<Pod>();
        test_int(Pod::ctor_invoked, 0);

        ecs.entity().add<Pod>();
        test_int(Pod::ctor_invoked, 1);
    }
}

struct DeferEmplaceTest {
    double x, y;

    DeferEmplaceTest(double x_, double y_) {
        x = x_;
        y = y_;
    }
};

void ComponentLifecycle_defer_emplace(void) {
    flecs::world ecs;

    flecs::entity e = ecs.entity();

    ecs.defer_begin();
    e.emplace<DeferEmplaceTest>(10.0, 20.0);
    test_assert(!e.has<DeferEmplaceTest>());
    ecs.defer_end();
    test_assert(e.has<DeferEmplaceTest>());

    const DeferEmplaceTest *p = e.try_get<DeferEmplaceTest>();
    test_assert(p != NULL);
    test_int(p->x, 10);
    test_int(p->y, 20);
}

void ComponentLifecycle_emplace_w_on_add(void) {
    flecs::world ecs;

    flecs::entity e1 = ecs.entity();

    int on_add = 0;
    ecs.component<Position>()
        .on_add([&](flecs::entity e, Position&) {
            on_add = true;
            test_assert(e == e1);
        });

    e1.emplace<Position>();
    test_int(on_add, 1);
}

void ComponentLifecycle_emplace_w_on_add_existing(void) {
    flecs::world ecs;

    flecs::entity e1 = ecs.entity().add<Velocity>();
    
    int on_add = 0;
    ecs.component<Position>()
        .on_add([&](flecs::entity e, Position&) {
            on_add = true;
            test_assert(e == e1);
        });

    e1.emplace<Position>();
    test_int(on_add, 1);
}

void ComponentLifecycle_set_pair_no_copy(void) {
    flecs::world ecs;

    struct Tag { };

    flecs::entity e = ecs.entity()
        .set<NoCopy, Tag>({ 10 });

    const NoCopy *ptr = e.try_get<NoCopy, Tag>();
    test_assert(ptr != NULL);
    test_int(ptr->x_, 10);
}

void ComponentLifecycle_set_pair_w_entity_no_copy(void) {
    flecs::world ecs;

    flecs::entity tag = ecs.entity();

    flecs::entity e = ecs.entity()
        .set<NoCopy>(tag, { 10 });

    const NoCopy *ptr = e.try_get<NoCopy>(tag);
    test_assert(ptr != NULL);
    test_int(ptr->x_, 10);
}

void ComponentLifecycle_set_pair_second_no_copy(void) {
    flecs::world ecs;

    flecs::entity tag = ecs.entity();

    flecs::entity e = ecs.entity()
        .set_second<NoCopy>(tag, { 10 });

    const NoCopy *ptr = e.try_get_second<NoCopy>(tag);
    test_assert(ptr != NULL);
    test_int(ptr->x_, 10);
}

void ComponentLifecycle_set_override_no_copy(void) {
    flecs::world ecs;

    flecs::entity e = ecs.entity()
        .set_auto_override<NoCopy>({ 10 });

    const NoCopy *ptr = e.try_get<NoCopy>();
    test_assert(ptr != NULL);
    test_int(ptr->x_, 10);

    test_assert(e.has(flecs::AUTO_OVERRIDE | ecs.id<NoCopy>()));
}

void ComponentLifecycle_set_override_pair_no_copy(void) {
    flecs::world ecs;

    flecs::entity e = ecs.entity()
        .set_auto_override<NoCopy, Tag>({ 10 });

    const NoCopy *ptr = e.try_get<NoCopy, Tag>();
    test_assert(ptr != NULL);
    test_int(ptr->x_, 10);

    test_assert(e.has(flecs::AUTO_OVERRIDE | ecs.pair<NoCopy, Tag>()));
}

void ComponentLifecycle_set_override_pair_w_entity_no_copy(void) {
    flecs::world ecs;

    flecs::entity tag = ecs.entity();

    flecs::entity e = ecs.entity()
        .set_auto_override<NoCopy>(tag, { 10 });

    const NoCopy *ptr = e.try_get<NoCopy>(tag);
    test_assert(ptr != NULL);
    test_int(ptr->x_, 10);

    test_assert(e.has(flecs::AUTO_OVERRIDE | ecs.pair<NoCopy>(tag)));
}

void ComponentLifecycle_dtor_after_defer_set(void) {
    {
        flecs::world ecs;

        auto e = ecs.entity();

        ecs.defer_begin();
        e.set<Pod>({10});
        test_assert(!e.has<Pod>());
        test_int(Pod::ctor_invoked, 2);
        test_int(Pod::dtor_invoked, 1);
        test_int(Pod::move_invoked, 1);
        test_int(Pod::move_ctor_invoked, 0);
        ecs.defer_end();

        test_assert(e.has<Pod>());
        test_int(Pod::ctor_invoked, 3);
        test_int(Pod::dtor_invoked, 2);
        test_int(Pod::move_invoked, 2);
        test_int(Pod::move_ctor_invoked, 0);

        const Pod *ptr = e.try_get<Pod>();
        test_assert(ptr != NULL);
        test_int(ptr->value, 10);

        test_int(Pod::ctor_invoked, 3);
        test_int(Pod::dtor_invoked, 2);
        test_int(Pod::move_invoked, 2);
        test_int(Pod::move_ctor_invoked, 0);
    }

    test_int(Pod::ctor_invoked, 3);
    test_int(Pod::dtor_invoked, 3);
    test_int(Pod::move_invoked, 2);
    test_int(Pod::move_ctor_invoked, 0);
}

void ComponentLifecycle_dtor_with_relation(void) {
    {
        flecs::world ecs;

        auto e = ecs.entity();
        auto e2 = ecs.entity().set<Pod>({5});

        e.set<Pod>({10}).add<Tag>(e2);

        test_int(Pod::ctor_invoked, 4);
        test_int(Pod::dtor_invoked, 3);
        test_int(Pod::move_invoked, 2);
        test_int(Pod::move_ctor_invoked, 1);

        const Pod *ptr = e.try_get<Pod>();
        test_assert(ptr != NULL);
        test_int(ptr->value, 10);

        test_int(Pod::ctor_invoked, 4);
        test_int(Pod::dtor_invoked, 3);
        test_int(Pod::move_invoked, 2);
        test_int(Pod::move_ctor_invoked, 1);
    }

    test_int(Pod::ctor_invoked, 4);
    test_int(Pod::dtor_invoked, 6);
    test_int(Pod::move_invoked, 3);
    test_int(Pod::move_ctor_invoked, 2);
}

void ComponentLifecycle_register_parent_after_child_w_hooks(void) {
    {
        flecs::world ecs;

        ecs.component<Pod::Child>();
        ecs.component<Pod>();

        ecs.entity().set<Pod>({});
    }

    test_int(Pod::ctor_invoked, 2);
    test_int(Pod::dtor_invoked, 2);
    test_int(Pod::move_invoked, 1);
    test_int(Pod::move_ctor_invoked, 0);
    test_int(Pod::copy_invoked, 0);
    test_int(Pod::copy_ctor_invoked, 0);
}

void ComponentLifecycle_register_parent_after_child_w_hooks_implicit(void) {
    {
        flecs::world ecs;

        ecs.entity().add<Pod::Child>().set<Pod>({});
    }

    test_int(Pod::ctor_invoked, 2);
    test_int(Pod::dtor_invoked, 2);
    test_int(Pod::move_invoked, 1);
    test_int(Pod::move_ctor_invoked, 0);
    test_int(Pod::copy_invoked, 0);
    test_int(Pod::copy_ctor_invoked, 0);
}

void ComponentLifecycle_dtor_relation_target(void) {
    {
        flecs::world ecs;

        auto e = ecs.entity();
        auto e2 = ecs.entity().emplace<CountNoDefaultCtor>(5).add<Tag>(e);
        ecs.entity().emplace<CountNoDefaultCtor>(5);

        test_int(CountNoDefaultCtor::ctor_invoked, 2);
        test_int(CountNoDefaultCtor::dtor_invoked, 1);
        test_int(CountNoDefaultCtor::move_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 1);

        const CountNoDefaultCtor *ptr = e2.try_get<CountNoDefaultCtor>();
        test_assert(ptr != NULL);
        test_int(ptr->value, 5);

        test_int(CountNoDefaultCtor::ctor_invoked, 2);
        test_int(CountNoDefaultCtor::dtor_invoked, 1);
        test_int(CountNoDefaultCtor::move_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 1);

        e.destruct();

        test_int(CountNoDefaultCtor::ctor_invoked, 2);
        test_int(CountNoDefaultCtor::dtor_invoked, 2);
        test_int(CountNoDefaultCtor::move_invoked, 0);
        test_int(CountNoDefaultCtor::move_ctor_invoked, 2);
    }

    test_int(CountNoDefaultCtor::ctor_invoked, 2);
    test_int(CountNoDefaultCtor::dtor_invoked, 4);
    test_int(CountNoDefaultCtor::move_invoked, 0);
    test_int(CountNoDefaultCtor::move_ctor_invoked, 2);
}

void ComponentLifecycle_sparse_component(void) {
    flecs::world world;

    world.component<Pod>().add(flecs::Sparse);

    auto e = world.entity().add<Pod>();
    test_assert(e.id() != 0);
    test_assert(e.has<Pod>());

    const Pod *pod = e.try_get<Pod>();
    test_assert(pod != NULL);
    test_int(Pod::ctor_invoked, 1);
    test_int(Pod::dtor_invoked, 0);
    test_int(Pod::copy_invoked, 0);
    test_int(Pod::move_invoked, 0);
    test_int(Pod::copy_ctor_invoked, 0);
    test_int(Pod::move_ctor_invoked, 0);

    e.add<Position>();

    test_int(Pod::ctor_invoked, 1);
    test_int(Pod::dtor_invoked, 0);
    test_int(Pod::copy_invoked, 0);
    test_int(Pod::move_invoked, 0);
    test_int(Pod::copy_ctor_invoked, 0);
    test_int(Pod::move_ctor_invoked, 0);

    test_int(pod->value, 10);

    e.remove<Pod>();

    test_int(Pod::ctor_invoked, 1);
    test_int(Pod::dtor_invoked, 1);
    test_int(Pod::copy_invoked, 0);
    test_int(Pod::move_invoked, 0);
    test_int(Pod::copy_ctor_invoked, 0);
    test_int(Pod::move_ctor_invoked, 0);
}

void ComponentLifecycle_count_in_add_hook(void) {
    flecs::world ecs;

    int count = 0;

    ecs.component<Position>().on_add([&](Position& p) {
        count = ecs.count<Position>();
    });

    ecs.entity().set<Position>({1, 2});
    test_int(count, 1);
    
    int matched = 0;
    ecs.query<Position>().each([&](Position& p) {
        matched ++;
    });

    test_int(matched, 1);
}

void ComponentLifecycle_count_in_remove_hook(void) {
    flecs::world ecs;

    int count = 0;

    ecs.component<Position>().on_remove([&](Position& p) {
        count = ecs.count<Position>();
    });

    auto ent = ecs.entity().set<Position>({1, 2});
    test_int(count, 0);

    ent.destruct();
    test_int(count, 1);
    
    int matched = 0;
    ecs.query<Position>().each([&](Position& p) {
        matched ++;
    });

    test_int(matched, 0);
}

/* This test checks that the hook configuration API (ecs_set_hooks_id) 
 * is invoked in a consistent manner */
void ComponentLifecycle_set_multiple_hooks(void) {
    flecs::world ecs;

    /* `Pod` type has various lifecycle hooks */
    auto pod = ecs.component<Pod>();
    
    /* It should be possible to configure other hooks afterwards: */
    int adds = 0;
    int sets = 0;
    int removes = 0;
    pod.on_add([&](Pod&) {
        adds++;
    });

    pod.on_set([&](Pod&) {
        sets++;
    });

    pod.on_remove([&](Pod&) {
        removes++;
    });

    /* Test hooks actually work: */

    ecs.entity().add<Pod>();
    test_int(adds, 1);

    ecs.entity().set<Pod>(Pod());
    test_int(adds, 2);
    test_int(sets, 1);

    ecs.release(); /* destroys world */
    test_int(removes, 2); /* two instances of `Pod` removed */
}

struct WithGreaterThan {
    int value;
    bool operator>(const WithGreaterThan &other) const {
        return value > other.value;
    }
};

struct WithLessThan {
    int value;
    bool operator<(const WithLessThan &other) const {
        return value < other.value;
    }
};

struct WithLessAndGreaterThan {
    int value;
    bool operator<(const WithLessAndGreaterThan &other) const {
        return value < other.value;
    }
    bool operator>(const WithLessAndGreaterThan &other) const {
        return value > other.value;
    }
};


struct WithEqualsAndGreaterThan {
    int value;
    bool operator==(const WithEqualsAndGreaterThan &other) const {
        return value == other.value;
    }
    bool operator>(const WithEqualsAndGreaterThan &other) const {
        return value > other.value;
    }
};

struct WithEqualsAndLessThan {
    int value;
    bool operator==(const WithEqualsAndLessThan &other) const {
        return value == other.value;
    }
    bool operator<(const WithEqualsAndLessThan &other) const {
        return value < other.value;
    }
};

struct WithEqualsOnly {
    int value;
    bool operator==(const WithEqualsOnly &other) const {
        return value == other.value;
    }
};

struct WithoutOperators {
    int16_t value;
};

static
int compare(flecs::world& ecs, flecs::entity_t id, const void *a, const void *b) {
    const ecs_type_info_t* ti = ecs_get_type_info(ecs, id);
    return ti->hooks.cmp(a, b, ti);
}

static
bool equals(flecs::world& ecs, flecs::entity_t id, const void *a, const void *b) {
    const ecs_type_info_t* ti = ecs_get_type_info(ecs, id);
    return ti->hooks.equals(a, b, ti);
}

void ComponentLifecycle_compare_WithGreaterThan(void) {
    flecs::world ecs;

    auto component = ecs.component<WithGreaterThan>();

    const ecs_type_hooks_t* hooks = ecs_get_hooks_id(ecs, component);
    test_assert(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL);
    test_assert(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL);

    /* generate cmp operator from C++ operator> */
    component.on_compare(); 

    hooks = ecs_get_hooks_id(ecs, component);
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL));
    /* `equals` is automatically generated: */
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL));

    WithGreaterThan a = {1};
    WithGreaterThan b = {2};
    WithGreaterThan c = {1};

    test_assert(compare(ecs, component, &a, &b) < 0);
    test_assert(compare(ecs, component, &b, &a) > 0);
    test_assert(compare(ecs, component, &a, &c) == 0);
    test_assert(compare(ecs, component, &b, &c) > 0);
    test_assert(compare(ecs, component, &c, &b) < 0);
    test_assert(compare(ecs, component, &b, &b) == 0);

    /* test using autogenerated equals operator: */
    test_assert(equals(ecs, component, &a, &b) == false);
    test_assert(equals(ecs, component, &a, &c) == true);
    test_assert(equals(ecs, component, &a, &a) == true);     
}

void ComponentLifecycle_compare_WithLessThan(void) {
    flecs::world ecs;

    auto component = ecs.component<WithLessThan>();

    const ecs_type_hooks_t* hooks = ecs_get_hooks_id(ecs, component);

    test_assert(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL);
    test_assert(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL);

    /* generate cmp operator from C++ operator< */
    component.on_compare(); 

    hooks = ecs_get_hooks_id(ecs, component);
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL));
    /* `equals` is automatically generated: */
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL));


    WithLessThan a = {1};
    WithLessThan b = {2};
    WithLessThan c = {1};

    test_assert(compare(ecs, component, &a, &b) < 0);
    test_assert(compare(ecs, component, &b, &a) > 0);
    test_assert(compare(ecs, component, &a, &c) == 0);
    test_assert(compare(ecs, component, &b, &c) > 0);
    test_assert(compare(ecs, component, &c, &b) < 0);
    test_assert(compare(ecs, component, &b, &b) == 0);

    /* test using autogenerated equals operator: */
    test_assert(equals(ecs, component, &a, &b) == false);
    test_assert(equals(ecs, component, &a, &c) == true);
    test_assert(equals(ecs, component, &a, &a) == true);         
}

void ComponentLifecycle_compare_WithLessAndGreaterThan(void) {
    flecs::world ecs;

    auto component = ecs.component<WithLessAndGreaterThan>();

    const ecs_type_hooks_t* hooks = ecs_get_hooks_id(ecs, component);
    test_assert(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL);
    test_assert(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL);

    /* generate cmp operator from C++ operator> and operator< */
    component.on_compare(); 

    hooks = ecs_get_hooks_id(ecs, component);
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL));
    /* `equals` is automatically generated: */
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL));

    WithLessAndGreaterThan a = {1};
    WithLessAndGreaterThan b = {2};
    WithLessAndGreaterThan c = {1};

    test_assert(compare(ecs, component, &a, &b) < 0);
    test_assert(compare(ecs, component, &b, &a) > 0);
    test_assert(compare(ecs, component, &a, &c) == 0);
    test_assert(compare(ecs, component, &b, &c) > 0);
    test_assert(compare(ecs, component, &c, &b) < 0);
    test_assert(compare(ecs, component, &b, &b) == 0);

    /* test using autogenerated equals operator: */
    test_assert(equals(ecs, component, &a, &b) == false);
    test_assert(equals(ecs, component, &a, &c) == true);
    test_assert(equals(ecs, component, &a, &a) == true);     
}

void ComponentLifecycle_compare_WithEqualsAndGreaterThan(void) {
    flecs::world ecs;

    auto component = ecs.component<WithEqualsAndGreaterThan>();

    const ecs_type_hooks_t* hooks = ecs_get_hooks_id(ecs, component);
    test_assert(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL);
    test_assert(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL);

    /* generate cmp operator from C++ operator> and operator== */
    component.on_compare(); 

    hooks = ecs_get_hooks_id(ecs, component);
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL));
    /* `equals` is automatically generated: */
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL));

    WithEqualsAndGreaterThan a = {1};
    WithEqualsAndGreaterThan b = {2};
    WithEqualsAndGreaterThan c = {1};

    test_assert(compare(ecs, component, &a, &b) < 0);
    test_assert(compare(ecs, component, &b, &a) > 0);
    test_assert(compare(ecs, component, &a, &c) == 0);
    test_assert(compare(ecs, component, &b, &c) > 0);
    test_assert(compare(ecs, component, &c, &b) < 0);
    test_assert(compare(ecs, component, &b, &b) == 0);

    /* test using equals operator: */
    test_assert(equals(ecs, component, &a, &b) == false);
    test_assert(equals(ecs, component, &a, &c) == true);
    test_assert(equals(ecs, component, &a, &a) == true);
}

void ComponentLifecycle_compare_WithEqualsAndLessThan(void) {
    flecs::world ecs;

    auto component = ecs.component<WithEqualsAndLessThan>();

    const ecs_type_hooks_t* hooks = ecs_get_hooks_id(ecs, component);
    test_assert(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL);
    test_assert(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL);

    /* generate cmp operator from C++ operator< and operator== */
    component.on_compare(); 

    hooks = ecs_get_hooks_id(ecs, component);
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL));
    /* `equals` is automatically generated: */
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL));


    WithEqualsAndLessThan a = {1};
    WithEqualsAndLessThan b = {2};
    WithEqualsAndLessThan c = {1};

    test_assert(compare(ecs, component, &a, &b) < 0);
    test_assert(compare(ecs, component, &b, &a) > 0);
    test_assert(compare(ecs, component, &a, &c) == 0);
    test_assert(compare(ecs, component, &b, &c) > 0);
    test_assert(compare(ecs, component, &c, &b) < 0);
    test_assert(compare(ecs, component, &b, &b) == 0);

    /* test using equals operator: */
    test_assert(equals(ecs, component, &a, &b) == false);
    test_assert(equals(ecs, component, &a, &c) == true);
    test_assert(equals(ecs, component, &a, &a) == true);


}

void ComponentLifecycle_compare_WithEqualsOnly(void) {
    flecs::world ecs;

    auto component = ecs.component<WithEqualsOnly>();

    const ecs_type_hooks_t* hooks = ecs_get_hooks_id(ecs, component);
    test_assert(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL);
    test_assert(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL);

    /* generate equals operator from C++ operator== */
    component.on_equals(); 

    hooks = ecs_get_hooks_id(ecs, component);

    /* can't compare if no < or > operators are defined */
    test_assert(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL);

    /* operator equals is defined: */
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL));

    WithEqualsOnly a = {1};
    WithEqualsOnly b = {2};
    WithEqualsOnly c = {1};
    
    test_assert(equals(ecs, component, &a, &b) == false);
    test_assert(equals(ecs, component, &a, &c) == true);
    test_assert(equals(ecs, component, &a, &a) == true);

}

void ComponentLifecycle_compare_WithoutOperators(void) {
    flecs::world ecs;

    auto component = ecs.component<WithoutOperators>();

    const ecs_type_hooks_t* hooks = ecs_get_hooks_id(ecs, component);

    /* can't compare if no operators are defined at all */
    test_assert(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL);
    test_assert(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL);

    /* define operator equals via a explicit callback: */
    component.on_equals([](
        const WithoutOperators *a, 
        const WithoutOperators *b, 
        const ecs_type_info_t* ti) -> bool {
            return a->value == b->value;
        });

    hooks = ecs_get_hooks_id(ecs, component);

    test_assert(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL);
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL));

    WithoutOperators a = {1};
    WithoutOperators b = {2};
    WithoutOperators c = {1};
    
    test_assert(equals(ecs, component, &a, &b) == false);
    test_assert(equals(ecs, component, &a, &c) == true);
    test_assert(equals(ecs, component, &a, &a) == true);    

    component.on_compare([](
        const WithoutOperators *a, 
        const WithoutOperators *b, 
        const ecs_type_info_t *ti) -> int {
            return a->value - b->value;
        });

    hooks = ecs_get_hooks_id(ecs, component);

    test_assert(!(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL));
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL));

    test_assert(compare(ecs, component, &a, &b) < 0);
    test_assert(compare(ecs, component, &b, &a) > 0);
    test_assert(compare(ecs, component, &a, &c) == 0);
    test_assert(compare(ecs, component, &b, &c) > 0);
    test_assert(compare(ecs, component, &c, &b) < 0);
    test_assert(compare(ecs, component, &b, &b) == 0);    
}


template <typename T>
struct TestUnsignedEnum {
    enum Type : T {
        Red = 1,
        Yellow = 2,
        Blue = 3
    };
};

template <typename T>
struct TestSignedEnum {
    enum Type : T {
        Red = -1,
        Yellow = 0,
        Blue = 1
    };
};

void ComponentLifecycle_compare_uint8_Enum(void) {

    flecs::world ecs;

    using Enum8 = TestUnsignedEnum<uint8_t>::Type;

    auto component = ecs.component<Enum8>();

    const ecs_type_hooks_t* hooks = ecs_get_hooks_id(ecs, component);

    test_assert(!(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL));
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL));

    Enum8 a = Enum8::Red;
    Enum8 b = Enum8::Yellow;
    Enum8 c = Enum8::Red;

    test_assert(compare(ecs, component, &a, &b) < 0);
    test_assert(compare(ecs, component, &b, &a) > 0);
    test_assert(compare(ecs, component, &a, &c) == 0);
    test_assert(compare(ecs, component, &b, &c) > 0);
    test_assert(compare(ecs, component, &c, &b) < 0);
    test_assert(compare(ecs, component, &b, &b) == 0);

    test_assert(equals(ecs, component, &a, &c));
    test_assert(equals(ecs, component, &b, &b));
}


void ComponentLifecycle_compare_uint16_Enum(void) {

    flecs::world ecs;

    using Enum16 = TestUnsignedEnum<uint16_t>::Type;

    auto component = ecs.component<Enum16>();

    const ecs_type_hooks_t* hooks = ecs_get_hooks_id(ecs, component);

    test_assert(!(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL));
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL));

    Enum16 a = Enum16::Red;
    Enum16 b = Enum16::Yellow;
    Enum16 c = Enum16::Red;

    test_assert(compare(ecs, component, &a, &b) < 0);
    test_assert(compare(ecs, component, &b, &a) > 0);
    test_assert(compare(ecs, component, &a, &c) == 0);
    test_assert(compare(ecs, component, &b, &c) > 0);
    test_assert(compare(ecs, component, &c, &b) < 0);
    test_assert(compare(ecs, component, &b, &b) == 0);    

    test_assert(equals(ecs, component, &a, &c));
    test_assert(equals(ecs, component, &b, &b));
}

void ComponentLifecycle_compare_uint32_Enum(void) {

    flecs::world ecs;

    using Enum32 = TestUnsignedEnum<uint32_t>::Type;

    auto component = ecs.component<Enum32>();

    const ecs_type_hooks_t* hooks = ecs_get_hooks_id(ecs, component);

    test_assert(!(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL));
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL));

    Enum32 a = Enum32::Red;
    Enum32 b = Enum32::Yellow;
    Enum32 c = Enum32::Red;

    test_assert(compare(ecs, component, &a, &b) < 0);
    test_assert(compare(ecs, component, &b, &a) > 0);
    test_assert(compare(ecs, component, &a, &c) == 0);
    test_assert(compare(ecs, component, &b, &c) > 0);
    test_assert(compare(ecs, component, &c, &b) < 0);
    test_assert(compare(ecs, component, &b, &b) == 0);    

    test_assert(equals(ecs, component, &a, &c));
    test_assert(equals(ecs, component, &b, &b));
}


void ComponentLifecycle_compare_uint64_Enum(void) {

    flecs::world ecs;

    using Enum64 = TestUnsignedEnum<uint64_t>::Type;

    auto component = ecs.component<Enum64>();

    const ecs_type_hooks_t* hooks = ecs_get_hooks_id(ecs, component);

    test_assert(!(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL));
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL));

    Enum64 a = Enum64::Red;
    Enum64 b = Enum64::Yellow;
    Enum64 c = Enum64::Red;

    test_assert(compare(ecs, component, &a, &b) < 0);
    test_assert(compare(ecs, component, &b, &a) > 0);
    test_assert(compare(ecs, component, &a, &c) == 0);
    test_assert(compare(ecs, component, &b, &c) > 0);
    test_assert(compare(ecs, component, &c, &b) < 0);
    test_assert(compare(ecs, component, &b, &b) == 0);    

    test_assert(equals(ecs, component, &a, &c));
    test_assert(equals(ecs, component, &b, &b));
}

void ComponentLifecycle_compare_int8_Enum(void) {

    flecs::world ecs;

    using Enum8 = TestSignedEnum<int8_t>::Type;

    auto component = ecs.component<Enum8>();

    const ecs_type_hooks_t* hooks = ecs_get_hooks_id(ecs, component);

    test_assert(!(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL));
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL));

    Enum8 a = Enum8::Red;
    Enum8 b = Enum8::Yellow;
    Enum8 c = Enum8::Red;

    test_assert(compare(ecs, component, &a, &b) < 0);
    test_assert(compare(ecs, component, &b, &a) > 0);
    test_assert(compare(ecs, component, &a, &c) == 0);
    test_assert(compare(ecs, component, &b, &c) > 0);
    test_assert(compare(ecs, component, &c, &b) < 0);
    test_assert(compare(ecs, component, &b, &b) == 0);

    test_assert(equals(ecs, component, &a, &c));
    test_assert(equals(ecs, component, &b, &b));
}

void ComponentLifecycle_compare_int16_Enum(void) {

    flecs::world ecs;

    using Enum16 = TestSignedEnum<int16_t>::Type;

    auto component = ecs.component<Enum16>();

    const ecs_type_hooks_t* hooks = ecs_get_hooks_id(ecs, component);

    test_assert(!(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL));
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL));

    Enum16 a = Enum16::Red;
    Enum16 b = Enum16::Yellow;
    Enum16 c = Enum16::Red;

    test_assert(compare(ecs, component, &a, &b) < 0);
    test_assert(compare(ecs, component, &b, &a) > 0);
    test_assert(compare(ecs, component, &a, &c) == 0);
    test_assert(compare(ecs, component, &b, &c) > 0);
    test_assert(compare(ecs, component, &c, &b) < 0);
    test_assert(compare(ecs, component, &b, &b) == 0);

    test_assert(equals(ecs, component, &a, &c));
    test_assert(equals(ecs, component, &b, &b));
}

void ComponentLifecycle_compare_int32_Enum(void) {

    flecs::world ecs;

    using Enum32 = TestSignedEnum<int32_t>::Type;

    auto component = ecs.component<Enum32>();

    const ecs_type_hooks_t* hooks = ecs_get_hooks_id(ecs, component);

    test_assert(!(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL));
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL));

    Enum32 a = Enum32::Red;
    Enum32 b = Enum32::Yellow;
    Enum32 c = Enum32::Red;

    test_assert(compare(ecs, component, &a, &b) < 0);
    test_assert(compare(ecs, component, &b, &a) > 0);
    test_assert(compare(ecs, component, &a, &c) == 0);
    test_assert(compare(ecs, component, &b, &c) > 0);
    test_assert(compare(ecs, component, &c, &b) < 0);
    test_assert(compare(ecs, component, &b, &b) == 0);

    test_assert(equals(ecs, component, &a, &c));
    test_assert(equals(ecs, component, &b, &b));
}

void ComponentLifecycle_compare_int64_Enum(void) {

    flecs::world ecs;

    using Enum64 = TestSignedEnum<int64_t>::Type;

    auto component = ecs.component<Enum64>();

    const ecs_type_hooks_t* hooks = ecs_get_hooks_id(ecs, component);

    test_assert(!(hooks->flags & ECS_TYPE_HOOK_CMP_ILLEGAL));
    test_assert(!(hooks->flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL));

    Enum64 a = Enum64::Red;
    Enum64 b = Enum64::Yellow;
    Enum64 c = Enum64::Red;

    test_assert(compare(ecs, component, &a, &b) < 0);
    test_assert(compare(ecs, component, &b, &a) > 0);
    test_assert(compare(ecs, component, &a, &c) == 0);
    test_assert(compare(ecs, component, &b, &c) > 0);
    test_assert(compare(ecs, component, &c, &b) < 0);
    test_assert(compare(ecs, component, &b, &b) == 0);

    test_assert(equals(ecs, component, &a, &c));
    test_assert(equals(ecs, component, &b, &b));
}

struct NonDefaultConstructible {
    NonDefaultConstructible(int x) : x{x} {}
    int x;
};

void ComponentLifecycle_move_ctor_no_default_ctor(void) {
    flecs::world world;
    flecs::entity e1 = world.entity().emplace<NonDefaultConstructible>(1);
    flecs::entity e2 = world.entity().emplace<NonDefaultConstructible>(2);
    e1.add<Tag>();
    test_assert(e1.has<Tag>());

    {
        const NonDefaultConstructible *ptr = e1.try_get<NonDefaultConstructible>();
        test_assert(ptr != nullptr);
        test_int(ptr->x, 1);
    }
    {
        const NonDefaultConstructible *ptr = e2.try_get<NonDefaultConstructible>();
        test_assert(ptr != nullptr);
        test_int(ptr->x, 2);
    }
}
