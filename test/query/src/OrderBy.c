#include <query.h>
#include <stdlib.h>

static
int compare_position(
    ecs_entity_t e1,
    const void *ptr1,
    ecs_entity_t e2,
    const void *ptr2)
{
    const Position *p1 = ptr1;
    const Position *p2 = ptr2;
    return (p1->x > p2->x) - (p1->x < p2->x);
}

static
int compare_entity(
    ecs_entity_t e1,
    const void *ptr1,
    ecs_entity_t e2,
    const void *ptr2)
{
    return (e1 > e2) - (e1 < e2);
}

void OrderBy_sort_by_component(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);

    ecs_entity_t e1 = ecs_insert(world, ecs_value(Position, {3, 0}));
    ecs_entity_t e2 = ecs_insert(world, ecs_value(Position, {1, 0}));
    ecs_entity_t e3 = ecs_insert(world, ecs_value(Position, {5, 0}));
    ecs_entity_t e4 = ecs_insert(world, ecs_value(Position, {2, 0}));
    ecs_entity_t e5 = ecs_insert(world, ecs_value(Position, {4, 0}));

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    ecs_iter_t it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 5);

    test_assert(it.entities[0] == e2);
    test_assert(it.entities[1] == e4);
    test_assert(it.entities[2] == e1);
    test_assert(it.entities[3] == e5);
    test_assert(it.entities[4] == e3);
    test_assert(!ecs_query_next(&it));

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_by_component_same_value_1(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);

    ecs_entity_t e1 = ecs_insert(world, ecs_value(Position, {3, 0}));
    ecs_entity_t e2 = ecs_insert(world, ecs_value(Position, {3, 0}));
    ecs_entity_t e3 = ecs_insert(world, ecs_value(Position, {2, 0}));
    ecs_entity_t e4 = ecs_insert(world, ecs_value(Position, {2, 0}));
    ecs_entity_t e5 = ecs_insert(world, ecs_value(Position, {1, 0}));

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    ecs_iter_t it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 5);
    

    test_assert(it.entities[0] == e5);
    test_assert(it.entities[1] == e4);
    test_assert(it.entities[2] == e3);
    test_assert(it.entities[3] == e2);
    test_assert(it.entities[4] == e1);

    test_assert(!ecs_query_next(&it));

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_by_component_same_value_2(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);

    ecs_entity_t e1 = ecs_insert(world, ecs_value(Position, {2, 0}));
    ecs_entity_t e2 = ecs_insert(world, ecs_value(Position, {1, 0}));
    ecs_entity_t e3 = ecs_insert(world, ecs_value(Position, {2, 0}));
    ecs_entity_t e4 = ecs_insert(world, ecs_value(Position, {1, 0}));
    ecs_entity_t e5 = ecs_insert(world, ecs_value(Position, {2, 0}));
    ecs_entity_t e6 = ecs_insert(world, ecs_value(Position, {1, 0}));

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    ecs_iter_t it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));

    test_int(it.count, 6);
    test_assert(it.entities[0] == e4);
    test_assert(it.entities[1] == e6);
    test_assert(it.entities[2] == e2);
    test_assert(it.entities[3] == e1);
    test_assert(it.entities[4] == e3);
    test_assert(it.entities[5] == e5);

    test_assert(!ecs_query_next(&it));

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_by_component_2_tables(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_entity_t e1 = ecs_insert(world, ecs_value(Position, {3, 0}));
    ecs_entity_t e2 = ecs_insert(world, ecs_value(Position, {1, 0}));
    ecs_entity_t e3 = ecs_insert(world, ecs_value(Position, {5, 0}));
    ecs_entity_t e4 = ecs_insert(world, ecs_value(Position, {2, 0}));
    ecs_entity_t e5 = ecs_insert(world, ecs_value(Position, {4, 0}));

    ecs_add(world, e3, Velocity);
    ecs_add(world, e4, Velocity);
    ecs_add(world, e5, Velocity);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    ecs_iter_t it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e2);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e4);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e1);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 2);
    test_assert(it.entities[0] == e5);
    test_assert(it.entities[1] == e3);

    test_assert(!ecs_query_next(&it));

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_by_component_3_tables(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);

    ecs_entity_t e1 = ecs_insert(world, ecs_value(Position, {3, 0}));
    ecs_entity_t e2 = ecs_insert(world, ecs_value(Position, {1, 0}));
    ecs_entity_t e3 = ecs_insert(world, ecs_value(Position, {6, 0}));
    ecs_entity_t e4 = ecs_insert(world, ecs_value(Position, {2, 0}));
    ecs_entity_t e5 = ecs_insert(world, ecs_value(Position, {4, 0}));
    ecs_entity_t e6 = ecs_insert(world, ecs_value(Position, {5, 0}));
    ecs_entity_t e7 = ecs_insert(world, ecs_value(Position, {7, 0}));

    ecs_add(world, e5, Velocity);
    ecs_add(world, e6, Mass);
    ecs_add(world, e7, Mass);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    ecs_iter_t it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 3);
    test_assert(it.entities[0] == e2);
    test_assert(it.entities[1] == e4);
    test_assert(it.entities[2] == e1);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e5);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e6);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e3);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e7);

    test_assert(!ecs_query_next(&it));

    ecs_query_fini(q);

    ecs_fini(world);
}


void OrderBy_sort_by_entity(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_entity_t e1 = ecs_insert(world, ecs_value(Position, {3, 0}));
    ecs_entity_t e2 = ecs_insert(world, ecs_value(Position, {1, 0}));
    ecs_entity_t e3 = ecs_insert(world, ecs_value(Position, {5, 0}));
    ecs_entity_t e4 = ecs_insert(world, ecs_value(Position, {2, 0}));
    ecs_entity_t e5 = ecs_insert(world, ecs_value(Position, {4, 0}));

    ecs_add(world, e2, Velocity);
    ecs_add(world, e4, Velocity);
    ecs_add(world, e5, Velocity);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by_callback = compare_entity
    });

    ecs_iter_t it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e1);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e2);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e3);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 2);
    test_assert(it.entities[0] == e4);
    test_assert(it.entities[1] == e5);

    test_assert(!ecs_query_next(&it));

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_after_add(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);

    ecs_entity_t e1 = ecs_insert(world, ecs_value(Position, {3, 0}));
    ecs_entity_t e2 = ecs_insert(world, ecs_value(Position, {1, 0}));
    ecs_entity_t e3 = ecs_insert(world, ecs_value(Position, {6, 0}));
    ecs_entity_t e4 = ecs_insert(world, ecs_value(Position, {2, 0}));
    ecs_entity_t e5 = ecs_insert(world, ecs_value(Position, {5, 0}));

    ecs_add(world, e3, Velocity);
    ecs_add(world, e4, Velocity);
    ecs_add(world, e5, Velocity);

    ecs_add(world, e3, Mass);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position, Velocity",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    ecs_iter_t it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 2);
    test_assert(it.entities[0] == e4);
    test_assert(it.entities[1] == e5);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e3);

    test_assert(!ecs_query_next(&it));

    ecs_add(world, e1, Velocity);

    it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 3);
    test_assert(it.entities[0] == e4);
    test_assert(it.entities[1] == e1);
    test_assert(it.entities[2] == e5);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e3);

    test_assert(!ecs_query_next(&it)); 

    ecs_add(world, e2, Velocity);   
    ecs_add(world, e2, Mass);

    it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e2);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 3);
    test_assert(it.entities[0] == e4);
    test_assert(it.entities[1] == e1);
    test_assert(it.entities[2] == e5);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e3);

    test_assert(!ecs_query_next(&it));

    ecs_query_fini(q);  

    ecs_fini(world);
}

void OrderBy_sort_after_remove(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);

    ecs_entity_t e1 = ecs_insert(world, ecs_value(Position, {3, 0}));
    ecs_entity_t e2 = ecs_insert(world, ecs_value(Position, {1, 0}));
    ecs_entity_t e3 = ecs_insert(world, ecs_value(Position, {6, 0}));
    ecs_entity_t e4 = ecs_insert(world, ecs_value(Position, {2, 0}));
    ecs_entity_t e5 = ecs_insert(world, ecs_value(Position, {5, 0}));

    ecs_add(world, e1, Velocity);
    ecs_add(world, e2, Velocity);   

    ecs_add(world, e3, Velocity);
    ecs_add(world, e4, Velocity);
    ecs_add(world, e5, Velocity);

    ecs_add(world, e2, Mass);
    ecs_add(world, e3, Mass);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position, Velocity",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    ecs_iter_t it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e2);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 3);
    test_assert(it.entities[0] == e4);
    test_assert(it.entities[1] == e1);
    test_assert(it.entities[2] == e5);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e3);

    test_assert(!ecs_query_next(&it));  

    ecs_remove(world, e1, Velocity);

    it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e2);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 2);
    test_assert(it.entities[0] == e4);
    test_assert(it.entities[1] == e5);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e3);

    test_assert(!ecs_query_next(&it)); 

    ecs_remove(world, e2, Mass); 

    it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 3);
    test_assert(it.entities[0] == e2);
    test_assert(it.entities[1] == e4);
    test_assert(it.entities[2] == e5);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e3);

    test_assert(!ecs_query_next(&it));

    ecs_remove(world, e2, Velocity); 

    it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 2);
    test_assert(it.entities[0] == e4);
    test_assert(it.entities[1] == e5);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e3);

    test_assert(!ecs_query_next(&it));

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_after_delete(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);

    ecs_entity_t e1 = ecs_insert(world, ecs_value(Position, {3, 0}));
    ecs_entity_t e2 = ecs_insert(world, ecs_value(Position, {1, 0}));
    ecs_entity_t e3 = ecs_insert(world, ecs_value(Position, {6, 0}));
    ecs_entity_t e4 = ecs_insert(world, ecs_value(Position, {2, 0}));
    ecs_entity_t e5 = ecs_insert(world, ecs_value(Position, {5, 0}));

    ecs_add(world, e1, Velocity);
    ecs_add(world, e2, Velocity);   

    ecs_add(world, e3, Velocity);
    ecs_add(world, e4, Velocity);
    ecs_add(world, e5, Velocity);

    ecs_add(world, e2, Mass);
    ecs_add(world, e3, Mass);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position, Velocity",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    ecs_iter_t it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e2);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 3);
    test_assert(it.entities[0] == e4);
    test_assert(it.entities[1] == e1);
    test_assert(it.entities[2] == e5);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e3);

    test_assert(!ecs_query_next(&it));  

    ecs_delete(world, e1);

    it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e2);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 2);
    test_assert(it.entities[0] == e4);
    test_assert(it.entities[1] == e5);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e3);

    test_assert(!ecs_query_next(&it)); 

    ecs_delete(world, e2); 

    it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 2);
    test_assert(it.entities[0] == e4);
    test_assert(it.entities[1] == e5);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e3);

    test_assert(!ecs_query_next(&it));

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_after_set(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);

    ecs_entity_t e1 = ecs_insert(world, ecs_value(Position, {3, 0}));
    ecs_entity_t e2 = ecs_insert(world, ecs_value(Position, {1, 0}));
    ecs_entity_t e3 = ecs_insert(world, ecs_value(Position, {6, 0}));
    ecs_entity_t e4 = ecs_insert(world, ecs_value(Position, {2, 0}));
    ecs_entity_t e5 = ecs_insert(world, ecs_value(Position, {5, 0}));

    ecs_add(world, e1, Velocity);
    ecs_add(world, e2, Velocity);   

    ecs_add(world, e3, Velocity);
    ecs_add(world, e4, Velocity);
    ecs_add(world, e5, Velocity);

    ecs_add(world, e2, Mass);
    ecs_add(world, e3, Mass);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position, Velocity",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    ecs_iter_t it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e2);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 3);
    test_assert(it.entities[0] == e4);
    test_assert(it.entities[1] == e1);
    test_assert(it.entities[2] == e5);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e3);

    test_assert(!ecs_query_next(&it)); 

    ecs_set(world, e1, Position, {7, 0});

    it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e2);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 2);
    test_assert(it.entities[0] == e4);
    test_assert(it.entities[1] == e5);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e3);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e1);

    test_assert(!ecs_query_next(&it));

    ecs_query_fini(q);

    ecs_fini(world);
}

static
void FlipP(ecs_iter_t *it) {
    Position *p = ecs_field(it, Position, 0);

    int32_t i;
    for (i = 0; i < it->count; i ++) {
        float x = p[i].x;
        p[i].x = p[i].y;
        p[i].y = x;
    }
}

void OrderBy_sort_after_system(void) {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Mass);

    ECS_SYSTEM(world, FlipP, EcsOnUpdate, Position);

    ecs_entity_t e1 = ecs_insert(world, ecs_value(Position, {3, 5}));
    ecs_entity_t e2 = ecs_insert(world, ecs_value(Position, {1, 2}));
    ecs_entity_t e3 = ecs_insert(world, ecs_value(Position, {6, 6}));
    ecs_entity_t e4 = ecs_insert(world, ecs_value(Position, {2, 1}));
    ecs_entity_t e5 = ecs_insert(world, ecs_value(Position, {5, 3}));

    ecs_add(world, e1, Velocity);
    ecs_add(world, e2, Velocity);   

    ecs_add(world, e3, Velocity);
    ecs_add(world, e4, Velocity);
    ecs_add(world, e5, Velocity);

    ecs_add(world, e2, Mass);
    ecs_add(world, e3, Mass);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position, [in] Velocity",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    ecs_iter_t it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e2);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 3);
    test_assert(it.entities[0] == e4);
    test_assert(it.entities[1] == e1);
    test_assert(it.entities[2] == e5);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e3);

    test_assert(!ecs_query_next(&it));

    ecs_progress(world, 0);

    it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e4);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e2);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 2);    
    test_assert(it.entities[0] == e5);
    test_assert(it.entities[1] == e1);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e3);

    test_assert(!ecs_query_next(&it));

    ecs_progress(world, 0);

    /* Second iteration, query now needs to check dirty admin to see if system
     * updated component */
    it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e2);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 3);
    test_assert(it.entities[0] == e4);
    test_assert(it.entities[1] == e1);
    test_assert(it.entities[2] == e5);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e3);

    test_assert(!ecs_query_next(&it));

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_after_query(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Mass);

    ecs_entity_t e1 = ecs_insert(world, ecs_value(Position, {3, 5}));
    ecs_entity_t e2 = ecs_insert(world, ecs_value(Position, {1, 2}));
    ecs_entity_t e3 = ecs_insert(world, ecs_value(Position, {6, 6}));
    ecs_entity_t e4 = ecs_insert(world, ecs_value(Position, {2, 1}));
    ecs_entity_t e5 = ecs_insert(world, ecs_value(Position, {5, 3}));

    ecs_add(world, e1, Velocity);
    ecs_add(world, e2, Velocity);   

    ecs_add(world, e3, Velocity);
    ecs_add(world, e4, Velocity);
    ecs_add(world, e5, Velocity);

    ecs_add(world, e2, Mass);
    ecs_add(world, e3, Mass);

    ecs_query_t *flip_q = ecs_query(world, { .expr = "Position" });

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position, [in] Velocity",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    ecs_iter_t it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e2);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 3);
    test_assert(it.entities[0] == e4);
    test_assert(it.entities[1] == e1);
    test_assert(it.entities[2] == e5);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e3);

    test_assert(!ecs_query_next(&it));

    ecs_iter_t qit = ecs_query_iter(world, flip_q);
    while (ecs_query_next(&qit)) {
        FlipP(&qit);
    }

    /* First iteration, query will register monitor with table, so table is
     * always marked dirty */
    it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e4);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e2);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 2);    
    test_assert(it.entities[0] == e5);
    test_assert(it.entities[1] == e1);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e3);

    test_assert(!ecs_query_next(&it));

    qit = ecs_query_iter(world, flip_q);
    while (ecs_query_next(&qit)) {
        FlipP(&qit);
    }

    /* Second iteration, query now needs to check dirty admin to see if system
     * updated component */
    it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e2);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 3);
    test_assert(it.entities[0] == e4);
    test_assert(it.entities[1] == e1);
    test_assert(it.entities[2] == e5);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);    
    test_assert(it.entities[0] == e3);

    test_assert(!ecs_query_next(&it));

    ecs_query_fini(q);
    ecs_query_fini(flip_q);

    ecs_fini(world);
}

void OrderBy_sort_by_component_move_pivot(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);

    ecs_entity_t e1 = ecs_insert(world, ecs_value(Position, {5, 0}));
    ecs_entity_t e2 = ecs_insert(world, ecs_value(Position, {10, 0}));
    ecs_entity_t e3 = ecs_insert(world, ecs_value(Position, {2, 0}));
    ecs_entity_t e4 = ecs_insert(world, ecs_value(Position, {1, 0}));

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    ecs_iter_t it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 4);

    test_assert(it.entities[0] == e4);
    test_assert(it.entities[1] == e3);
    test_assert(it.entities[2] == e1);
    test_assert(it.entities[3] == e2);

    test_assert(!ecs_query_next(&it));

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_1000_entities(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    for (int i = 0; i < 1000; i ++) {
        int32_t v = rand();
        ecs_insert(world, ecs_value(Position, {v}));

        int32_t x = 0;

        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            Position *p = ecs_field(&it, Position, 0);

            test_assert(it.count == (i + 1));

            int32_t j;
            for (j = 0; j < it.count; j ++) {  
                test_assert(x <= p[j].x);
                x = p[j].x;
            }
        }
    }

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_1000_entities_w_duplicates(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    for (int i = 0; i < 500; i ++) {
        int32_t v = rand();
        ecs_insert(world, ecs_value(Position, {v}));
        ecs_insert(world, ecs_value(Position, {v}));

        int32_t x = 0;

        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            Position *p = ecs_field(&it, Position, 0);

            test_assert(it.count == (i + 1) * 2);

            int32_t j;
            for (j = 0; j < it.count; j ++) {  
                test_assert(x <= p[j].x);
                x = p[j].x;
            }
        }
    }

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_1000_entities_again(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    ecs_entity_t start = ecs_new(world);

    for (int i = 0; i < 1000; i ++) {
        int32_t v = rand();
        ecs_make_alive(world, i + start);
        ecs_set(world, i + start, Position, {v});

        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            Position *p = ecs_field(&it, Position, 0);

            test_assert(it.count == (i + 1));

            int32_t j, x = 0;
            for (j = 0; j < it.count; j ++) {  
                test_assert(x <= p[j].x);
                x = p[j].x;
            }
        }
    }

    for (int i = 0; i < 1000; i ++) {
        int32_t v = rand();
        ecs_set(world, i + start, Position, {v});
    }

    int32_t x = 0;

    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        Position *p = ecs_field(&it, Position, 0);

        test_assert(it.count == 1000);

        int32_t j;
        for (j = 0; j < it.count; j ++) {  
            test_assert(x <= p[j].x);
            x = p[j].x;
        }
    }

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_1000_entities_2_types(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    for (int i = 0; i < 500; i ++) {
        int32_t v = rand();
        ecs_insert(world, ecs_value(Position, {v}));
        ecs_entity_t e = ecs_insert(world, ecs_value(Position, {v}));
        ecs_add(world, e, Velocity);

        ecs_iter_t it = ecs_query_iter(world, q);
        int32_t count = 0, x = 0;
        while (ecs_query_next(&it)) {
            Position *p = ecs_field(&it, Position, 0);

            count += it.count;

            int32_t j;
            for (j = 0; j < it.count; j ++) {  
                test_assert(x <= p[j].x);
                x = p[j].x;
            }
        }

        test_int(count, (i + 1) * 2);
    }

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_1000_entities_2_types_again(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    ecs_entity_t start = ecs_new(world);

    for (int i = 0; i < 1000; i ++) {
        int32_t v = rand();
        ecs_make_alive(world, i + start);
        ecs_set(world, i + start, Position, {v});

        if (!(i % 2)) {
            ecs_add(world, i + start, Velocity);
        }

        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            Position *p = ecs_field(&it, Position, 0);

            int32_t j, x = 0;
            for (j = 0; j < it.count; j ++) {  
                test_assert(x <= p[j].x);
                x = p[j].x;
            }
        }
    }

    for (int i = 0; i < 1000; i ++) {
        int32_t v = rand();
        ecs_set(world, i + start, Position, {v});
    }

    ecs_iter_t it = ecs_query_iter(world, q);
    int32_t count = 0, x = 0;
    while (ecs_query_next(&it)) {
        Position *p = ecs_field(&it, Position, 0);

        count += it.count;

        int32_t j;
        for (j = 0; j < it.count; j ++) {  
            test_assert(x <= p[j].x);
            x = p[j].x;
        }
    }

    test_int(count, 1000);

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_1000_entities_add_type_after_sort(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    ecs_entity_t start = ecs_new(world);

    for (int i = 0; i < 500; i ++) {
        int32_t v = rand();
        ecs_make_alive(world, i + start);
        ecs_set(world, i + start, Position, {v});

        ecs_iter_t it = ecs_query_iter(world, q);
        int32_t count = 0;
        while (ecs_query_next(&it)) {
            Position *p = ecs_field(&it, Position, 0);

            count += it.count;

            int32_t j, x = 0;
            for (j = 0; j < it.count; j ++) {  
                test_assert(x <= p[j].x);
                x = p[j].x;
            }
        }

        test_int(count, i + 1);
    }

    for (int i = 0; i < 500; i ++) {
        int32_t v = rand();
        ecs_set(world, i + start, Position, {v});
    }

    for (int i = 0; i < 500; i ++) {
        int32_t v = rand();
        ecs_make_alive(world, i + start + 500);
        ecs_set(world, i + start + 500, Position, {v});
        ecs_add(world, i + start + 500, Velocity);

        ecs_iter_t it = ecs_query_iter(world, q);
        int32_t count = 0, x = 0;
        while (ecs_query_next(&it)) {
            Position *p = ecs_field(&it, Position, 0);

            count += it.count;

            int32_t j;
            for (j = 0; j < it.count; j ++) {  
                test_assert(x <= p[j].x);
                x = p[j].x;
            }
        }

        test_int(count, i + 500 + 1);
    }

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_1500_entities_3_types(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    for (int i = 0; i < 500; i ++) {
        ecs_insert(world, ecs_value(Position, {rand()}));
        ecs_entity_t e = ecs_insert(world, ecs_value(Position, {rand()}));
        ecs_add(world, e, Velocity);

        e = ecs_insert(world, ecs_value(Position, {rand()}));
        ecs_add(world, e, Mass);

        ecs_iter_t it = ecs_query_iter(world, q);
        int32_t count = 0, x = 0;
        while (ecs_query_next(&it)) {
            Position *p = ecs_field(&it, Position, 0);

            count += it.count;

            int32_t j;
            for (j = 0; j < it.count; j ++) {  
                test_assert(x <= p[j].x);
                x = p[j].x;
            }
        }

        test_int(count, (i + 1) * 3);
    }

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_2000_entities_4_types(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    for (int i = 0; i < 500; i ++) {
        ecs_insert(world, ecs_value(Position, {rand()}));

        ecs_entity_t e = ecs_insert(world, ecs_value(Position, {rand()}));
        ecs_add(world, e, Velocity);

        e = ecs_insert(world, ecs_value(Position, {rand()}));
        ecs_add(world, e, Mass);

        e = ecs_insert(world, ecs_value(Position, {rand()}));
        ecs_add(world, e, Velocity);
        ecs_add(world, e, Mass);        

        ecs_iter_t it = ecs_query_iter(world, q);
        int32_t count = 0, x = 0;
        while (ecs_query_next(&it)) {
            Position *p = ecs_field(&it, Position, 0);

            count += it.count;

            int32_t j;
            for (j = 0; j < it.count; j ++) {  
                test_assert(x <= p[j].x);
                x = p[j].x;
            }
        }

        test_int(count, (i + 1) * 4);
    }

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_shared_component(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ecs_add_pair(world, ecs_id(Position), EcsOnInstantiate, EcsInherit);

    ecs_entity_t base_1 = ecs_insert(world, ecs_value(Position, {0, 0}));
    ecs_entity_t base_2 = ecs_insert(world, ecs_value(Position, {3, 0}));
    ecs_entity_t base_3 = ecs_insert(world, ecs_value(Position, {7, 0}));

    ecs_entity_t e1 = ecs_insert(world, ecs_value(Position, {6, 0}));
    ecs_entity_t e2 = ecs_insert(world, ecs_value(Position, {1, 0}));
    ecs_entity_t e3 = ecs_insert(world, ecs_value(Position, {5, 0}));
    ecs_entity_t e4 = ecs_insert(world, ecs_value(Position, {2, 0}));
    ecs_entity_t e5 = ecs_insert(world, ecs_value(Position, {4, 0}));
    ecs_entity_t e6 = ecs_new_w_pair(world, EcsIsA, base_3);
    ecs_entity_t e7 = ecs_new_w_pair(world, EcsIsA, base_2);
    ecs_entity_t e8 = ecs_new_w_pair(world, EcsIsA, base_1);
    ecs_entity_t e9 = ecs_new_w_pair(world, EcsIsA, base_1);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position(self|up IsA)",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position,
    });

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == base_1);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 2);
    test_assert(it.entities[0] == e8);
    test_assert(it.entities[1] == e9);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 3);
    test_assert(it.entities[0] == e2);
    test_assert(it.entities[1] == e4);
    test_assert(it.entities[2] == base_2);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e7);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 4);
    test_assert(it.entities[0] == e5);
    test_assert(it.entities[1] == e3);
    test_assert(it.entities[2] == e1);
    test_assert(it.entities[3] == base_3);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e6);

    test_assert(!ecs_query_next(&it));

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_shared_component_childof(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);

    ecs_entity_t base_1 = ecs_insert(world, ecs_value(Position, {0, 0}));
    ecs_entity_t base_2 = ecs_insert(world, ecs_value(Position, {3, 0}));
    ecs_entity_t base_3 = ecs_insert(world, ecs_value(Position, {7, 0}));

    ecs_entity_t e1 = ecs_insert(world, ecs_value(Position, {6, 0}));
    ecs_entity_t e2 = ecs_insert(world, ecs_value(Position, {1, 0}));
    ecs_entity_t e3 = ecs_insert(world, ecs_value(Position, {5, 0}));
    ecs_entity_t e4 = ecs_insert(world, ecs_value(Position, {2, 0}));
    ecs_entity_t e5 = ecs_insert(world, ecs_value(Position, {4, 0}));
    ecs_entity_t e6 = ecs_new_w_pair(world, EcsChildOf, base_3);
    ecs_entity_t e7 = ecs_new_w_pair(world, EcsChildOf, base_2);
    ecs_entity_t e8 = ecs_new_w_pair(world, EcsChildOf, base_1);
    ecs_entity_t e9 = ecs_new_w_pair(world, EcsChildOf, base_1);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position(self|up ChildOf)",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position,
    });

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == base_1);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 2);
    test_assert(it.entities[0] == e8);
    test_assert(it.entities[1] == e9);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 3);
    test_assert(it.entities[0] == e2);
    test_assert(it.entities[1] == e4);
    test_assert(it.entities[2] == base_2);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e7);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 4);
    test_assert(it.entities[0] == e5);
    test_assert(it.entities[1] == e3);
    test_assert(it.entities[2] == e1);
    test_assert(it.entities[3] == base_3);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 1);
    test_assert(it.entities[0] == e6);

    test_assert(!ecs_query_next(&it));

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_2_entities_2_types(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    ecs_insert(world, ecs_value(Position, {rand()}));

    ecs_entity_t e = ecs_insert(world, ecs_value(Position, {rand()}));
    ecs_add(world, e, Velocity); 

    ecs_iter_t it = ecs_query_iter(world, q);
    int32_t count = 0;
    while (ecs_query_next(&it)) {
        Position *p = ecs_field(&it, Position, 0);

        count += it.count;

        int32_t j, x = 0;
        for (j = 0; j < it.count; j ++) {  
            test_assert(x <= p[j].x);
            x = p[j].x;
        }
    }

    test_int(count, 2);

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_3_entities_3_types(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    ecs_insert(world, ecs_value(Position, {rand()}));

    ecs_entity_t e = ecs_insert(world, ecs_value(Position, {rand()}));
    ecs_add(world, e, Velocity);

    e = ecs_insert(world, ecs_value(Position, {rand()}));
    ecs_add(world, e, Mass);   

    ecs_iter_t it = ecs_query_iter(world, q);
    int32_t count = 0;
    while (ecs_query_next(&it)) {
        Position *p = ecs_field(&it, Position, 0);

        count += it.count;

        int32_t j, x = 0;
        for (j = 0; j < it.count; j ++) {  
            test_assert(x <= p[j].x);
            x = p[j].x;
        }
    }

    test_int(count, 3);

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_3_entities_3_types_2(void) {
    ecs_world_t *world = ecs_mini();

    ECS_TAG(world, Tag);
    ECS_TAG(world, Foo);
    ECS_TAG(world, Bar);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Tag",
        .order_by_callback = compare_entity
    });

    ecs_entity_t e1 = ecs_new_w(world, Tag);
    ecs_entity_t e2 = ecs_new_w(world, Tag);
    ecs_entity_t e3 = ecs_new_w(world, Tag);

    ecs_add_id(world, e1, Foo);
    ecs_add_id(world, e2, Bar);

    ecs_iter_t it = ecs_query_iter(world, q);
    ecs_entity_t e = 0;
    int32_t count = 0;
    while (ecs_query_next(&it)) {
        count += it.count;

        int32_t i;
        for (i = 0; i < it.count; i ++) {  
            test_assert(e < it.entities[i]);
            e = it.entities[i];
            test_assert(e == e1 || e == e2 || e == e3);
        }
    }

    test_int(count, 3);

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_4_entities_4_types(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    ecs_insert(world, ecs_value(Position, {rand()}));

    ecs_entity_t e = ecs_insert(world, ecs_value(Position, {rand()}));
    ecs_add(world, e, Velocity);

    e = ecs_insert(world, ecs_value(Position, {rand()}));
    ecs_add(world, e, Mass);

    e = ecs_insert(world, ecs_value(Position, {rand()}));
    ecs_add(world, e, Velocity);
    ecs_add(world, e, Mass);     

    ecs_iter_t it = ecs_query_iter(world, q);
    int32_t count = 0;
    while (ecs_query_next(&it)) {
        Position *p = ecs_field(&it, Position, 0);

        count += it.count;

        int32_t j, x = 0;
        for (j = 0; j < it.count; j ++) {  
            test_assert(x <= p[j].x);
            x = p[j].x;
        }
    }

    test_int(count, 4);

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_w_tags_only(void) {
    ecs_world_t *world = ecs_mini();

    ECS_TAG(world, Tag);

    ecs_query_t *q = ecs_query(world, {
        .terms = {{Tag}},
        .order_by_callback = compare_entity
    });

    ecs_entity_t root = ecs_new(world);
    ecs_entity_t e1 = ecs_new_w_pair(world, EcsChildOf, root);
    ecs_entity_t e2 = ecs_new_w_pair(world, EcsChildOf, root);

    ecs_add(world, e2, Tag);
    ecs_add(world, e1, Tag);

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(ecs_query_next(&it));
    test_int(it.count, 2);
    test_int(it.entities[0], e1);
    test_int(it.entities[1], e2);
    ecs_iter_fini(&it);

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_childof_marked(void) {
    ecs_world_t *world = ecs_mini();

    ECS_TAG(world, Tag);

    ecs_query_t *q = ecs_query(world, {
        .terms = {{Tag}},
        .order_by_callback = compare_entity
    });

    ecs_entity_t root = ecs_new(world);
    ecs_entity_t e1 = ecs_new_w_pair(world, EcsChildOf, root);
    ecs_entity_t e2 = ecs_new_w_pair(world, EcsChildOf, e1);
    ecs_entity_t e3 = ecs_new_w_pair(world, EcsChildOf, root);

    ecs_add(world, e3, Tag);
    ecs_add(world, e1, Tag);

    // Trigger sorting
    ecs_iter_t it = ecs_query_iter(world, q);
    ecs_iter_fini(&it);

    ecs_delete(world, root);

    test_assert(!ecs_is_alive(world, root));
    test_assert(!ecs_is_alive(world, e1));
    test_assert(!ecs_is_alive(world, e2));
    test_assert(!ecs_is_alive(world, e3));

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_isa_marked(void) {
    ecs_world_t *world = ecs_mini();

    ECS_TAG(world, Tag);
    ecs_add_pair(world, Tag, EcsOnInstantiate, EcsInherit);

    ecs_query_t *q = ecs_query(world, {
        .terms = {{Tag}},
        .order_by_callback = compare_entity
    });

    ecs_entity_t root = ecs_new(world);
    ecs_entity_t e1 = ecs_new_w_pair(world, EcsIsA, root);
    ecs_entity_t e2 = ecs_new_w_pair(world, EcsIsA, e1);
    ecs_entity_t e3 = ecs_new_w_pair(world, EcsIsA, root);

    ecs_add(world, e3, Tag);
    ecs_add(world, e1, Tag);

    // Trigger sorting
    ecs_iter_t it = ecs_query_iter(world, q);
    ecs_delete(world, root);
    ecs_iter_fini(&it);

    test_assert(!ecs_is_alive(world, root));
    test_int(1, ecs_get_type(world, e1)->count);
    test_assert(ecs_has(world, e1, Tag));
    test_int(1, ecs_get_type(world, e2)->count);
    test_assert(ecs_has_pair(world, e2, EcsIsA, e1));
    test_int(1, ecs_get_type(world, e3)->count);
    test_assert(ecs_has(world, e3, Tag));

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_relation_marked(void) {
    ecs_world_t *world = ecs_mini();

    ECS_TAG(world, Tag);
    ECS_TAG(world, Rel);

    ecs_query_t *q = ecs_query(world, {
        .terms = {{Tag}},
        .order_by_callback = compare_entity
    });

    ecs_entity_t root = ecs_new(world);
    ecs_entity_t e1 = ecs_new_w_pair(world, Rel, root);
    ecs_entity_t e2 = ecs_new_w_pair(world, Rel, e1);
    ecs_entity_t e3 = ecs_new_w_pair(world, Rel, root);

    ecs_add(world, e3, Tag);
    ecs_add(world, e1, Tag);

    // Trigger sorting
    ecs_iter_t it = ecs_query_iter(world, q);
    ecs_iter_fini(&it);

    ecs_delete(world, root);

    test_assert(!ecs_is_alive(world, root));
    test_int(1, ecs_get_type(world, e1)->count);
    test_assert(ecs_has(world, e1, Tag));
    test_int(1, ecs_get_type(world, e2)->count);
    test_assert(ecs_has_pair(world, e2, Rel, e1));
    test_int(1, ecs_get_type(world, e3)->count);
    test_assert(ecs_has(world, e3, Tag));

    ecs_query_fini(q);

    ecs_fini(world);
}

static bool dummy_compare_invoked = false;

static
int dummy_compare(
    ecs_entity_t e1,
    const void *ptr1,
    ecs_entity_t e2,
    const void *ptr2)
{
    dummy_compare_invoked = true;
    return (e1 > e2) - (e1 < e2);
}

void OrderBy_dont_resort_after_set_unsorted_component(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_query_t *q = ecs_query(world, {
        .expr = "[in] Position",
        .order_by = ecs_id(Position),
        .order_by_callback = dummy_compare
    });

    ecs_entity_t e1 = ecs_new(world);
    ecs_set(world, e1, Position, {0, 0});
    ecs_set(world, e1, Velocity, {0, 0});

    ecs_entity_t e2 = ecs_new(world);
    ecs_set(world, e2, Position, {0, 0});
    ecs_set(world, e2, Velocity, {0, 0});

    // Initial sort
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) { }
    test_bool(dummy_compare_invoked, true); 
    dummy_compare_invoked = false;

    // No changes, shouldn't sort
    it = ecs_query_iter(world, q);
    test_bool(dummy_compare_invoked, false);
    ecs_iter_fini(&it);

    // No change in sorted component, shouldn't sort
    ecs_modified(world, e2, Velocity);
    it = ecs_query_iter(world, q);
    test_bool(dummy_compare_invoked, false);
    ecs_iter_fini(&it);

    // Change in sorted component, should sort
    ecs_modified(world, e2, Position);
    it = ecs_query_iter(world, q);
    test_bool(dummy_compare_invoked, true);
    ecs_iter_fini(&it);

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_dont_resort_after_set_unsorted_component_w_tag(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Velocity);

    ecs_entity_t Tag = ecs_new_low_id(world); // wiggle id between components

    ECS_COMPONENT(world, Position);

    /* Because Position's id is larger than the Tag, its type index in the
     * storage table is different from the actual table. Ensure that this 
     * doesn't break the test. */

    test_assert(Tag < ecs_id(Position));

    ecs_query_t *q = ecs_query(world, {
        .expr = "[in] Position",
        .order_by = ecs_id(Position),
        .order_by_callback = dummy_compare
    });

    ecs_entity_t e1 = ecs_new_w_id(world, Tag);
    ecs_set(world, e1, Position, {0, 0});
    ecs_set(world, e1, Velocity, {0, 0});

    ecs_entity_t e2 = ecs_new_w_id(world, Tag);
    ecs_set(world, e2, Position, {0, 0});
    ecs_set(world, e2, Velocity, {0, 0});

    // Initial sort
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) { }
    test_bool(dummy_compare_invoked, true);
    dummy_compare_invoked = false;

    // No changes, shouldn't sort
    it = ecs_query_iter(world, q);
    test_bool(dummy_compare_invoked, false);
    ecs_iter_fini(&it);

    // No change in sorted component, shouldn't sort
    ecs_modified(world, e2, Velocity);
    it = ecs_query_iter(world, q);
    test_bool(dummy_compare_invoked, false);
    ecs_iter_fini(&it);

    // Change in sorted component, should sort
    ecs_modified(world, e2, Position);
    it = ecs_query_iter(world, q);
    test_bool(dummy_compare_invoked, true);
    ecs_iter_fini(&it);

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_dont_resort_after_set_unsorted_component_w_tag_w_out_term(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Velocity);

    ecs_entity_t Tag = ecs_new_low_id(world); // wiggle id between components

    ECS_COMPONENT(world, Position);

    /* Same as previous testcase, but with query out column vs. modified */

    test_assert(Tag < ecs_id(Position));

    // Sorted query
    ecs_query_t *q = ecs_query(world, {
        .expr = "[in] Position",
        .order_by = ecs_id(Position),
        .order_by_callback = dummy_compare
    });

    // Dummy queries that mutate
    ecs_query_t *q_a = ecs_query(world, { .expr = "Position" }); // [inout]
    ecs_query_t *q_b = ecs_query(world, { .expr = "[out] Position" });
    ecs_query_t *q_c = ecs_query(world, { .expr = "[out] Velocity" });

    ecs_entity_t e1 = ecs_new_w_id(world, Tag);
    ecs_set(world, e1, Position, {0, 0});
    ecs_set(world, e1, Velocity, {0, 0});

    ecs_entity_t e2 = ecs_new_w_id(world, Tag);
    ecs_set(world, e2, Position, {0, 0});
    ecs_set(world, e2, Velocity, {0, 0});

    // Initial sort
    ecs_iter_t itq = ecs_query_iter(world, q);
    test_bool(dummy_compare_invoked, true); 
    while (ecs_query_next(&itq)) { }
    dummy_compare_invoked = false;

    // No changes, shouldn't sort
    ecs_iter_t it = ecs_query_iter(world, q);
    test_bool(dummy_compare_invoked, false);
    ecs_iter_fini(&it);

    // No change in sorted component, shouldn't sort
    { ecs_iter_t it = ecs_query_iter(world, q_c); while (ecs_query_next(&it)); }
    it = ecs_query_iter(world, q);
    test_bool(dummy_compare_invoked, false);
    ecs_iter_fini(&it);

    // Change in sorted component (inout), should sort
    { ecs_iter_t it = ecs_query_iter(world, q_a); while (ecs_query_next(&it)); }
    itq = ecs_query_iter(world, q);
    test_bool(dummy_compare_invoked, true);
    while (ecs_query_next(&itq)) { }
    dummy_compare_invoked = false;

    // Change in sorted component (out), should sort
    { ecs_iter_t it = ecs_query_iter(world, q_b); while (ecs_query_next(&it)); }
    it = ecs_query_iter(world, q);
    test_bool(dummy_compare_invoked, true);
    dummy_compare_invoked = false;
    ecs_iter_fini(&it);

    ecs_query_fini(q);

    ecs_query_fini(q_a);
    ecs_query_fini(q_b);
    ecs_query_fini(q_c);

    ecs_fini(world);
}

void OrderBy_sort_component_not_queried_for(void) {
    install_test_abort();

    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_log_set_level(-4);
    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Velocity),
        .order_by_callback = compare_position
    });

    test_assert(q == NULL);

    ecs_fini(world);
}

void OrderBy_sort_by_wildcard(void) {
    install_test_abort();

    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);

    test_expect_abort();
    ecs_query(world, {
        .expr = "(Position, *)",
        .order_by = ecs_pair(ecs_id(Position), EcsWildcard),
        .order_by_callback = compare_position
    });
}

void Sorting_sort_shared_w_delete(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ecs_add_pair(world, ecs_id(Position), EcsOnInstantiate, EcsInherit);

    ecs_query_t *q = ecs_query(world, {
        .terms = {
            { .id = ecs_id(Position), .inout = EcsIn }
        },
        .order_by_callback = compare_position,
        .order_by = ecs_id(Position)
    });

    ecs_entity_t base = ecs_insert(world, ecs_value(Position, {0, 0}));
    ecs_add_id(world, base, EcsPrefab);

    ecs_entity_t e1 = ecs_new_w_pair(world, EcsIsA, base);
    ecs_entity_t e2 = ecs_new_w_pair(world, EcsIsA, base);
    ecs_entity_t e3 = ecs_new_w_pair(world, EcsIsA, base);
    ecs_entity_t e4 = ecs_new_w_pair(world, EcsIsA, base);
    ecs_entity_t e5 = ecs_new_w_pair(world, EcsIsA, base);

    {
        ecs_iter_t it = ecs_query_iter(world, q);
        test_bool(true, ecs_query_next(&it));
        test_int(5, it.count);
        test_uint(e1, it.entities[0]);
        test_uint(e2, it.entities[1]);
        test_uint(e3, it.entities[2]);
        test_uint(e4, it.entities[3]);
        test_uint(e5, it.entities[4]);
        test_bool(false, ecs_query_next(&it));
    }

    ecs_delete(world, e1);
    ecs_delete(world, e2);
    ecs_delete(world, e3);
    ecs_delete(world, e4);

    {
        ecs_iter_t it = ecs_query_iter(world, q);
        test_bool(true, ecs_query_next(&it));
        test_int(1, it.count);
        test_uint(e5, it.entities[4]);
        test_bool(false, ecs_query_next(&it));
    }

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_sort_not_term(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);

    ecs_log_set_level(-4);
    ecs_query_t *q = ecs_query(world, {
        .expr = "!Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    test_assert(q == NULL);

    ecs_fini(world);
}

void OrderBy_sort_or_term(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_log_set_level(-4);
    ecs_query_t *q = ecs_query(world, {
        .expr = "Position || Velocity",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    test_assert(q == NULL);

    ecs_fini(world);
}

void OrderBy_sort_optional_term(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);

    ecs_log_set_level(-4);
    ecs_query_t *q = ecs_query(world, {
        .expr = "?Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    test_assert(q == NULL);

    ecs_fini(world);
}

void OrderBy_sort_shared_w_delete(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ecs_add_pair(world, ecs_id(Position), EcsOnInstantiate, EcsInherit);

    ecs_query_t *q = ecs_query(world, {
        .terms = {
            { .id = ecs_id(Position), .inout = EcsIn }
        },
        .order_by_callback = compare_position,
        .order_by = ecs_id(Position)
    });

    ecs_entity_t base = ecs_insert(world, ecs_value(Position, {0, 0}));
    ecs_add_id(world, base, EcsPrefab);

    ecs_entity_t e1 = ecs_new_w_pair(world, EcsIsA, base);
    ecs_entity_t e2 = ecs_new_w_pair(world, EcsIsA, base);
    ecs_entity_t e3 = ecs_new_w_pair(world, EcsIsA, base);
    ecs_entity_t e4 = ecs_new_w_pair(world, EcsIsA, base);
    ecs_entity_t e5 = ecs_new_w_pair(world, EcsIsA, base);

    {
        ecs_iter_t it = ecs_query_iter(world, q);
        test_bool(true, ecs_query_next(&it));
        test_int(5, it.count);
        test_uint(e1, it.entities[0]);
        test_uint(e2, it.entities[1]);
        test_uint(e3, it.entities[2]);
        test_uint(e4, it.entities[3]);
        test_uint(e5, it.entities[4]);
        test_bool(false, ecs_query_next(&it));
    }

    ecs_delete(world, e1);
    ecs_delete(world, e2);
    ecs_delete(world, e3);
    ecs_delete(world, e4);

    {
        ecs_iter_t it = ecs_query_iter(world, q);
        test_bool(true, ecs_query_next(&it));
        test_int(1, it.count);
        test_uint(e5, it.entities[4]);
        test_bool(false, ecs_query_next(&it));
    }

    ecs_fini(world);
}

static int ctor_invoked = 0;
static int move_invoked = 0;
static int dtor_invoked = 0;

ECS_CTOR(Velocity, ptr, {
    ctor_invoked ++;
})

ECS_MOVE(Velocity, dst, src, {
    *dst = *src;
    move_invoked ++;
})

ECS_DTOR(Velocity, ptr, {
    dtor_invoked ++;
})

void OrderBy_sort_w_nontrivial_component(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_set_hooks(world, Velocity, {
        .ctor = ecs_ctor(Velocity),
        .move = ecs_move(Velocity),
        .dtor = ecs_dtor(Velocity)
    });

    ecs_entity_t e1 = ecs_insert(world, ecs_value(Position, {3, 0}));
    ecs_entity_t e2 = ecs_insert(world, ecs_value(Position, {1, 0}));
    ecs_entity_t e3 = ecs_insert(world, ecs_value(Position, {5, 0}));
    ecs_entity_t e4 = ecs_insert(world, ecs_value(Position, {2, 0}));
    ecs_entity_t e5 = ecs_insert(world, ecs_value(Position, {4, 0}));

    ecs_set(world, e1, Velocity, {0, 3});
    ecs_set(world, e2, Velocity, {0, 1});
    ecs_set(world, e3, Velocity, {0, 5});
    ecs_set(world, e4, Velocity, {0, 2});
    ecs_set(world, e5, Velocity, {0, 4});

    ctor_invoked = 0;
    move_invoked = 0;
    dtor_invoked = 0;

    ecs_query_t *q = ecs_query_init(world, &(ecs_query_desc_t){
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position
    });

    ecs_iter_t it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 5);

    test_assert(it.entities[0] == e2);
    test_assert(it.entities[1] == e4);
    test_assert(it.entities[2] == e1);
    test_assert(it.entities[3] == e5);
    test_assert(it.entities[4] == e3);

    test_assert(!ecs_query_next(&it));

    {
        const Velocity *v = ecs_get(world, e1, Velocity);
        test_int(v->x, 0); test_int(v->y, 3);
    }
    {
        const Velocity *v = ecs_get(world, e2, Velocity);
        test_int(v->x, 0); test_int(v->y, 1);
    }
    {
        const Velocity *v = ecs_get(world, e3, Velocity);
        test_int(v->x, 0); test_int(v->y, 5);
    }
    {
        const Velocity *v = ecs_get(world, e4, Velocity);
        test_int(v->x, 0); test_int(v->y, 2);
    }
    {
        const Velocity *v = ecs_get(world, e5, Velocity);
        test_int(v->x, 0); test_int(v->y, 4);
    }

    test_int(ctor_invoked, 4);
    test_int(move_invoked, 12);
    test_int(dtor_invoked, 4);

    ecs_fini(world);
}

void OrderBy_order_empty_table(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_TAG(world, Foo);

    ecs_entity_t e1 = ecs_insert(world, ecs_value(Position, {3, 0}));
    ecs_entity_t e2 = ecs_insert(world, ecs_value(Position, {1, 0}));
    ecs_entity_t e3 = ecs_insert(world, ecs_value(Position, {5, 0}));
    ecs_entity_t e4 = ecs_insert(world, ecs_value(Position, {2, 0}));
    ecs_entity_t e5 = ecs_insert(world, ecs_value(Position, {4, 0}));

    // Create empty table
    ecs_add(world, e5, Foo);
    ecs_delete(world, e5);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position,
        .flags = EcsQueryMatchEmptyTables
    });

    ecs_iter_t it = ecs_query_iter(world, q);

    test_assert(ecs_query_next(&it));
    test_int(it.count, 4);

    test_assert(it.entities[0] == e2);
    test_assert(it.entities[1] == e4);
    test_assert(it.entities[2] == e1);
    test_assert(it.entities[3] == e3);
    test_assert(!ecs_query_next(&it));

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_order_empty_table_only(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);

    // Create empty table
    ecs_entity_t e = ecs_new_w(world, Position);
    ecs_delete(world, e);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position,
        .flags = EcsQueryMatchEmptyTables
    });

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(!ecs_query_next(&it));

    ecs_query_fini(q);

    ecs_fini(world);
}

void OrderBy_order_empty_table_only_2_tables(void) {
    ecs_world_t *world = ecs_mini();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    // Create empty tables
    ecs_entity_t e1 = ecs_new_w(world, Position);
    ecs_delete(world, e1);

    ecs_entity_t e2 = ecs_new_w(world, Position);
    ecs_add(world, e2, Velocity);
    ecs_delete(world, e2);

    ecs_query_t *q = ecs_query(world, {
        .expr = "Position",
        .order_by = ecs_id(Position),
        .order_by_callback = compare_position,
        .flags = EcsQueryMatchEmptyTables
    });

    ecs_iter_t it = ecs_query_iter(world, q);
    test_assert(!ecs_query_next(&it));

    ecs_query_fini(q);

    ecs_fini(world);
}
