#include "stinger_traversal_test.h"

#define restrict

TEST_F(StingerTraversalTest, STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN) {
  for (int64_t v=0; v < 200; v++) {
    STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN(S,v) {
      std::set<int64_t>::iterator edge_it;
      std::set<int64_t> &edge_list = expected_edges.at(std::make_pair(STINGER_RO_EDGE_TYPE,STINGER_RO_EDGE_SOURCE));
      edge_it = edge_list.find(STINGER_RO_EDGE_DEST);
      if (edge_it == edge_list.end()) {
        ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_RO_EDGE_TYPE 
                      <<  " SOURCE: " << STINGER_RO_EDGE_SOURCE 
                      << " DEST: " << STINGER_RO_EDGE_DEST; 
      } else {
        edge_list.erase(edge_it);
      }
    } STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_END();
  }

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_edges.begin(); map_it != expected_edges.end(); map_it++) {
    std::set<int64_t> edge_list = map_it->second;
    if (!edge_list.empty()) {
      ADD_FAILURE() << "Not all edges traversed " << map_it->first.second;
    
      std::set<int64_t>::iterator edge_it;
      for (edge_it = edge_list.begin(); edge_it != edge_list.end(); edge_it++) {
        std::cerr << map_it->first.first << " " << map_it->first.second << " " << *edge_it << std::endl;
      }
    }
  }
}

TEST_F(StingerTraversalTest, STINGER_READ_ONLY_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN) {
  for (int64_t v=0; v < 200; v++) {
    STINGER_READ_ONLY_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(S,1,v) {
      std::set<int64_t>::iterator edge_it;
      std::set<int64_t> &edge_list = expected_edges.at(std::make_pair(STINGER_RO_EDGE_TYPE,STINGER_RO_EDGE_SOURCE));
      edge_it = edge_list.find(STINGER_RO_EDGE_DEST);
      if (edge_it == edge_list.end()) {
        ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_RO_EDGE_TYPE 
                      <<  " SOURCE: " << STINGER_RO_EDGE_SOURCE 
                      << " DEST: " << STINGER_RO_EDGE_DEST; 
      } else {
        edge_list.erase(edge_it);
      }
    } STINGER_READ_ONLY_FORALL_EDGES_OF_TYPE_OF_VTX_END();
  }

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_edges.begin(); map_it != expected_edges.end(); map_it++) {
    std::set<int64_t> edge_list = map_it->second;
    if (map_it->first.first == 1 && !edge_list.empty()) {
      ADD_FAILURE() << "Not all edges traversed " << map_it->first.second;
    
      std::set<int64_t>::iterator edge_it;
      for (edge_it = edge_list.begin(); edge_it != edge_list.end(); edge_it++) {
        std::cerr << map_it->first.first << " " << map_it->first.second << " " << *edge_it << std::endl;
      }
    }
  }
}

TEST_F(StingerTraversalTest, STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN) {
  for (int64_t v=0; v < 200; v++) {
    STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(S,v) {
      std::set<int64_t>::iterator edge_it;
      std::set<int64_t> * edge_list;
      OMP("omp critical") 
      {
        edge_list = &expected_edges.at(std::make_pair(STINGER_RO_EDGE_TYPE,STINGER_RO_EDGE_SOURCE));
        edge_it = edge_list->find(STINGER_RO_EDGE_DEST);
      }
      if (edge_it == edge_list->end()) {
        ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_RO_EDGE_TYPE 
                      <<  " SOURCE: " << STINGER_RO_EDGE_SOURCE 
                      << " DEST: " << STINGER_RO_EDGE_DEST; 
      } else {
        OMP("omp critical") 
        {
          edge_list = &expected_edges.at(std::make_pair(STINGER_RO_EDGE_TYPE,STINGER_RO_EDGE_SOURCE));
          edge_it = edge_list->find(STINGER_RO_EDGE_DEST);
          edge_list->erase(edge_it);
        }

      }
    } STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_END();
  }

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_edges.begin(); map_it != expected_edges.end(); map_it++) {
    std::set<int64_t> edge_list = map_it->second;
    if (!edge_list.empty()) {
      ADD_FAILURE() << "Not all edges traversed " << map_it->first.second;
    
      std::set<int64_t>::iterator edge_it;
      for (edge_it = edge_list.begin(); edge_it != edge_list.end(); edge_it++) {
        std::cerr << map_it->first.first << " " << map_it->first.second << " " << *edge_it << std::endl;
      }
    }
  }
}

TEST_F(StingerTraversalTest, STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN) {
  for (int64_t v=0; v < 200; v++) {
    STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(S,1,v) {
      std::set<int64_t>::iterator edge_it;
      std::set<int64_t> * edge_list;
      OMP("omp critical") 
      {
        edge_list = &expected_edges.at(std::make_pair(STINGER_RO_EDGE_TYPE,STINGER_RO_EDGE_SOURCE));
        edge_it = edge_list->find(STINGER_RO_EDGE_DEST);
      }
      if (edge_it == edge_list->end()) {
        ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_RO_EDGE_TYPE 
                      <<  " SOURCE: " << STINGER_RO_EDGE_SOURCE 
                      << " DEST: " << STINGER_RO_EDGE_DEST; 
      } else {
        OMP("omp critical") 
        {
          edge_list = &expected_edges.at(std::make_pair(STINGER_RO_EDGE_TYPE,STINGER_RO_EDGE_SOURCE));
          edge_it = edge_list->find(STINGER_RO_EDGE_DEST);
          edge_list->erase(edge_it);
        }

      }
    } STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END();
  }

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_edges.begin(); map_it != expected_edges.end(); map_it++) {
    std::set<int64_t> edge_list = map_it->second;
    if (map_it->first.first == 1 && !edge_list.empty()) {
      ADD_FAILURE() << "Not all edges traversed " << map_it->first.second;
    
      std::set<int64_t>::iterator edge_it;
      for (edge_it = edge_list.begin(); edge_it != edge_list.end(); edge_it++) {
        std::cerr << map_it->first.first << " " << map_it->first.second << " " << *edge_it << std::endl;
      }
    }
  }
}

TEST_F(StingerTraversalTest, STINGER_READ_ONLY_FORALL_EDGES_BEGIN) {
  STINGER_READ_ONLY_FORALL_EDGES_BEGIN(S,1) {
    std::set<int64_t>::iterator edge_it;
    std::set<int64_t> &edge_list = expected_edges.at(std::make_pair(STINGER_RO_EDGE_TYPE,STINGER_RO_EDGE_SOURCE));
    edge_it = edge_list.find(STINGER_RO_EDGE_DEST);
    if (edge_it == edge_list.end()) {
      ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_RO_EDGE_TYPE 
                    <<  " SOURCE: " << STINGER_RO_EDGE_SOURCE 
                    << " DEST: " << STINGER_RO_EDGE_DEST; 
    } else {
      edge_list.erase(edge_it);
    }
  } STINGER_READ_ONLY_FORALL_EDGES_END();

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_edges.begin(); map_it != expected_edges.end(); map_it++) {
    std::set<int64_t> edge_list = map_it->second;
    if (map_it->first.first == 1 && !edge_list.empty()) {
      ADD_FAILURE() << "Not all edges traversed " << map_it->first.second;
    
      std::set<int64_t>::iterator edge_it;
      for (edge_it = edge_list.begin(); edge_it != edge_list.end(); edge_it++) {
        std::cerr << map_it->first.first << " " << map_it->first.second << " " << *edge_it << std::endl;
      }
    }
  }
}

TEST_F(StingerTraversalTest, STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_BEGIN) {
  STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_BEGIN(S,1) {
    std::set<int64_t>::iterator edge_it;
    std::set<int64_t> * edge_list;
    OMP("omp critical") 
    {
      edge_list = &expected_edges.at(std::make_pair(STINGER_RO_EDGE_TYPE,STINGER_RO_EDGE_SOURCE));
      edge_it = edge_list->find(STINGER_RO_EDGE_DEST);
    }
    if (edge_it == edge_list->end()) {
      ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_RO_EDGE_TYPE 
                    <<  " SOURCE: " << STINGER_RO_EDGE_SOURCE 
                    << " DEST: " << STINGER_RO_EDGE_DEST; 
    } else {
      OMP("omp critical") 
      {
        edge_list = &expected_edges.at(std::make_pair(STINGER_RO_EDGE_TYPE,STINGER_RO_EDGE_SOURCE));
        edge_it = edge_list->find(STINGER_RO_EDGE_DEST);
        if (edge_it != edge_list->end()) {
          edge_list->erase(edge_it);
        }
      }

    }
  } STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_END();

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_edges.begin(); map_it != expected_edges.end(); map_it++) {
    std::set<int64_t> edge_list = map_it->second;
    if (map_it->first.first == 1 && !edge_list.empty()) {
      ADD_FAILURE() << "Not all edges traversed " << map_it->first.second;
    
      std::set<int64_t>::iterator edge_it;
      for (edge_it = edge_list.begin(); edge_it != edge_list.end(); edge_it++) {
        std::cerr << map_it->first.first << " " << map_it->first.second << " " << *edge_it << std::endl;
      }
    }
  }
}

