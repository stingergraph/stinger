#include "stinger_traversal_test.h"

TEST_F(StingerTraversalTest, STINGER_FORALL_EDGES_OF_VTX_BEGIN) {
  for (int64_t v=0; v < 200; v++) {
    STINGER_FORALL_EDGES_OF_VTX_BEGIN(S,v) {
      std::set<int64_t>::iterator edge_it;
      if (STINGER_IS_OUT_EDGE) {
        std::set<int64_t> &edge_list = expected_out_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
        edge_it = edge_list.find(STINGER_EDGE_DEST);
        if (edge_it == edge_list.end()) {
          ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                        <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                        << " DEST: " << STINGER_EDGE_DEST; 
        } else {
          edge_list.erase(edge_it);
        }
      }
      if (STINGER_IS_IN_EDGE) {
        std::set<int64_t> &edge_list = expected_in_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
        edge_it = edge_list.find(STINGER_EDGE_DEST);
        if (edge_it == edge_list.end()) {
          ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                        <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                        << " DEST: " << STINGER_EDGE_DEST; 
        } else {
          edge_list.erase(edge_it);
        }
      }
    } STINGER_FORALL_EDGES_OF_VTX_END();
  }

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_out_edges.begin(); map_it != expected_out_edges.end(); map_it++) {
    std::set<int64_t> edge_list = map_it->second;
    if (!edge_list.empty()) {
      ADD_FAILURE() << "Not all edges traversed " << map_it->first.second;
    
      std::set<int64_t>::iterator edge_it;
      for (edge_it = edge_list.begin(); edge_it != edge_list.end(); edge_it++) {
        std::cerr << map_it->first.first << " " << map_it->first.second << " " << *edge_it << std::endl;
      }
    }
  }

  for (map_it = expected_in_edges.begin(); map_it != expected_in_edges.end(); map_it++) {
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

TEST_F(StingerTraversalTest, STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN) {
  for (int64_t v=0; v < 200; v++) {
    STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(S,v) {
      std::set<int64_t>::iterator edge_it;
      std::set<int64_t> &edge_list = expected_out_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
      EXPECT_TRUE(STINGER_IS_OUT_EDGE);
      edge_it = edge_list.find(STINGER_EDGE_DEST);
      if (edge_it == edge_list.end()) {
        ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                      <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                      << " DEST: " << STINGER_EDGE_DEST; 
      } else {
        edge_list.erase(edge_it);
      }
    } STINGER_FORALL_OUT_EDGES_OF_VTX_END();
  }

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_out_edges.begin(); map_it != expected_out_edges.end(); map_it++) {
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

TEST_F(StingerTraversalTest, STINGER_FORALL_IN_EDGES_OF_VTX_BEGIN) {
  for (int64_t v=0; v < 200; v++) {
    STINGER_FORALL_IN_EDGES_OF_VTX_BEGIN(S,v) {
      std::set<int64_t>::iterator edge_it;
      std::set<int64_t> &edge_list = expected_in_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
      EXPECT_TRUE(STINGER_IS_IN_EDGE);
      edge_it = edge_list.find(STINGER_EDGE_DEST);
      if (edge_it == edge_list.end()) {
        ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                      <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                      << " DEST: " << STINGER_EDGE_DEST; 
      } else {
        edge_list.erase(edge_it);
      }
    } STINGER_FORALL_IN_EDGES_OF_VTX_END();
  }

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_in_edges.begin(); map_it != expected_in_edges.end(); map_it++) {
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

TEST_F(StingerTraversalTest, STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN) {
  for (int64_t v=0; v < 200; v++) {
    STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(S,1,v) {
      std::set<int64_t>::iterator edge_it;
      if (STINGER_IS_OUT_EDGE) {
        std::set<int64_t> &edge_list = expected_out_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
        edge_it = edge_list.find(STINGER_EDGE_DEST);
        if (edge_it == edge_list.end()) {
          ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                        <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                        << " DEST: " << STINGER_EDGE_DEST; 
        } else {
          edge_list.erase(edge_it);
        }
      }
      if (STINGER_IS_IN_EDGE) {
        std::set<int64_t> &edge_list = expected_in_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
        edge_it = edge_list.find(STINGER_EDGE_DEST);
        if (edge_it == edge_list.end()) {
          ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                        <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                        << " DEST: " << STINGER_EDGE_DEST; 
        } else {
          edge_list.erase(edge_it);
        }
      }      
    } STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_END();
  }

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_out_edges.begin(); map_it != expected_out_edges.end(); map_it++) {
    std::set<int64_t> edge_list = map_it->second;
    if (map_it->first.first == 1 && !edge_list.empty()) {
      ADD_FAILURE() << "Not all edges traversed " << map_it->first.second;
    
      std::set<int64_t>::iterator edge_it;
      for (edge_it = edge_list.begin(); edge_it != edge_list.end(); edge_it++) {
        std::cerr << map_it->first.first << " " << map_it->first.second << " " << *edge_it << std::endl;
      }
    }
  }

  for (map_it = expected_in_edges.begin(); map_it != expected_in_edges.end(); map_it++) {
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

TEST_F(StingerTraversalTest, STINGER_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN) {
  for (int64_t v=0; v < 200; v++) {
    STINGER_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN(S,1,v) {
      std::set<int64_t>::iterator edge_it;
      EXPECT_TRUE(STINGER_IS_OUT_EDGE);
      std::set<int64_t> &edge_list = expected_out_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
      edge_it = edge_list.find(STINGER_EDGE_DEST);
      if (edge_it == edge_list.end()) {
        ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                      <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                      << " DEST: " << STINGER_EDGE_DEST; 
      } else {
        edge_list.erase(edge_it);
      }
    } STINGER_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END();
  }

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_out_edges.begin(); map_it != expected_out_edges.end(); map_it++) {
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

TEST_F(StingerTraversalTest, STINGER_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN) {
  for (int64_t v=0; v < 200; v++) {
    STINGER_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN(S,1,v) {
      std::set<int64_t>::iterator edge_it;
      EXPECT_TRUE(STINGER_IS_IN_EDGE);
      std::set<int64_t> &edge_list = expected_in_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
      edge_it = edge_list.find(STINGER_EDGE_DEST);
      if (edge_it == edge_list.end()) {
        ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                      <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                      << " DEST: " << STINGER_EDGE_DEST; 
      } else {
        edge_list.erase(edge_it);
      }
    } STINGER_FORALL_IN_EDGES_OF_TYPE_OF_VTX_END();
  }

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_in_edges.begin(); map_it != expected_in_edges.end(); map_it++) {
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

TEST_F(StingerTraversalTest, STINGER_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN) {
  for (int64_t v=0; v < 200; v++) {
    STINGER_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(S,v) {
      std::set<int64_t>::iterator edge_it;
      std::set<int64_t> * edge_list;
      if (STINGER_IS_OUT_EDGE) {
        OMP("omp critical") 
        {
          edge_list = &expected_out_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
          edge_it = edge_list->find(STINGER_EDGE_DEST);
        }
        if (edge_it == edge_list->end()) {
          ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                        <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                        << " DEST: " << STINGER_EDGE_DEST; 
        } else {
          OMP("omp critical") 
          {
            edge_list = &expected_out_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
            edge_it = edge_list->find(STINGER_EDGE_DEST);
            edge_list->erase(edge_it);
          }

        }
      }
      if (STINGER_IS_IN_EDGE) {
        OMP("omp critical") 
        {
          edge_list = &expected_in_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
          edge_it = edge_list->find(STINGER_EDGE_DEST);
        }
        if (edge_it == edge_list->end()) {
          ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                        <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                        << " DEST: " << STINGER_EDGE_DEST; 
        } else {
          OMP("omp critical") 
          {
            edge_list = &expected_in_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
            edge_it = edge_list->find(STINGER_EDGE_DEST);
            edge_list->erase(edge_it);
          }

        }
      }
    } STINGER_PARALLEL_FORALL_EDGES_OF_VTX_END();
  }

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_out_edges.begin(); map_it != expected_out_edges.end(); map_it++) {
    std::set<int64_t> edge_list = map_it->second;
    if (!edge_list.empty()) {
      ADD_FAILURE() << "Not all edges traversed " << map_it->first.second;
    
      std::set<int64_t>::iterator edge_it;
      for (edge_it = edge_list.begin(); edge_it != edge_list.end(); edge_it++) {
        std::cerr << map_it->first.first << " " << map_it->first.second << " " << *edge_it << std::endl;
      }
    }
  }
  for (map_it = expected_in_edges.begin(); map_it != expected_in_edges.end(); map_it++) {
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

TEST_F(StingerTraversalTest, STINGER_PARALLEL_FORALL_OUT_EDGES_OF_VTX_BEGIN) {
  for (int64_t v=0; v < 200; v++) {
    STINGER_PARALLEL_FORALL_OUT_EDGES_OF_VTX_BEGIN(S,v) {
      std::set<int64_t>::iterator edge_it;
      std::set<int64_t> * edge_list;
      EXPECT_TRUE(STINGER_IS_OUT_EDGE);
      OMP("omp critical") 
      {
        edge_list = &expected_out_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
        edge_it = edge_list->find(STINGER_EDGE_DEST);
      }
      if (edge_it == edge_list->end()) {
        ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                      <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                      << " DEST: " << STINGER_EDGE_DEST; 
      } else {
        OMP("omp critical") 
        {
          edge_list = &expected_out_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
          edge_it = edge_list->find(STINGER_EDGE_DEST);
          edge_list->erase(edge_it);
        }

      }
    } STINGER_PARALLEL_FORALL_OUT_EDGES_OF_VTX_END();
  }

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_out_edges.begin(); map_it != expected_out_edges.end(); map_it++) {
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

TEST_F(StingerTraversalTest, STINGER_PARALLEL_FORALL_IN_EDGES_OF_VTX_BEGIN) {
  for (int64_t v=0; v < 200; v++) {
    STINGER_PARALLEL_FORALL_IN_EDGES_OF_VTX_BEGIN(S,v) {
      std::set<int64_t>::iterator edge_it;
      std::set<int64_t> * edge_list;
      EXPECT_TRUE(STINGER_IS_IN_EDGE);
      OMP("omp critical") 
      {
        edge_list = &expected_in_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
        edge_it = edge_list->find(STINGER_EDGE_DEST);
      }
      if (edge_it == edge_list->end()) {
        ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                      <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                      << " DEST: " << STINGER_EDGE_DEST; 
      } else {
        OMP("omp critical") 
        {
          edge_list = &expected_in_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
          edge_it = edge_list->find(STINGER_EDGE_DEST);
          edge_list->erase(edge_it);
        }

      }
    } STINGER_PARALLEL_FORALL_IN_EDGES_OF_VTX_END();
  }

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_in_edges.begin(); map_it != expected_in_edges.end(); map_it++) {
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

TEST_F(StingerTraversalTest, STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN) {
  for (int64_t v=0; v < 200; v++) {
    STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(S,1,v) {
      std::set<int64_t>::iterator edge_it;
      std::set<int64_t> * edge_list;
      if (STINGER_IS_OUT_EDGE) {
        OMP("omp critical") 
        {
          edge_list = &expected_out_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
          edge_it = edge_list->find(STINGER_EDGE_DEST);
        }
        if (edge_it == edge_list->end()) {
          ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                        <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                        << " DEST: " << STINGER_EDGE_DEST; 
        } else {
          OMP("omp critical") 
          {
            edge_list = &expected_out_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
            edge_it = edge_list->find(STINGER_EDGE_DEST);
            edge_list->erase(edge_it);
          }
        }
      }
      if (STINGER_IS_IN_EDGE) {
        OMP("omp critical") 
        {
          edge_list = &expected_in_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
          edge_it = edge_list->find(STINGER_EDGE_DEST);
        }
        if (edge_it == edge_list->end()) {
          ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                        <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                        << " DEST: " << STINGER_EDGE_DEST; 
        } else {
          OMP("omp critical") 
          {
            edge_list = &expected_in_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
            edge_it = edge_list->find(STINGER_EDGE_DEST);
            edge_list->erase(edge_it);
          }
        }
      }
    } STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END();
  }

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_out_edges.begin(); map_it != expected_out_edges.end(); map_it++) {
    std::set<int64_t> edge_list = map_it->second;
    if (map_it->first.first == 1 && !edge_list.empty()) {
      ADD_FAILURE() << "Not all edges traversed " << map_it->first.second;
    
      std::set<int64_t>::iterator edge_it;
      for (edge_it = edge_list.begin(); edge_it != edge_list.end(); edge_it++) {
        std::cerr << map_it->first.first << " " << map_it->first.second << " " << *edge_it << std::endl;
      }
    }
  }
  for (map_it = expected_in_edges.begin(); map_it != expected_in_edges.end(); map_it++) {
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

TEST_F(StingerTraversalTest, STINGER_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN) {
  for (int64_t v=0; v < 200; v++) {
    STINGER_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN(S,1,v) {
      std::set<int64_t>::iterator edge_it;
      std::set<int64_t> * edge_list;
      EXPECT_TRUE(STINGER_IS_OUT_EDGE);
      OMP("omp critical") 
      {
        edge_list = &expected_out_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
        edge_it = edge_list->find(STINGER_EDGE_DEST);
      }
      if (edge_it == edge_list->end()) {
        ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                      <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                      << " DEST: " << STINGER_EDGE_DEST; 
      } else {
        OMP("omp critical") 
        {
          edge_list = &expected_out_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
          edge_it = edge_list->find(STINGER_EDGE_DEST);
          edge_list->erase(edge_it);
        }
      }
    } STINGER_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END();
  }

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_out_edges.begin(); map_it != expected_out_edges.end(); map_it++) {
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

TEST_F(StingerTraversalTest, STINGER_PARALLEL_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN) {
  for (int64_t v=0; v < 200; v++) {
    STINGER_PARALLEL_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN(S,1,v) {
      std::set<int64_t>::iterator edge_it;
      std::set<int64_t> * edge_list;
      EXPECT_TRUE(STINGER_IS_IN_EDGE);
      OMP("omp critical") 
      {
        edge_list = &expected_in_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
        edge_it = edge_list->find(STINGER_EDGE_DEST);
      }
      if (edge_it == edge_list->end()) {
        ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                      <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                      << " DEST: " << STINGER_EDGE_DEST; 
      } else {
        OMP("omp critical") 
        {
          edge_list = &expected_in_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
          edge_it = edge_list->find(STINGER_EDGE_DEST);
          edge_list->erase(edge_it);
        }
      }
    } STINGER_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END();
  }

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_in_edges.begin(); map_it != expected_in_edges.end(); map_it++) {
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

TEST_F(StingerTraversalTest, STINGER_FORALL_EDGES_BEGIN) {
  STINGER_FORALL_EDGES_BEGIN(S,1) {
    std::set<int64_t>::iterator edge_it;
    std::set<int64_t> &edge_list = expected_out_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
    edge_it = edge_list.find(STINGER_EDGE_DEST);
    if (edge_it == edge_list.end()) {
      ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                    <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                    << " DEST: " << STINGER_EDGE_DEST; 
    } else {
      edge_list.erase(edge_it);
    }
  } STINGER_FORALL_EDGES_END();

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_out_edges.begin(); map_it != expected_out_edges.end(); map_it++) {
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

TEST_F(StingerTraversalTest, STINGER_PARALLEL_FORALL_EDGES_BEGIN) {
  STINGER_PARALLEL_FORALL_EDGES_BEGIN(S,1) {
    std::set<int64_t>::iterator edge_it;
    std::set<int64_t> * edge_list;
    OMP("omp critical") 
    {
      edge_list = &expected_out_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
      edge_it = edge_list->find(STINGER_EDGE_DEST);
    }
    if (edge_it == edge_list->end()) {
      ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                    <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                    << " DEST: " << STINGER_EDGE_DEST; 
    } else {
      OMP("omp critical") 
      {
        edge_list = &expected_out_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
        edge_it = edge_list->find(STINGER_EDGE_DEST);
        edge_list->erase(edge_it);
      }

    }
  } STINGER_PARALLEL_FORALL_EDGES_END();

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_out_edges.begin(); map_it != expected_out_edges.end(); map_it++) {
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

TEST_F(StingerTraversalTest, STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN) {
  STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S) {
    std::set<int64_t>::iterator edge_it;
    std::set<int64_t> &edge_list = expected_out_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
    edge_it = edge_list.find(STINGER_EDGE_DEST);
    if (edge_it == edge_list.end()) {
      ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                    <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                    << " DEST: " << STINGER_EDGE_DEST; 
    } else {
      edge_list.erase(edge_it);
    }
  } STINGER_FORALL_EDGES_OF_ALL_TYPES_END();

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_out_edges.begin(); map_it != expected_out_edges.end(); map_it++) {
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

TEST_F(StingerTraversalTest, STINGER_PARALLEL_FORALL_EDGES_OF_ALL_TYPES_BEGIN) {
  STINGER_PARALLEL_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S) {
    std::set<int64_t>::iterator edge_it;
    std::set<int64_t> * edge_list;
    OMP("omp critical") 
    {
      edge_list = &expected_out_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
      edge_it = edge_list->find(STINGER_EDGE_DEST);
    }
    if (edge_it == edge_list->end()) {
      ADD_FAILURE() << "Unexpected edge -- Type: " << STINGER_EDGE_TYPE 
                    <<  " SOURCE: " << STINGER_EDGE_SOURCE 
                    << " DEST: " << STINGER_EDGE_DEST; 
    } else {
      OMP("omp critical") 
      {
        edge_list = &expected_out_edges.at(std::make_pair(STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE));
        edge_it = edge_list->find(STINGER_EDGE_DEST);
        edge_list->erase(edge_it);
      }

    }
  } STINGER_PARALLEL_FORALL_EDGES_OF_ALL_TYPES_END();

  std::map< std::pair<int64_t, int64_t>, std::set<int64_t> >::iterator map_it;

  for (map_it = expected_out_edges.begin(); map_it != expected_out_edges.end(); map_it++) {
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