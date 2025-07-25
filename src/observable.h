/**
 * @file observable.h
 * @brief Functions for emitting events.
 */

#ifndef FLECS_OBSERVABLE_H
#define FLECS_OBSERVABLE_H

/** All observers for a specific (component) id */
typedef struct ecs_event_id_record_t {
    /* Triggers for Self */
    ecs_map_t self;                  /* map<trigger_id, trigger_t> */
    ecs_map_t self_up;               /* map<trigger_id, trigger_t> */
    ecs_map_t up;                    /* map<trigger_id, trigger_t> */

    ecs_map_t observers;             /* map<trigger_id, trigger_t> */

    /* Triggers for SuperSet, SubSet */
    ecs_map_t set_observers;         /* map<trigger_id, trigger_t> */

    /* Triggers for Self with non-This subject */
    ecs_map_t entity_observers;      /* map<trigger_id, trigger_t> */

    /* Number of active observers for (component) id */
    int32_t observer_count;
} ecs_event_id_record_t;

typedef struct ecs_observer_impl_t {
    ecs_observer_t pub;

    int32_t *last_event_id;     /**< Last handled event id */
    int32_t last_event_id_storage;

    ecs_flags32_t flags;        /**< Observer flags */

    int8_t term_index;          /**< Index of the term in parent observer (single term observers only) */
    ecs_id_t register_id;       /**< Id observer is registered with (single term observers only) */
    uint64_t id;                /**< Internal id (not entity id) */

    ecs_vec_t children;         /**< If multi observer, vector stores child observers */

    ecs_query_t *not_query;     /**< Query used to populate observer data when a
                                     term with a not operator triggers. */

    /* Mixins */
    flecs_poly_dtor_t dtor;
} ecs_observer_impl_t;

#define flecs_observer_impl(observer) (ECS_CONST_CAST(ecs_observer_impl_t*, observer))

/* Get event record (all observers for an event). */
ecs_event_record_t* flecs_event_record_get(
    const ecs_observable_t *o,
    ecs_entity_t event);

/* Get or create event record. */
ecs_event_record_t* flecs_event_record_ensure(
    ecs_observable_t *o,
    ecs_entity_t event);

/* Get event id record (all observers for an event/component). */
ecs_event_id_record_t* flecs_event_id_record_get(
    const ecs_event_record_t *er,
    ecs_id_t id);

/* Get or create event id record. */
ecs_event_id_record_t* flecs_event_id_record_ensure(
    ecs_world_t *world,
    ecs_event_record_t *er,
    ecs_id_t id);

/* Remove event id record. */
void flecs_event_id_record_remove(
    ecs_event_record_t *er,
    ecs_id_t id);

/* Initialize observable (typically the world). */
void flecs_observable_init(
    ecs_observable_t *observable);

/* Free observable. */
void flecs_observable_fini(
    ecs_observable_t *observable);

/* Check if any observers exist for event/component. */
bool flecs_observers_exist(
    ecs_observable_t *observable,
    ecs_id_t id,
    ecs_entity_t event);

/* Initialize observer. */
ecs_observer_t* flecs_observer_init(
    ecs_world_t *world,
    ecs_entity_t entity,
    const ecs_observer_desc_t *desc);

/* Free observer. */
void flecs_observer_fini(
    ecs_observer_t *observer);

/* Emit event. */
void flecs_emit( 
    ecs_world_t *world,
    ecs_world_t *stage,
    ecs_event_desc_t *desc);

/* Default function to set in iter::next */
bool flecs_default_next_callback(
    ecs_iter_t *it);

/* Invoke observers. */
void flecs_observers_invoke(
    ecs_world_t *world,
    ecs_map_t *observers,
    ecs_iter_t *it,
    ecs_table_t *table,
    ecs_entity_t trav);

/* Invalidate reachable cache. */
void flecs_emit_propagate_invalidate(
    ecs_world_t *world,
    ecs_table_t *table,
    int32_t offset,
    int32_t count);

/* Set bit indicating that observer is disabled. */
void flecs_observer_set_disable_bit(
    ecs_world_t *world,
    ecs_entity_t e,
    ecs_flags32_t bit,
    bool cond);

#endif
