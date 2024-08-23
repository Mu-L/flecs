/**
 * @file query/engine/eval_flattened.c
 * @brief Flattened hierarchy iteration.
 */

#include "../../private_api.h"

bool flecs_query_flat_fixed(
    const ecs_query_op_t *op,
    bool redo,
    const ecs_query_run_ctx_t *ctx,
    ecs_query_and_flat_ctx_t *op_ctx)
{
    if (!redo) {
        /* Initialize context for first result */

        /* Get first and second elements of pair */
        ecs_entity_t first = ECS_PAIR_FIRST(op_ctx->flatten_id);
        ecs_entity_t second = ECS_PAIR_SECOND(op_ctx->flatten_id);
        ecs_assert(second != EcsWildcard, ECS_INTERNAL_ERROR, NULL);

        /* We're matching a (ChildOf, parent) pair. */
        second = flecs_entities_get_alive(ctx->world, second);
        if (!second) {
            /* This is a query for root entities, and root entities can't
                * be flattened. (R, 0) pairs are only valid if R is ChildOf. */
            ecs_assert(first == EcsChildOf, ECS_INTERNAL_ERROR, NULL);
            return false;
        }

        /* Get the (Children, R) component from the parent. If the matched
            * relationship is ChildOf, this will fetch (Children, ChildOf). */
        const EcsChildren *children = ecs_get_pair(
            ctx->world, second, EcsChildren, first);
        if (!children) {
            return false;
        }

        /* Prepare the query context with the children array, number of 
            * children and index of the currently iterated child. */
        op_ctx->children = ecs_vec_first_t(
            &children->children, ecs_entity_t);
        op_ctx->children_count = ecs_vec_count(&children->children);

        /* If entity has Children component, it must have children. */
        ecs_assert(op_ctx->children_count != 0, ECS_INTERNAL_ERROR, NULL);

        op_ctx->cur_child = -1;
    }

    /* Move to the next child in the children array. */
    ecs_assert(op_ctx->cur_child < op_ctx->children_count, 
        ECS_INTERNAL_ERROR, NULL);
    int32_t cur = ++ op_ctx->cur_child;
    if (op_ctx->cur_child == op_ctx->children_count) {
        return false;
    }

    /* Assign the child to the source of the operation. This will initialize the
     * source variable (typically $this) with the entity, which allows 
     * subsequent operations that use the same variable to use the entity. */
    flecs_query_set_src(op, op_ctx->children[cur], ctx);
    
    /* Set matched id to (R, parent) */
    ctx->it->ids[op->field_index] = op_ctx->flatten_id;

    return true;
}

bool flecs_query_flat_wildcard(
    const ecs_query_op_t *op,
    bool redo,
    const ecs_query_run_ctx_t *ctx,
    ecs_query_and_flat_ctx_t *op_ctx)
{
    /* Get relationship of (R, *) pair */
    ecs_entity_t first = ECS_PAIR_FIRST(op_ctx->flatten_id);
    ecs_assert(ECS_PAIR_SECOND(op_ctx->flatten_id) == EcsWildcard,
        ECS_INTERNAL_ERROR, NULL);

    if (!redo) {
        /* Initialize context for first result */

        /* Get id record and create iterator for (Parent, R) */
        ecs_id_record_t *idr = flecs_id_record_get(
            ctx->world, ecs_pair_t(EcsParent, first));
        if (!idr) {
            /* No flattened hierarchies */
            return false;
        }

        if (!flecs_table_cache_iter(&idr->cache, &op_ctx->and.it)) {
            /* No tables with flattened hierarchies */
            return false;
        }
    }

    ecs_table_record_t *tr;
    ecs_table_t *table;

repeat:
    if (!redo) {
        /* Find next table with (Parent, R) */
        tr = flecs_table_cache_next(&op_ctx->and.it, ecs_table_record_t);
        if (!tr) {
            return false;
        }

        table = tr->hdr.table;
        op_ctx->cur_child = 0;
        op_ctx->children_count = ecs_table_count(table);
        op_ctx->children = &ecs_table_entities(table)[0];
        ecs_assert(op_ctx->children_count != 0, ECS_INTERNAL_ERROR, NULL);
    } else {
        /* Iterate next child in table */
        tr = (ecs_table_record_t*)op_ctx->and.it.cur;
        ecs_assert(tr != NULL, ECS_INTERNAL_ERROR, NULL);
        table = tr->hdr.table;
        if (++ op_ctx->cur_child == op_ctx->children_count) {
            redo = false;
            goto repeat;
        }
    }

    int32_t cur = op_ctx->cur_child;
    ecs_assert(cur < op_ctx->children_count, ECS_INTERNAL_ERROR, NULL);

    /* Set entity as source, see above. */
    flecs_query_set_src(op, op_ctx->children[cur], ctx);

    /* Get parent for entity from Parent component */
    ecs_entity_t parent = ECS_ELEM_T(
        table->data.columns[tr->column].data, EcsParent, cur)->parent;

    /* Set matched id to (R, parent) */
    ctx->it->ids[op->field_index] = ecs_pair(first, parent);

    /* If the pair id matched by the term contains a variable, populate it. This
     * ensures that for a (R, $var) pair, the $var variable is initialized with
     * the parent. */
    flecs_query_set_vars(op, ecs_pair(first, parent), ctx);
    
    return true;
}

bool flecs_query_flat(
    const ecs_query_op_t *op,
    bool redo,
    const ecs_query_run_ctx_t *ctx)
{
    ecs_query_and_flat_ctx_t *op_ctx = flecs_op_ctx(ctx, and_flat);

    /* Get component id to match. This will be a pair that looks like 
     * (ChildOf, parent) or (ChildOf, *). */
    op_ctx->flatten_id = flecs_query_op_get_id(op, ctx);

    /* Flattened iteration only applies to pairs that have a relationship
     * with the CanFlatten trait, so id must be a pair. */
    ecs_assert(ECS_IS_PAIR(op_ctx->flatten_id), ECS_INTERNAL_ERROR, NULL);

    /* Switch between querying for a single parent or all parents */
    if (ECS_PAIR_SECOND(op_ctx->flatten_id) != EcsWildcard) {
        return flecs_query_flat_fixed(op, redo, ctx, op_ctx);
    } else {
        return flecs_query_flat_wildcard(op, redo, ctx, op_ctx);
    }
}

bool flecs_query_ids_flat(
    const ecs_query_op_t *op,
    const ecs_query_run_ctx_t *ctx,
    ecs_id_t id)
{
    /* If id is not a pair, this check can't be for a flattened relationship. */
    if (ECS_IS_PAIR(id)) {
        ecs_id_t wc = ecs_pair(ECS_PAIR_FIRST(id), EcsWildcard);
        ecs_id_record_t *idr = flecs_id_record_get(ctx->world, wc);
        if (!idr) {
            /* Id record should exist for flattened */
            return false;
        }

        if (idr->flags & EcsIdCanFlatten) {
            ecs_entity_t first = ECS_PAIR_FIRST(id);
            ecs_entity_t second = ECS_PAIR_SECOND(id);

            /* The pair has a relationship that can be flattened. Check
             * if second element of the pair flattened children. */
            ecs_entity_t parent = flecs_entities_get_alive(ctx->world, second);
            ecs_assert(parent != 0, ECS_INTERNAL_ERROR, NULL);

            /* Check for (Children, R) component which stores flattened trees 
             * for relationship R. */
            if (ecs_owns_pair(ctx->world, parent, ecs_id(EcsChildren), first)) {
                /* Parent has flattened children */
                return true;
            }
        }
    }

    return false;
}
