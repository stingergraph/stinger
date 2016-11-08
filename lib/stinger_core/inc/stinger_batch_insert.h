/*
 * stinger_batch_insert.h
 * Author: Eric Hein <ehein6@gatech.edu>
 * Date: 10/24/2016
 * Purpose:
 *   Provides optimized edge insert/update routines for stinger.
 *   Multiple updates for the same source vertex are dispatched to each thread,
 *   reducing the number of edge-list traversals that need to be done.
 *
 * Accepts a pair of random-access iterators to the range of edge updates to perform.
 * Caller must also provide an adapter template argument: a struct of static functions
 * to access the source, destination, weight, time, and result code of the update.
 * This implementation handles duplicates and sets the return code correctly.
 */

#ifndef STINGER_BATCH_INSERT_H_
#define STINGER_BATCH_INSERT_H_

#include "stinger.h"
#include "stinger_internal.h"
#include "stinger_atomics.h"
#include "x86_full_empty.h"
#define LOG_AT_V
#include "stinger_error.h"
#undef LOG_AT_V

#include <vector>
#include <algorithm>
#include <utility>
#include <cmath>

// *** Public interface (definitions at end of file) ***
template<typename adapter, typename iterator>
void stinger_batch_incr_edges(stinger_t *G, iterator begin, iterator end);
template<typename adapter, typename iterator>
void stinger_batch_insert_edges(stinger_t * G, iterator begin, iterator end);
template<typename adapter, typename iterator>
void stinger_batch_incr_edge_pairs(stinger_t * G, iterator begin, iterator end);
template<typename adapter, typename iterator>
void stinger_batch_insert_edge_pairs(stinger_t * G, iterator begin, iterator end);

// *** Implementation ***
namespace gt { namespace stinger {

/*
 * Rather than add these template arguments to every function in the file, just wrap everything in a template class
 *
 * adapter - provides static methods for accessing fields of an update
 * iterator - iterator for a collection of updates
 */
template<typename adapter, typename iterator>
class BatchInserter
{
protected:
    // Everything in this class is protected and static, these friend functions are the only public interface
    BatchInserter() {}
    friend void stinger_batch_incr_edges<adapter, iterator>(stinger_t *G, iterator begin, iterator end);
    friend void stinger_batch_insert_edges<adapter, iterator>(stinger_t * G, iterator begin, iterator end);
    friend void stinger_batch_incr_edge_pairs<adapter, iterator>(stinger_t * G, iterator begin, iterator end);
    friend void stinger_batch_insert_edge_pairs<adapter, iterator>(stinger_t * G, iterator begin, iterator end);

    // what 'iterator' points to
    typedef typename std::iterator_traits<iterator>::value_type update;

    // Result codes
    // Caller initializes result code to 0, and expects 0 if edge is already present.
    // But we need to use this field to track if the update has been processed yet.
    // We use a fixed offset, then subtract it before returning to match behavior of functions like stinger_incr_edge.
    static const int64_t result_code_offset = 10;
    enum result_codes {
        PENDING =           0,
        EDGE_ADDED =        1 + result_code_offset,
        EDGE_UPDATED =      0 + result_code_offset,
        EDGE_NOT_ADDED =   -1 + result_code_offset
    };

    // Set of functions to sort/filter a collection of updates by source vertex
    struct source_funcs
    {
        static int64_t
        get(const update &x){
            return adapter::get_source(x);
        }
        static void
        set(update &x, int64_t neighbor){
            adapter::set_source(x, neighbor);
        }
        static bool
        equals(const update &a, const update &b){
            return adapter::get_source(a) == adapter::get_source(b);
        }
        static bool
        compare(const update &a, const update &b){
            return adapter::get_source(a) < adapter::get_source(b);
        }
        static bool
        sort(const update &a, const update &b){
            if (adapter::get_type(a) != adapter::get_type(b))
                return adapter::get_type(a) < adapter::get_type(b);
            if (adapter::get_source(a) != adapter::get_source(b))
                return adapter::get_source(a) < adapter::get_source(b);
            if (adapter::get_dest(a) != adapter::get_dest(b))
                return adapter::get_dest(a) < adapter::get_dest(b);
            if (adapter::get_time(a) != adapter::get_time(b))
                return adapter::get_time(a) < adapter::get_time(b);
            return false;
        }
    };

    // Set of functions to sort/filter a collection of updates by destination vertex
    struct dest_funcs
    {
        static int64_t
        get(const update &x){
            return adapter::get_dest(x);
        }
        static void
        set(update &x, int64_t neighbor){
            adapter::set_dest(x, neighbor);
        }
        static bool
        equals(const update &a, const update &b){
            return adapter::get_dest(a) == adapter::get_dest(b);
        }
        static bool
        compare(const update &a, const update &b){
            return adapter::get_dest(a) < adapter::get_dest(b);
        }
        static bool
        sort(const update &a, const update &b){
            if (adapter::get_type(a) != adapter::get_type(b))
                return adapter::get_type(a) < adapter::get_type(b);
            if (adapter::get_dest(a) != adapter::get_dest(b))
                return adapter::get_dest(a) < adapter::get_dest(b);
            if (adapter::get_source(a) != adapter::get_source(b))
                return adapter::get_source(a) < adapter::get_source(b);
            if (adapter::get_time(a) != adapter::get_time(b))
                return adapter::get_time(a) < adapter::get_time(b);
            return false;
        }
    };

    // Finds an element in a sorted range using binary search
    // http://stackoverflow.com/a/446327/1877086
    template<class Iter, class T, class Compare>
    static Iter
    binary_find(Iter begin, Iter end, T val, Compare comp)
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
    static iterator
    find_updates(iterator begin, iterator end, int64_t neighbor)
    {
        // Create a dummy update object with the neighbor we are looking for
        update key;
        use_dest::set(key, neighbor);
        // Find the updates for this neighbor
        iterator pos = binary_find(begin, end, key, use_dest::compare);
        // If the first update is not pending, we have already done all the updates for this neighbor
        if (pos != end && adapter::get_result(*pos) == PENDING) return pos;
        else return end;
    }

    // Keeps track of the next pending update to insert into the graph
    class next_update_tracker
    {
    protected:
        iterator pos;
        iterator end;
    public:
        next_update_tracker(iterator begin, iterator end)
        : pos(begin), end(end) {}

        iterator
        operator () ()
        {
            while (pos != end && adapter::get_result(*pos) != PENDING) { ++pos; }
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
    do_edge_updates(
        int64_t result, bool create, iterator pos, iterator updates_end,
        stinger_t * G, stinger_eb *eb, size_t e, int64_t operation)
    {
        // 'pos' points to the first update for this edge slot; there may be more than one, or none if it equals 'updates_end'
        // Keep incrementing the iterator until we reach an update with a different neighbor
        for (iterator u = pos; u != updates_end && use_dest::get(*u) == use_dest::get(*pos); ++u) {
            int64_t neighbor = use_dest::get(*pos);
            int64_t weight = adapter::get_weight(*u);
            int64_t time = adapter::get_time(*u);
            if (u == pos) {
                adapter::set_result(*u, result);
                update_edge_data_and_direction (G, eb, e, neighbor, weight, time, direction, create ? EDGE_WEIGHT_SET : operation);
            } else {
                adapter::set_result(*u, EDGE_UPDATED);
                update_edge_data_and_direction (G, eb, e, neighbor, weight, time, direction, operation);
            }
        }
    }

    /*
     * The core algorithm for updating edges in one direction.
     * Similar to stinger_update_directed_edge(), but optimized to perform several updates for the same vertex.
     * direction - are we updating out-edges or in-edges?
     * use_dest - Either source_funcs or dest_funcs (allows us to swap source/destination easily)
     */
    template<int64_t direction, class use_dest>
    static void
    update_directed_edges_for_vertex(
            stinger_t *G, int64_t src, int64_t type,
            iterator updates_begin,
            iterator updates_end,
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

        // Track the next edge that should be inserted
        next_update_tracker next_update(updates_begin, updates_end);

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
                    iterator u = find_updates<use_dest>(next_update(), updates_end, dest);
                    // If we already have an in-edge for this destination, we will reuse the edge slot
                    // But the return code should reflect that we added an edge
                    int64_t result = (direction & tmp->edges[k].neighbor) ? EDGE_UPDATED : EDGE_ADDED;
                    do_edge_updates<direction, use_dest>(result, false, u, updates_end,
                        G, tmp, k, operation);
                }
            }
        }

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
                            iterator u = find_updates<use_dest>(next_update(), updates_end, myNeighbor);
                            // If we already have an in-edge for this destination, we will reuse the edge slot
                            // But the return code should reflect that we added an edge
                            int64_t result = (direction & tmp->edges[k].neighbor) ? EDGE_UPDATED : EDGE_ADDED;
                            do_edge_updates<direction, use_dest>(result, false, u, updates_end,
                                G, tmp, k, operation);
                        }

                        if (myNeighbor < 0 || k >= endk) {
                            // Found an empty slot for the edge, lock it and check again to make sure
                            int64_t timefirst = readfe ((uint64_t *)&(tmp->edges[k].timeFirst) );
                            int64_t thisEdge = (tmp->edges[k].neighbor & (~STINGER_EDGE_DIRECTION_MASK));
                            endk = tmp->high;

                            iterator u = find_updates<use_dest>(next_update(), updates_end, thisEdge);
                            if (thisEdge < 0 || k >= endk) {
                                // Slot is empty, add the edge
                                do_edge_updates<direction, use_dest>(EDGE_ADDED, true, next_update(), updates_end,
                                    G, tmp, k, operation);
                            } else if (u != updates_end) {
                                // Another thread just added the edge. Do a normal update
                                int64_t result = (direction & tmp->edges[k].neighbor) ? EDGE_UPDATED : EDGE_ADDED;
                                do_edge_updates<direction, use_dest>(result, false, u, updates_end,
                                    G, tmp, k, operation);
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
                        adapter::set_result(*next_update(), EDGE_NOT_ADDED);
                    }
                    return;
                } else {
                    // Add our edge to the first slot in the new edge block
                    do_edge_updates<direction, use_dest>(EDGE_ADDED, true, next_update(), updates_end,
                        G, ebpool_priv + newBlock, 0, operation);
                    // Add the block to the list
                    ebpool_priv[newBlock].next = 0;
                    push_ebs (G, 1, &newBlock);
                    // Unlock the tail pointer
                    writeef (block_ptr, (uint64_t)newBlock);
                }
            } else {
                // Another thread already added a block, unlock and try again
                writeef (block_ptr, (uint64_t)old_eb);
            }
        }
    }

    template <class use_source>
    static bool same_source_and_type(const iterator &a, const iterator &b)
    {
        return adapter::get_type(*a) == adapter::get_type(*b)
            && use_source::equals(*a, *b);
    }

    /*
     * Splits a range of updates into chunks that all update the same source,
     * then calls update_directed_edges_for_vertex() in parallel on each range
     *
     * Template arguments:
     * direction - are we updating out-edges or in-edges?
     * use_source - set of functions to use for working with the source vertex (source_funcs or dest_funcs)
     * use_dest - set of functions to use for working with the destination vertex (source_funcs or dest_funcs)
     */
    template<int64_t direction, class use_source, class use_dest>
    static void
    do_batch_update(stinger_t * G, iterator updates_begin, iterator updates_end, int64_t operation)
    {
        typedef typename std::vector<iterator>::iterator iterator_ptr;
        typedef typename std::pair<iterator, iterator> range;
        typedef typename std::vector<range>::iterator range_iterator;

        // Sort by type, src, dst, time ascending
        LOG_V("Sorting...");
        std::sort(updates_begin, updates_end, use_source::sort);

        // Get a list of pointers to each element
        LOG_V("Finding unique sources...");
        int64_t num_updates = std::distance(updates_begin, updates_end);
        std::vector<iterator> pointers(num_updates);
        OMP("omp parallel for")
        for (int64_t i = 0; i < num_updates; ++i) { pointers[i] = updates_begin + i; }

        // Reduce down to a list of pointers to the beginning of each range of unique source ID's
        // NOTE: using std::back_inserter here would prevent std::unique_copy from running in parallel
        //       So instead, allocate room for all the updates and shrink the vector afterwards
        std::vector<iterator> unique_sources(num_updates);
        iterator_ptr last_unique_source = std::unique_copy(
            pointers.begin(), pointers.end(), unique_sources.begin(), same_source_and_type<use_source>);
        unique_sources.erase(last_unique_source, unique_sources.end());
        // Now each consecutive pair of elements represents a range of updates for the same type and source ID
        unique_sources.push_back(updates_end);

        // Split up long ranges of updates for the same source vertex
        LOG_V("Splitting ranges...");
        std::vector<range> update_ranges;
        OMP("omp parallel for")
        for (iterator_ptr ptr = unique_sources.begin(); ptr < unique_sources.end()-1; ++ptr)
        {
            iterator begin = *ptr;
            iterator end = *(ptr + 1);

            // Calculate number of updates for each range
            size_t num_updates = std::distance(begin, end);
            size_t num_ranges = omp_get_num_threads();
            size_t updates_per_range = std::floor((double)num_updates / num_ranges);

            std::vector<range> local_ranges; local_ranges.reserve(num_ranges);
            if (updates_per_range < omp_get_num_threads())
            {
                // If there aren't many updates, just give them all to one thread
                local_ranges.push_back(make_pair(begin, end));
            } else {
                // Split the updates evenly amoung threads
                for (size_t i = 0; i < num_ranges-1; ++i)
                {
                    local_ranges.push_back(make_pair(begin, begin + updates_per_range));
                    begin += updates_per_range;
                }
                // Last range may be a different size if work doesn't divide evenly
                local_ranges.push_back(make_pair(begin, end));
            }

            // Combine all ranges into shared list
            OMP("omp critical")
            update_ranges.insert(update_ranges.end(), local_ranges.begin(), local_ranges.end());
        }


        LOG_V_A("Entering parallel update loop: %ld updates for %ld vertices.",
            std::distance(updates_begin, updates_end), unique_sources.size()-1);
        OMP("omp parallel for schedule(dynamic)")
        for (range_iterator range = update_ranges.begin(); range < update_ranges.end(); ++range)
        {
            // Get this thread's range of updates
            iterator begin = range->first;
            iterator end = range->second;
            // Each range guaranteed to have same edge type and source vertex
            int64_t type = adapter::get_type(*begin);
            int64_t source = use_source::get(*begin);

            LOG_D_A("Thread %d processing %ld updates (%ld -> *)", omp_get_thread_num(), std::distance(begin, end), source);
            update_directed_edges_for_vertex<direction, use_dest>(G, source, type, begin, end, operation);
        }
    }

    static bool
    source_less_than_destination(const update &x){
        return adapter::get_source(x) < adapter::get_dest(x);
    }

    // We use the result field to keep track of which updates have been performed
    // Between IN and OUT iterations, we clear it out to make sure all the updates are performed again
    static void
    clear_results(iterator begin, iterator end)
    {
        OMP("omp parallel for")
        for (iterator u = begin; u < end; ++u) {
            int64_t result = adapter::get_result(*u);
            switch (result){
                case EDGE_ADDED:
                case EDGE_UPDATED:
                case EDGE_NOT_ADDED:
                    // Reset return code so we process this update next iteration
                    adapter::set_result(*u, PENDING);
                    break;
                case EDGE_ADDED-result_code_offset:
                case EDGE_NOT_ADDED-result_code_offset:
                    // Caller must have set the return code, leave it be
                    break;
                case PENDING:
                default:
                    // We didn't finish all the updates, or invalid codes were set
                    assert(0);

            }
        }
    }

    static void
    remap_results(iterator begin, iterator end)
    {
        OMP("omp parallel for")
        for (iterator u = begin; u < end; ++u) {
            int64_t result = adapter::get_result(*u);
            switch (result){
                case EDGE_ADDED:
                case EDGE_UPDATED:
                case EDGE_NOT_ADDED:
                    // Subtract 10 to get back to return code caller expects
                    adapter::set_result(*u, result - result_code_offset);
                    break;
                case EDGE_ADDED-result_code_offset:
                case EDGE_NOT_ADDED-result_code_offset:
                    // Caller must have set the return code, leave it be
                    break;
                case PENDING:
                default:
                    // We didn't finish all the updates, or invalid codes were set
                    assert(0);

            }
        }
    }

    static void
    batch_update_dispatch(stinger_t * G, iterator updates_begin, iterator updates_end, int64_t operation, bool directed)
    {
        // Each edge is stored in two places: the out-edge at the source, and the in-edge at the destination
        // First iteration: group by source and update out-edges
        // Second iteration: group by destination and update in-edges
        // But, in order to avoid deadlock with concurrent deletions, we must always lock edges in the same order
        LOG_V("Partitioning batch to obey ordering constraints...");
        const iterator pos = std::partition(updates_begin, updates_end, source_less_than_destination);

        const int64_t OUT = STINGER_EDGE_DIRECTION_OUT;
        const int64_t IN = STINGER_EDGE_DIRECTION_IN;

        // All elements between begin and pos have src < dest. Update the out-edge slot first
        LOG_V("Beginning OUT updates for first half of batch...");
        do_batch_update<OUT, source_funcs, dest_funcs>(G, updates_begin, pos, operation);
        clear_results(updates_begin, pos);
        LOG_V("Beginning IN updates for first half of batch...");
        do_batch_update<IN, dest_funcs, source_funcs>(G, updates_begin, pos, operation);

        // All elements between pos and end have src > dest. Update the in-edge slot first
        LOG_V("Beginning IN updates for second half of batch...");
        do_batch_update<IN, dest_funcs, source_funcs>(G, pos, updates_end, operation);
        clear_results(pos, updates_end);
        LOG_V("Beginning OUT updates for second half of batch...");
        do_batch_update<OUT, source_funcs, dest_funcs>(G, pos, updates_end, operation);

        // For undirected, do the same updates again in the opposite direction
        if (!directed)
        {
            // NOTE We are discarding the return code from the first update, assuming that it will be the same in both directions
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
}; // end of class batch insert

}} // end namespace gt::stinger

template<typename adapter, typename iterator>
void
stinger_batch_incr_edges(stinger_t *G, iterator begin, iterator end)
{
    gt::stinger::BatchInserter<adapter, iterator>::batch_update_dispatch(G, begin, end, EDGE_WEIGHT_INCR, true);
}
template<typename adapter, typename iterator>
void
stinger_batch_insert_edges(stinger_t * G, iterator begin, iterator end)
{
    gt::stinger::BatchInserter<adapter, iterator>::batch_update_dispatch(G, begin, end, EDGE_WEIGHT_SET, true);
}
template<typename adapter, typename iterator>
void
stinger_batch_incr_edge_pairs(stinger_t * G, iterator begin, iterator end)
{
    gt::stinger::BatchInserter<adapter, iterator>::batch_update_dispatch(G, begin, end, EDGE_WEIGHT_INCR, false);
}
template<typename adapter, typename iterator>
void
stinger_batch_insert_edge_pairs(stinger_t * G, iterator begin, iterator end)
{
    gt::stinger::BatchInserter<adapter, iterator>::batch_update_dispatch(G, begin, end, EDGE_WEIGHT_SET, false);
}

#endif //STINGER_BATCH_INSERT_H_
