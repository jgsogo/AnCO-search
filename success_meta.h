
#pragma once

#include "jgsogo/AnCO/colony/success.h"
#include "jgsogo/AnCO/graph/graph.h"

using namespace AnCO;

struct success_meta : success_node_found {
    success_meta(_t_graph::_t_node_id id) : AnCO::success_node_found(id) {};
    success_meta(success_meta& other) : success_node_found(other.id) {};
    virtual void new_ant() { tmp.clear();};
    virtual bool operator()(edge_ptr ptr) {
        tmp.push_back(ptr);
        bool ret = (ptr->end == id);
        if (ret) {
            add_to_succesful(tmp);
            }
        return ret;
        };

    void add_to_succesful(std::vector<edge_ptr>& path) {
        // compute hash
        std::string hash = std::accumulate(path.begin(), path.end(), (*path.begin())->init, [](std::string x, edge_ptr ptr){ return x+ptr->end;});
        std::set<std::string>::iterator it; bool inserted;
        std::tie(it, inserted) = hash_paths.insert(hash);
        if (inserted) {
            succesful_paths.push_back(path);
            }
        };

    std::vector<edge_ptr> tmp;
    
    std::set<std::string> hash_paths;
    std::vector<std::vector<edge_ptr>> succesful_paths;
    };