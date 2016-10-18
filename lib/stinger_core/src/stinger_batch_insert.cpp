#include "stinger_batch_insert.h"

#include "stinger_internal.h"
#include "stinger_atomics.h"

#include "x86_full_empty.h"

#include <vector>
#include <algorithm>

typedef std::vector<stinger_edge_update>::iterator update_iterator;

// Used to sort a vector of stinger_edge_updates below
static bool stinger_edge_update_compare(const stinger_edge_update &a, const stinger_edge_update &b){
    return (a.type != b.type) ? a.type < b.type :
           (a.source != b.source) ? a.source < b.source :
           (a.destination != b.destination) ? a.destination < b.destination :
           (a.time != b.time) ? a.time < b.time :
           false;
}

// Used to get a list of unique source vertices in a batch of updates below
static int64_t
stinger_edge_update_get_source(const stinger_edge_update &x){
    return x.source;
}

static bool
stinger_edge_update_compare_sources(const stinger_edge_update &a, const stinger_edge_update &b){
    return a.source < b.source;
}

static bool
stinger_edge_update_compare_destinations(const stinger_edge_update &a, const stinger_edge_update &b){
    return a.destination < b.destination;
}

void
stinger_batch_update(stinger * G, std::vector<stinger_edge_update> &updates, int64_t direction, int64_t operation)
{
    // Sort by type, src, dst, time ascending
    std::sort(updates.begin(), updates.end(), stinger_edge_update_compare);

    // Get a list of unique sources
    std::vector<int64_t> unique_sources;
    std::transform(updates.begin(), updates.end(), std::back_inserter(unique_sources),
        stinger_edge_update_get_source);
    unique_sources.erase(
        std::unique(unique_sources.begin(), unique_sources.end()),
        unique_sources.end()
    );

    OMP("parallel for")
    for (std::vector<int64_t>::iterator f = unique_sources.begin(); f != unique_sources.end(); ++f)
    {
        // HACK also partition on etype
        assert(G->max_netypes == 1);
        int64_t type = 0;
        // Locate the range of updates for this source vertex
        int64_t source = *f;
        stinger_edge_update key; key.source = source;
        std::pair<update_iterator, update_iterator> pair =
        std::equal_range(updates.begin(), updates.end(), key, stinger_edge_update_compare_sources);
        update_iterator begin = pair.first;
        update_iterator end = pair.second;

        // TODO call single update version if there's only a few edges for a vertex
        // TODO split into smaller batches if there are a lot of edges for a vertex
        stinger_update_directed_edges_for_vertex(G, source, type, begin, end, direction, operation);
    }
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

// Find the first update with destination 'dest'
update_iterator find_updates(update_iterator begin, update_iterator end, int64_t dest)
{
    stinger_edge_update key;
    key.destination = dest;
    return binary_find(begin, end, key, stinger_edge_update_compare_destinations);
}

// Skip to the next update that hasn't happened yet
update_iterator find_next_pending_update(update_iterator pos, update_iterator end)
{
    while (pos->result != 0 && pos != end) { ++pos; }
    return pos;
}

void
stinger_update_directed_edges_for_vertex(
        stinger *G, int64_t src, int64_t type,
        std::vector<stinger_edge_update>::iterator updates_begin,
        std::vector<stinger_edge_update>::iterator updates_end,
        int64_t direction, int64_t operation)
{

    MAP_STING(G);

    curs curs;
    stinger_eb *tmp;
    stinger_eb *ebpool_priv = ebpool->ebpool;

    // FIXME subtract 10 from all return codes before returning
    const int64_t PENDING = 0;
    const int64_t EDGE_ADDED = 11;
    const int64_t EDGE_UPDATED = 10;
    const int64_t EDGE_NOT_ADDED = 9;

    curs = etype_begin (G, src, type);

    /*
Possibilities:
1: Edge already exists and only needs updated.
2: Edge does not exist, fits in an existing block.
3: Edge does not exist, needs a new block.
*/

    /* 1: Check if the edge already exists. */
    for (tmp = ebpool_priv + curs.eb; tmp != ebpool_priv; tmp = ebpool_priv + readff(&tmp->next)) {
        if(type == tmp->etype) {
            size_t k, endk;
            endk = tmp->high;
            // For each edge in the block
            for (k = 0; k < endk; ++k) {
                int64_t dest = (tmp->edges[k].neighbor & (~STINGER_EDGE_DIRECTION_MASK));
                update_iterator u = find_updates(updates_begin, updates_end, dest);
                for (;u != updates_end && u->destination == dest; u = find_next_pending_update(u, updates_end)){
                    if (direction & tmp->edges[k].neighbor) {
                        u->result = EDGE_UPDATED;
                    } else {
                        u->result = EDGE_ADDED;
                    }
                    update_edge_data_and_direction (G, tmp, k, u->destination, u->weight, u->time, direction, operation);
                }
            }
        }
    }

    // TODO add a stable_partition here to skip edges that were already updated

    // Track the next edge that should be inserted
    update_iterator next_pending_update = find_next_pending_update(updates_begin, updates_end);

    while (next_pending_update != updates_end) {
        eb_index_t * block_ptr = curs.loc;
        curs.eb = readff((uint64_t *)curs.loc);
        /* 2: The edge isn't already there.  Check for an empty slot. */
        for (tmp = ebpool_priv + curs.eb; tmp != ebpool_priv; tmp = ebpool_priv + readff(&tmp->next)) {
            if(type == tmp->etype) {
                size_t k, endk;
                endk = tmp->high;
                // This time we go past the high water mark to look at the empty edge slots
                for (k = 0; k < STINGER_EDGEBLOCKSIZE; ++k) {
                    int64_t myNeighbor = (tmp->edges[k].neighbor & (~STINGER_EDGE_DIRECTION_MASK));

                    if (k < endk) {
                        // Check for edges that were added by another thread since we last checked
                        update_iterator u = find_updates(next_pending_update, updates_end, myNeighbor);
                        for (;u != updates_end && u->destination == myNeighbor; u = find_next_pending_update(u, updates_end)){
                            // Edge was recently added, and we missed it above
                            if (direction & tmp->edges[k].neighbor) {
                                u->result = EDGE_UPDATED;
                            } else {
                                u->result = EDGE_ADDED;
                            }
                            update_edge_data_and_direction (G, tmp, k, u->destination, u->weight, u->time, direction, operation);
                        }
                        next_pending_update = find_next_pending_update(next_pending_update, updates_end);
                    }

                    if (myNeighbor < 0 || k >= endk) {
                        // Found an empty slot for the edge, lock it and check again to make sure
                        int64_t timefirst = readfe ((uint64_t *)&(tmp->edges[k].timeFirst) );
                        int64_t thisEdge = (tmp->edges[k].neighbor & (~STINGER_EDGE_DIRECTION_MASK));
                        endk = tmp->high;

                        update_iterator u = find_updates(updates_begin, updates_end, thisEdge);
                        if (thisEdge < 0 || k >= endk) {
                            // Slot is empty, add the edge
                            next_pending_update = find_next_pending_update(next_pending_update, updates_end);
                            stinger_edge_update & u = *next_pending_update;
                            update_edge_data_and_direction (G, tmp, k, u.destination, u.weight, u.time, direction, EDGE_WEIGHT_SET);
                            u.result = EDGE_ADDED;
                        } else if (u != updates_end) {
                            // Another thread just added the edge. Do a normal update
                            for (;u != updates_end && u->destination == myNeighbor; u = find_next_pending_update(u, updates_end)){
                                update_edge_data_and_direction (G, tmp, k, u->destination, u->weight, u->time, direction, operation);
                                u->result = EDGE_UPDATED;
                            }
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

        next_pending_update = find_next_pending_update(updates_begin, updates_begin);
        if (next_pending_update == updates_end) { return; }

        /* 3: Needs a new block to be inserted at end of list. */
        eb_index_t old_eb = readfe (block_ptr);
        if (!old_eb) {
            eb_index_t newBlock = new_eb (G, type, src);
            if (newBlock == 0) {
                // Ran out of edge blocks!
                writeef (block_ptr, (uint64_t)old_eb);
                for (; next_pending_update != updates_end; next_pending_update = find_next_pending_update(next_pending_update, updates_begin))
                {
                    next_pending_update->result = EDGE_NOT_ADDED;
                }
                return;
            } else {
                for (update_iterator u = next_pending_update; u != updates_end && u->destination == next_pending_update->destination; ++u){
                    u->result = u == next_pending_update ? EDGE_ADDED : EDGE_UPDATED;
                    update_edge_data_and_direction (G, ebpool_priv + newBlock, 0, u->destination, u->weight, u->time, direction,
                        u->result == EDGE_ADDED ? EDGE_WEIGHT_SET : operation); // Create it the first time, after that obey 'operation' param
                }
                // Add the block to the list
                ebpool_priv[newBlock].next = 0;
                push_ebs (G, 1, &newBlock);
                writeef (block_ptr, (uint64_t)newBlock);
                next_pending_update = find_next_pending_update(next_pending_update, updates_begin);
            }
        } else {
            writeef (block_ptr, (uint64_t)old_eb);
        }
    }

}
