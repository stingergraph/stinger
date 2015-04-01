#include <stdlib.h>
#include <iostream>
#include <vector>
#include <map>
#include <utility>      // std::pair, std:: make_pair
#include <queue>
# include <set>
#include "../../../../../../build/include/stinger_core/stinger.h"
#include "../../../../../../build/include/stinger_core/stinger_defs.h"
#include "../../../../../../build/include/stinger_core/stinger_traversal.h"


using namespace std;

class community_state
{
    private:
        vector<vector<int64_t> > parents; // Chronological list of parental history for each node
        set<int64_t> active; // Set of presently active communities
        vector<int64_t> size; // Present number of nodes inside each community
        vector<vector<int64_t> > children; // Chronological list of children merged into each community (recursive)
        //map<pair<int64_t, int64_t>, int64_t> adjacency; // Upper triangular sparse matrix of the present number of edges between each community pair (including self edges)
        vector<map<int64_t, int64_t> > nbrs; // Present set of neighbors for each community, based on the adjacency
        /*
        int64_t get_adjacency(int64_t u, int64_t v)
        {
            
            if(u < v) return adjacency.at(make_pair(u, v));
            else return adjacency.at(make_pair(v, u));
        }
        */
        
        void add_adjacency(int64_t u, int64_t v, int64_t w)
        {
            if(nbrs[u].find(v) == nbrs[u].end())
            {
                nbrs[u][v] = w;
                nbrs[v][u] = w;
            }
            else
            {
                if(nbrs[u][v] + w == 0)
                {
                    nbrs[u].erase(v);
                    nbrs[v].erase(u);
                }
                else
                {
                    nbrs[u][v] += w;
                    nbrs[v][u] += w;
                }
            }
        }
        
        void change_parent(int64_t v, int64_t p) // Recursively change parent of vertex v and all its children to p
        {
            children[p].push_back(v);
            //children[p].erase(-1); // Mark children[p] set as not empty (by removing -1 from its set)
            parents[v].push_back(p);
            queue<int64_t> child_queue;
            if(children[v].size() > 0) child_queue.push(v);
            while(child_queue.size() > 0)
            {
                int64_t child = child_queue.front();
                child_queue.pop();
                for(int64_t i=0;i<children[child].size();i++)
                {
                    int64_t newchild = children[child][i];
                    parents[newchild].push_back(p);
                    if(children[newchild].size() > 0) child_queue.push(newchild);
                }
            }
        }
    
    public:
        community_state(struct stinger * S) // Initializer
        {
            int64_t nv = stinger_max_active_vertex(S);
            
            for(int64_t v=0;v<nv;++v)
            {
                vector<int64_t> empty_vector;
                
                parents.push_back(empty_vector);
                children.push_back(empty_vector);
                
                parents[v].push_back(v);
                active.insert(v);
                size.push_back(1);
                //children[v].insert(-1); // -1 represents no children (empty set)
                
                STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, v)
                {
                    int64_t dest = STINGER_EDGE_DEST;
                    int64_t w = STINGER_EDGE_WEIGHT;
                    add_adjacency(v, dest, w);
                } STINGER_FORALL_EDGES_OF_VTX_END();
                
                set<int64_t> empty_set;
                nbrs.push_back(empty_set);
            }
        }
        
        int64_t find(int64_t v) // Find parent of vertex v
        {
            return parents[v].back();
        }
        
        void merge(int64_t i, int64_t j) // Merge communities i and j
        {
            try
            {
                int64_t parent_i = find(i);
                int64_t parent_j = find(j);
                if(parent_i != i)
                {
                    throw(i);
                }
                if(parent_j != j)
                {
                    throw(j);
                }
            }
            catch(int64_t x)
            {
                cout << "ERROR merge(" << i << ", " << j << "): " << x << " is not a community node!" << endl;
                throw;
            }
            if(i != j)
            {
                int64_t parent, child;
                if(size[i] > size[j])
                {
                    parent = i;
                    child = j;
                }
                else
                {
                    parent = j;
                    child = i;
                }
                change_parent(child, parent);
                active.erase(child);
                size[parent] += size[child];
                for(set<int64_t>::iterator nbr_it=nbrs[child].begin(); nbr_it!=nbrs[child].end(); ++nbr_it)
                {
                    int64_t nbr = nbr_it->first;
                    int64_t wt = nbr_it->second;
                    add_adjacency(parent, nbr, wt);
                }
            }
        }
        
        void split(int64_t i)
        {
            if(children[i].size()>0)
            {
                int64_t child = children[i].back();
                children[i].pop_back();
                change_parent(child, child);
                active.insert(child);
                size[i] -= size[child];
                for(set<int64_t>::iterator nbr_it=nbrs[child].begin(); nbr_it!=nbrs[child].end(); ++nbr_it)
                {
                    int64_t nbr = nbr_it->first;
                    int64_t wt = nbr_it->second;
                    add_adjacency(parent, nbr, -wt);
                }
            }
        }
};


                    
                    
