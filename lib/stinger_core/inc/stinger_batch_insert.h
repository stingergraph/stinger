#ifndef STINGER_BATCH_INSERT_H_
#define STINGER_BATCH_INSERT_H_

#include "stinger.h"
#include "stinger_internal.h"
#include "stinger_atomics.h"
#include "x86_full_empty.h"

#include <vector>
#include <algorithm>

template<typename update, typename update_iterator>
class BatchInserter
{
protected:
    // Result codes
    // Before returning to caller, we subtract 10 to make return codes match up with functions like stinger_incr_edge
    enum result_codes {
        PENDING = 0,
        EDGE_ADDED = 11,    // 1
        EDGE_UPDATED = 10,  // 0
        EDGE_NOT_ADDED = 9  // -1
    };

    struct source_funcs
    {
        static int64_t
        get(const update &x){
            return x.source();
        }
        static void
        set(update &x, int64_t neighbor){
            x.set_source(neighbor);
        }
        static bool
        equals(const update &a, const update &b){
            return a.source() == b.source();
        }
        static bool
        compare(const update &a, const update &b){
            return a.source() < b.source();
        }
        static bool
        sort(const update &a, const update &b){
            return (a.type() != b.type()) ? a.type() < b.type() :
                   (a.source() != b.source()) ? a.source() < b.source() :
                   (a.destination() != b.destination()) ? a.destination() < b.destination() :
                   (a.time() != b.time()) ? a.time() < b.time() :
                   false;
        }
    };

    struct dest_funcs
    {
        static int64_t
        get(const update &x){
            return x.destination();
        }
        static void
        set(update &x, int64_t neighbor){
            x.set_destination(neighbor);
        }
        static bool
        equals(const update &a, const update &b){
            return a.destination() == b.destination();
        }
        static bool
        compare(const update &a, const update &b){
            return a.destination() < b.destination();
        }
        static bool
        sort(const update &a, const update &b){
            return (a.type() != b.type()) ? a.type() < b.type() :
                   (a.destination() != b.destination()) ? a.destination() < b.destination() :
                   (a.source() != b.source()) ? a.source() < b.source() :
                   (a.time() != b.time()) ? a.time() < b.time() :
                   false;
        }
    };

    // Finds an element in a sorted range using binary search
    // http://stackoverflow.com/a/446327/1877086
    template<class Iter, class T, class Compare>
    Iter binary_find(Iter begin, Iter end, T val, Compare comp)
    {
        // Finds the lower bound in at most log(last - first) + 1 comparisons
        Iter i = std::lower_bound(begin, end, val, comp);

        if (i != end && !(comp(val, *i)))
            return i; // found
        else
            return end; // not found
    }


    // Find the first pending update with destination 'dest'
    template<class use_dest>
    update_iterator find_updates(update_iterator begin, update_iterator end, int64_t neighbor)
    {
        update key;
        use_dest::set(key, neighbor);
        update_iterator pos = binary_find(begin, end, key, use_dest::compare);
        return (pos->result == PENDING) ? pos : end;
    }

    class next_update_tracker
    {
    protected:
        update_iterator pos;
        update_iterator end;
    public:
        next_update_tracker(update_iterator begin, update_iterator end)
                : pos(begin), end(end) {}

        update_iterator operator () ()
        {
            while (pos->result != PENDING && pos != end) { ++pos; }
            return pos;
        }
    };

    /*
     * Arguments:
     * result - Result code to set on first update for a neighbor. Result of duplicate updates will always be EDGE_UPDATED.
     * create - Are we inserting into an empty slot, or just updating an existing slot?
     * pos - pointer to first update
     * updates_end - pointer to end of update list
     * G - pointer to STINGER
     * eb - pointer to edge block
     * e - edge index within block
      * operation - EDGE_WEIGHT_SET or EDGE_WEIGHT_INCR
     */

    template<int64_t direction, class use_dest>
    static void
    do_edge_updates(int64_t result, bool create, update_iterator pos, update_iterator updates_end, stinger * G, stinger_eb *eb, size_t e, int64_t operation)
    {
        int64_t neighbor = use_dest::get(*pos);

        for (update_iterator u = pos; u != updates_end && use_dest::get(*u) == neighbor; ++u) {
            if (u == pos) {
                u->result = result;
                update_edge_data_and_direction (G, eb, e, neighbor, u->weight, u->time, direction, create ? EDGE_WEIGHT_SET : operation);
            } else {
                u->result = EDGE_UPDATED;
                update_edge_data_and_direction (G, eb, e, neighbor, u->weight, u->time, direction, operation);
            }
        }
    }

    /*
     * The core algorithm for updating edges in one direction.
     * Similar to stinger_update_directed_edge(), but optimized to perform several updates for the same vertex.
     *
     */
    template<int64_t direction, class use_dest>
    static void
    update_directed_edges_for_vertex(
            stinger *G, int64_t src, int64_t type,
            update_iterator updates_begin,
            update_iterator updates_end,
            int64_t operation)
    {

        MAP_STING(G);
        stinger_eb *ebpool_priv = ebpool->ebpool;
        curs curs = etype_begin (G, src, type);

        assert(direction == STINGER_EDGE_DIRECTION_OUT || direction == STINGER_EDGE_DIRECTION_IN);

        /*
    Possibilities:
    1: Edge already exists and only needs updated.
    2: Edge does not exist, fits in an existing block.
    3: Edge does not exist, needs a new block.
    */

        /* 1: Check if the edge already exists. */
        for (stinger_eb *tmp = ebpool_priv + curs.eb; tmp != ebpool_priv; tmp = ebpool_priv + readff(&tmp->next)) {
            if(type == tmp->etype) {
                size_t k, endk;
                endk = tmp->high;
                // For each edge in the block
                for (k = 0; k < endk; ++k) {
                    // Mask off direction bits to get the raw neighbor of this edge
                    int64_t dest = (tmp->edges[k].neighbor & (~STINGER_EDGE_DIRECTION_MASK));
                    // Find updates for this destination
                    update_iterator u = find_updates<use_dest>(updates_begin, updates_end, dest);
                    // If we already have an in-edge for this destination, we will reuse the edge slot
                    // But the return code should reflect that we added an edge
                    int64_t result = (direction & tmp->edges[k].neighbor) ? EDGE_UPDATED : EDGE_ADDED;
                    do_edge_updates<direction, use_dest>(result, false, u, updates_end, G, tmp, k, operation);
                }
            }
        }

        // Track the next edge that should be inserted
        next_update_tracker next_update(updates_begin, updates_end);

        while (next_update() != updates_end) {
            eb_index_t * block_ptr = curs.loc;
            curs.eb = readff(curs.loc);
            /* 2: The edge isn't already there.  Check for an empty slot. */
            for (stinger_eb *tmp = ebpool_priv + curs.eb; tmp != ebpool_priv; tmp = ebpool_priv + readff(&tmp->next)) {
                if(type == tmp->etype) {
                    size_t k, endk;
                    endk = tmp->high;
                    // This time we go past the high water mark to look at the empty edge slots
                    for (k = 0; k < STINGER_EDGEBLOCKSIZE; ++k) {
                        int64_t myNeighbor = (tmp->edges[k].neighbor & (~STINGER_EDGE_DIRECTION_MASK));

                        // Check for edges that were added by another thread since we last checked
                        if (k < endk) {
                            // Find updates for this destination
                            update_iterator u = find_updates<use_dest>(updates_begin, updates_end, myNeighbor);
                            // If we already have an in-edge for this destination, we will reuse the edge slot
                            // But the return code should reflect that we added an edge
                            int64_t result = (direction & tmp->edges[k].neighbor) ? EDGE_UPDATED : EDGE_ADDED;
                            do_edge_updates<direction, use_dest>(result, false, u, updates_end, G, tmp, k, operation);
                        }

                        if (myNeighbor < 0 || k >= endk) {
                            // Found an empty slot for the edge, lock it and check again to make sure
                            int64_t timefirst = readfe ((uint64_t *)&(tmp->edges[k].timeFirst) );
                            int64_t thisEdge = (tmp->edges[k].neighbor & (~STINGER_EDGE_DIRECTION_MASK));
                            endk = tmp->high;

                            update_iterator u = find_updates<use_dest>(updates_begin, updates_end, thisEdge);
                            if (thisEdge < 0 || k >= endk) {
                                // Slot is empty, add the edge
                                do_edge_updates<direction, use_dest>(EDGE_ADDED, true, next_update(), updates_end, G, tmp, k, operation);
                            } else if (u != updates_end) {
                                // Another thread just added the edge. Do a normal update
                                int64_t result = (direction & tmp->edges[k].neighbor) ? EDGE_UPDATED : EDGE_ADDED;
                                do_edge_updates<direction, use_dest>(result, false, u, updates_end, G, tmp, k, operation);
                                writexf ( (uint64_t *)&(tmp->edges[k].timeFirst), timefirst);
                            } else {
                                // Another thread claimed the slot for a different edge, unlock and keep looking
                                writexf ( (uint64_t *)&(tmp->edges[k].timeFirst), timefirst);
                            }
                        }
                        if (next_update() == updates_end) { return; }
                    }
                }

                block_ptr = &(tmp->next);
            }

            /* 3: Needs a new block to be inserted at end of list. */
            // Try to lock the tail pointer of the last block
            eb_index_t old_eb = readfe (block_ptr);
            if (!old_eb) {
                // Create a new edge block
                eb_index_t newBlock = new_eb (G, type, src);
                if (newBlock == 0) {
                    // Ran out of edge blocks!
                    writeef (block_ptr, (uint64_t)old_eb);
                    while(next_update() != updates_end)
                    {
                        next_update()->result = EDGE_NOT_ADDED;
                    }
                    return;
                } else {
                    // Add our edge to the first slot in the new edge block
                    do_edge_updates<direction, use_dest>(EDGE_ADDED, true, next_update(), updates_end, G, ebpool_priv + newBlock, 0, operation);
                    // Add the block to the list
                    ebpool_priv[newBlock].next = 0;
                    push_ebs (G, 1, &newBlock);
                    writeef (block_ptr, (uint64_t)newBlock);
                }
            } else {
                writeef (block_ptr, (uint64_t)old_eb);
            }
        }
    }

    template<int64_t direction, class use_source, class use_dest>
    static void
    do_batch_update(stinger * G, update_iterator updates_begin, update_iterator updates_end, int64_t operation)
    {
        // Sort by type, src, dst, time ascending
        std::sort(updates_begin, updates_end, use_source::sort);
        // Get a list of unique sources
        std::vector<update> unique_sources;
        std::unique_copy(updates_begin, updates_end, std::back_inserter(unique_sources), use_source::equals);

        OMP("parallel for")
        for (update_iterator key = unique_sources.begin(); key < unique_sources.end(); ++key)
        {
            // HACK also partition on etype
            assert(G->max_netypes == 1);
            int64_t type = 0;
            // Locate the range of updates for this source vertex
            int64_t source = use_source::get(*key);
            std::pair<update_iterator, update_iterator> pair =
                    std::equal_range(updates_begin, updates_end, *key, use_source::compare);
            update_iterator begin = pair.first;
            update_iterator end = pair.second;

            // TODO call single update version if there's only a few edges for a vertex
            // TODO split into smaller batches if there are a lot of edges for a vertex
            // TODO use OMP "collapse" here to parallelize over both loops

            update_directed_edges_for_vertex<direction, use_dest>(G, source, type, begin, end, operation);
        }
    }

    static bool
    source_less_than_destination(const update &x){
        return x.source < x.destination;
    }

    // We use the result field to keep track of which updates have been performed
    // Between IN and OUT iterations, we clear it out to make sure all the updates are performed again
    static void
    clear_results(update_iterator begin, update_iterator end)
    {
        OMP("omp parallel for")
        for (update_iterator u = begin; u < end; ++u) { u->result = PENDING; }
    }

    static void
    remap_results(update_iterator begin, update_iterator end)
    {
        OMP("omp parallel for")
        for (update_iterator u = begin; u < end; ++u) { u->result = u->result - 10; }
    }

    static void
    batch_update_dispatch(stinger * G, update_iterator updates_begin, update_iterator updates_end, int64_t operation, bool directed)
    {
        // Each edge is stored in two places: the out-edge at the source, and the in-edge at the destination
        // First iteration: group by source and update out-edges
        // Second iteration: group by destination and update in-edges
        // But, in order to avoid deadlock with concurrent deletions, we must always lock edges in the same order
        const update_iterator pos = std::partition(updates_begin, updates_end, source_less_than_destination);

        const int64_t OUT = STINGER_EDGE_DIRECTION_OUT;
        const int64_t IN = STINGER_EDGE_DIRECTION_IN;

        // All elements between begin and pos have src < dest. Update the out-edge slot first
        do_batch_update<OUT, source_funcs, dest_funcs>(G, updates_begin, pos, operation);
        clear_results(updates_begin, pos);
        do_batch_update<IN, dest_funcs, source_funcs>(G, updates_begin, pos, operation);

        // All elements between pos and end have src > dest. Update the in-edge slot first
        do_batch_update<IN, dest_funcs, source_funcs>(G, pos, updates_end, operation);
        clear_results(pos, updates_end);
        do_batch_update<OUT, source_funcs, dest_funcs>(G, pos, updates_end, operation);

        // For undirected, do the same updates again in the opposite direction
        if (!directed)
        {
            // TODO does anyone really care what happens when half of an undirected insert has a different return code?
            clear_results(updates_begin, updates_end);

            // All elements between begin and pos have dest < src. Update the in-edge slot first
            do_batch_update<OUT, dest_funcs, source_funcs>(G, updates_begin, pos, operation);
            clear_results(updates_begin, pos);
            do_batch_update<IN, source_funcs, dest_funcs>(G, updates_begin, pos, operation);

            // All elements between pos and end have dest > src. Update the out-edge slot first
            do_batch_update<IN, source_funcs, dest_funcs>(G, pos, updates_end, operation);
            clear_results(pos, updates_end);
            do_batch_update<OUT, dest_funcs, source_funcs>(G, pos, updates_end, operation);
        }

        remap_results(updates_begin, updates_end);
    }
public:
    void
    incr_edges(stinger * G, update_iterator begin, update_iterator end)
    {
        batch_update_dispatch(G, begin, end, EDGE_WEIGHT_INCR, true);
    }
    void
    insert_edges(stinger * G, update_iterator begin, update_iterator end)
    {
        batch_update_dispatch(G, begin, end, EDGE_WEIGHT_SET, true);
    }
    void
    incr_edge_pairs(stinger * G, update_iterator begin, update_iterator end)
    {
        batch_update_dispatch(G, begin, end, EDGE_WEIGHT_INCR, false);
    }
    void
    insert_edge_pairs(stinger * G, update_iterator begin, update_iterator end)
    {
        batch_update_dispatch(G, begin, end, EDGE_WEIGHT_SET, false);
    }
}; // end of class batch insert
#endif //STINGER_BATCH_INSERT_H_
