#include "stinger_batch_insert.h"

#include "stinger_internal.h"
#include "stinger_atomics.h"

#include "x86_full_empty.h"

#include <vector>
#include <algorithm>
#include <stinger_alg.h>
#include <stinger_net/stinger_alg.h>

typedef stinger_edge_update update;
typedef std::vector<update>::iterator update_iterator;

// FIXME subtract 10 from all return codes before returning
const int64_t PENDING = 0;
const int64_t EDGE_ADDED = 11;
const int64_t EDGE_UPDATED = 10;
const int64_t EDGE_NOT_ADDED = 9;

// Used to sort a vector of stinger_edge_updates below
typedef bool (*update_comparator)(const update &, const update &);

static bool order_by_source(const update &a, const update &b){
    return (a.type != b.type) ? a.type < b.type :
           (a.source != b.source) ? a.source < b.source :
           (a.destination != b.destination) ? a.destination < b.destination :
           (a.time != b.time) ? a.time < b.time :
           false;
}

static bool order_by_dest(const update &a, const update &b){
    return (a.type != b.type) ? a.type < b.type :
           (a.destination != b.destination) ? a.destination < b.destination :
           (a.source != b.source) ? a.source < b.source :
           (a.time != b.time) ? a.time < b.time :
           false;
}

static bool
compare_sources(const update &a, const update &b){
    return a.source < b.source;
}

static bool
compare_destinations(const update &a, const update &b){
    return a.destination < b.destination;
}

static bool
sources_equal(const update &a, const update &b){
    return a.source == b.source;
}

static bool
destinations_equal(const update &a, const update &b){
    return a.destination == b.destination;
}

// Used to get a list of unique source/dest vertices in a batch of updates below
typedef int64_t (*update_attr_getter)(const update &);

static int64_t
get_source(const update &x){
    return x.source;
}

static int64_t
get_destination(const update &x){
    return x.destination;
}

// Used to get a list of unique source/dest vertices in a batch of updates below
typedef void (*update_attr_setter)(update &, int64_t);

static void
set_source(update &x, int64_t neighbor){
    x.source = neighbor;
}

static void
set_destination(update &x, int64_t neighbor){
    x.destination = neighbor;
}

void
clear_results(update_iterator begin, update_iterator end)
{
    OMP("omp parallel for")
    for (update_iterator u = begin; u < end; ++u) { u->result = PENDING; }
}

void
remap_results(update_iterator begin, update_iterator end)
{
    OMP("omp parallel for")
    for (update_iterator u = begin; u < end; ++u) { u->result = u->result - 10; }
}

struct function_group
{
    update_attr_getter get;
    update_attr_setter set;
    update_comparator sort;
    update_comparator compare;
    update_comparator equals;
};

function_group source_functions = {
    get_source,
    set_source,
    order_by_source,
    compare_sources,
    sources_equal
};
function_group destination_functions = {
    get_destination,
    set_destination,
    order_by_dest,
    compare_destinations,
    destinations_equal
};

void
stinger_batch_update(stinger * G, std::vector<update> &updates, int64_t operation)
{
    for (int i = 0; i < 2; ++i) {
        // First iteration: group by source and update out-edges
        // Second iteration: group by destination and update in-edges
        int64_t direction = (i == 0) ? STINGER_EDGE_DIRECTION_OUT : STINGER_EDGE_DIRECTION_IN;
        function_group &g = (i == 0) ? source_functions : destination_functions;

        // Set all result codes to 0 - only the last iteration return codes survive
        clear_results(updates.begin(), updates.end());

        // Sort by type, src, dst, time ascending
        std::sort(updates.begin(), updates.end(), g.sort);
        // Get a list of unique sources
        std::vector<update> unique_sources;
        std::unique_copy(updates.begin(), updates.end(), std::back_inserter(unique_sources), g.equals);

        OMP("parallel for")
        for (update_iterator key = unique_sources.begin(); key < unique_sources.end(); ++key)
        {
            // HACK also partition on etype
            assert(G->max_netypes == 1);
            int64_t type = 0;
            // Locate the range of updates for this source vertex
            int64_t source = g.get(*key);
            std::pair<update_iterator, update_iterator> pair =
            std::equal_range(updates.begin(), updates.end(), *key, g.compare);
            update_iterator begin = pair.first;
            update_iterator end = pair.second;

            // TODO call single update version if there's only a few edges for a vertex
            // TODO split into smaller batches if there are a lot of edges for a vertex
            stinger_update_directed_edges_for_vertex(G, source, type, begin, end, direction, operation);
        }
    }
    remap_results(updates.begin(), updates.end());
}

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
update_iterator find_updates(update_iterator begin, update_iterator end, int64_t neighbor, int64_t direction)
{
    function_group &g = direction == STINGER_EDGE_DIRECTION_OUT ? destination_functions : source_functions;
    update key;
    g.set(key, neighbor);
    update_iterator pos = binary_find(begin, end, key, g.compare);
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
 * result - Result code to set on first update. Result of duplicate updates will always be EDGE_UPDATED
 * create - Are we inserting into an empty slot, or just updating an existing slot
 * pos - pointer to first update
 * updates_end - pointer to end of update list
 * G - pointer to STINGER
 * eb - pointer to edge block
 * e - edge index within block
 * direction - edge direction
 * operation - EDGE_WEIGHT_SET or EDGE_WEIGHT_INCR
 */

static void do_edge_updates(int64_t result, bool create, update_iterator pos, update_iterator updates_end, stinger * G, stinger_eb *eb, size_t e, int64_t direction, int64_t operation)
{
    function_group &g = direction == STINGER_EDGE_DIRECTION_OUT ? destination_functions : source_functions;
    int64_t neighbor = g.get(*pos);

    for (update_iterator u = pos; u != updates_end && g.get(*u) == neighbor; ++u) {
        if (u == pos) {
            u->result = result;
            update_edge_data_and_direction (G, eb, e, g.get(*u), u->weight, u->time, direction, create ? EDGE_WEIGHT_SET : operation);
        } else {
            u->result = EDGE_UPDATED;
            update_edge_data_and_direction (G, eb, e, g.get(*u), u->weight, u->time, direction, operation);
        }
    }
}


void
stinger_update_directed_edges_for_vertex(
        stinger *G, int64_t src, int64_t type,
        update_iterator updates_begin,
        update_iterator updates_end,
        int64_t direction, int64_t operation)
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
                update_iterator u = find_updates(updates_begin, updates_end, dest, direction);
                // If we already have an in-edge for this destination, we will reuse the edge slot
                // But the return code should reflect that we added an edge
                int64_t result = (direction & tmp->edges[k].neighbor) ? EDGE_UPDATED : EDGE_ADDED;
                do_edge_updates(result, false, u, updates_end, G, tmp, k, direction, operation);
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
                        update_iterator u = find_updates(updates_begin, updates_end, myNeighbor, direction);
                        // If we already have an in-edge for this destination, we will reuse the edge slot
                        // But the return code should reflect that we added an edge
                        int64_t result = (direction & tmp->edges[k].neighbor) ? EDGE_UPDATED : EDGE_ADDED;
                        do_edge_updates(result, false, u, updates_end, G, tmp, k, direction, operation);
                    }

                    if (myNeighbor < 0 || k >= endk) {
                        // Found an empty slot for the edge, lock it and check again to make sure
                        int64_t timefirst = readfe ((uint64_t *)&(tmp->edges[k].timeFirst) );
                        int64_t thisEdge = (tmp->edges[k].neighbor & (~STINGER_EDGE_DIRECTION_MASK));
                        endk = tmp->high;

                        update_iterator u = find_updates(updates_begin, updates_end, thisEdge, direction);
                        if (thisEdge < 0 || k >= endk) {
                            // Slot is empty, add the edge
                            do_edge_updates(EDGE_ADDED, true, next_update(), updates_end, G, tmp, k, direction, operation);
                        } else if (u != updates_end) {
                            // Another thread just added the edge. Do a normal update
                            int64_t result = (direction & tmp->edges[k].neighbor) ? EDGE_UPDATED : EDGE_ADDED;
                            do_edge_updates(result, false, u, updates_end, G, tmp, k, direction, operation);
                            writexf ( (uint64_t *)&(tmp->edges[k].timeFirst), timefirst);
                        } else {
                            // Another thread claimed the slot for a different edge, unlock and keep looking
                            writexf ( (uint64_t *)&(tmp->edges[k].timeFirst), timefirst);
                        }
                    }
                }
            }

            block_ptr = &(tmp->next);
        }

        if (next_update() == updates_end) { return; }

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
                do_edge_updates(EDGE_ADDED, true, next_update(), updates_end, G, ebpool_priv + newBlock, 0, direction, operation);
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
