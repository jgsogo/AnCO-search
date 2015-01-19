/**
 * Print a simple "Hello world!"
 *
 * @file main.cpp
 * @section LICENSE

    This code is under MIT License, http://opensource.org/licenses/MIT
 */

#include <iostream>
#include <iterator>
#include <iomanip>

#include "jgsogo/AnCO/config.h"

#include "jgsogo/AnCO/graph/memgraph.h"
#include "jgsogo/AnCO/graph/graph_data_file.h"
#include "jgsogo/AnCO/log.h"
#include "jgsogo/AnCO/log_time.h"

#include "jgsogo/AnCO/algorithm/aco_base.h"
#include "jgsogo/AnCO/algorithm/aco_random.h"
#include "jgsogo/AnCO/algorithm/aco_mmas.h"
#include "jgsogo/AnCO/algorithm/prox_base.h"
#include "jgsogo/AnCO/algorithm/prox1.h"
#include "jgsogo/AnCO/algorithm/prox_percent.h"

#include "jgsogo/AnCO/colony/neighbourhood.h"
#include "jgsogo/AnCO/utils/threading.h"
#include "jgsogo/AnCO/utils/sleep.h"

#include "success_meta.h"
#include "aco_multiobjetivo.h"

#ifdef _WINDOWS
    #include <windows.h>
#endif
#ifdef __cplusplus__
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif

using namespace AnCO;

typedef algorithm::prox_percent prox_algorithm;
typedef algorithm::aco_random aco_algorithm;

typedef AnCO::colony<algorithm::aco_base> colony_type;

typedef AnCO::colony_neighbourhood<aco_algorithm, prox_algorithm> colony_neighbourhood_type;
typedef AnCO::neighbourhood<aco_algorithm, prox_algorithm> neighbourhood_type;


int main(int argc, char* argv[]) {
    if (argc < 2) { // Check the number of parameters
        // Tell the user how to run the program
        std::cerr << "Usage: " << argv[0] << " 'CONFIG_FILE' ['GRAPH_DATASET']" << std::endl;
        return 1;
        }
    config cfg = load_config(argv[1]);
    if (argc>2) {
        cfg.dataset = argv[2];
        }

    #ifdef _WINDOWS
        HWND console = GetConsoleWindow();
        RECT r;
        GetWindowRect(console, &r); //stores the console's current dimensions
        MoveWindow(console, r.left, r.top, 1500, 800, TRUE); // 800 width, 100 height
    #endif

    std::cout << "======" << std::endl;
    std::cout << "AnCO\n";
    std::cout << "======" << std::endl << std::endl;

    std::cout << "1) Graph dataset from file" << std::endl;
    graph_data_file dataset(cfg.dataset);
    log_time t;
    dataset.load_file();
    t.toc();    

    std::cout << std::endl << "2) Make graph available on memory" << std::endl;
    t.tic();
    AnCO::memgraph graph(dataset);
    t.toc();

    std::cout << std::endl << "3) Create neighbourhood of '" << cfg.n_colonies << "' colonies (aco_random)" << std::endl;
    neighbourhood_type colony_meta(graph, cfg.n_colonies, cfg.n_ants_per_colony, cfg.max_steps);

    std::cout << std::endl << "4) Train for " << cfg.training_iterations << " iterations" << std::endl;
    std::cout << std::endl << "... press INTRO to continue" << std::endl; getchar();
    unsigned int colony_meta_iteration = 0;
    while(colony_meta_iteration < cfg.training_iterations) {
        colony_meta.run();
        colony_meta.update();
        colony_type::aco_algorithm_impl::update_graph(graph);

        if (system("CLS")) system("clear");
        colony_meta_iteration = colony_meta.get_iteration();            
        std::cout << "Iteration " << colony_meta_iteration << std::endl;
        colony_meta.print(std::cout);
        std::cout << std::flush;
        }
         
    std::cout << "5) Select two random nodes" << std::endl;
    node_ptr start_node = graph.get_node_random();
    node_ptr end_node = graph.get_node_random();
    std::cout << "\t start node: " << start_node->id;
    std::cout << "\t end node: " << end_node->id;

    colony_neighbourhood_type start_colony(graph, cfg.n_ants_per_colony, cfg.max_steps);
    start_colony.set_base_node(start_node->id);
    colony_neighbourhood_type end_colony(graph, cfg.n_ants_per_colony, cfg.max_steps);
    end_colony.set_base_node(end_node->id);
    unsigned int iterations = 0;
    while (++iterations < cfg.training_iterations) {
        colony_meta.run();
        start_colony.run();
        end_colony.run();
        colony_meta.update();
        start_colony.update();
        end_colony.update();
        colony_type::aco_algorithm_impl::update_graph(graph);

        if (system("CLS")) system("clear");
        colony_meta_iteration = colony_meta.get_iteration();            
        std::cout << "Iteration " << colony_meta_iteration << std::endl;
            
        colony_meta.print(std::cout);
        std::cout << std::endl;
        std::cout << "START COLONY @ node " << start_node->id << std::endl;
        start_colony.print(std::cout);
        std::cout << std::endl << std::endl;
        std::cout << "END COLONY @ node " << end_node->id << std::endl;
        end_colony.print(std::cout);
        std::cout << std::endl << std::endl;
            
        std::cout << std::flush;
        }
    
    if (start_colony.get_metric()<=0.f || end_colony.get_metric()<=0.f) {
        std::cout << " NO path possible (?)!!" << std::endl;
        return 1;
        }

    std::cout << std::endl << "------------------------ begin META-GRAPH ---------------------" << std::endl << std::endl;
    std::cout << "6) Build meta-graph" << std::endl;
    std::cout << "\t (meta-graph should we running in background)" << std::endl;
    AnCO::graph_data_file_builder meta_dataset;
    // nodos
    std::cout << "\t - nodes: base node of each colony" << std::endl;
    meta_dataset.add_node(start_node->id);
    meta_dataset.add_node(end_node->id);
    auto colonies = colony_meta.get_colonies();
    std::for_each(colonies.begin(), colonies.end(), [&meta_dataset](const std::shared_ptr<colony_neighbourhood_type>& item){
        meta_dataset.add_node(item->get_base_node());
        });

    // edges
    std::cout << "\t - edges: neighbourhood with 'probability > 0.f', cost for 'edge = 1-probability'" << std::endl;
    auto prox_start = start_colony.get_proximity_vector();
    auto prox_end = end_colony.get_proximity_vector();
    for (auto i=0; i<colonies.size(); i++) {
        if (prox_start[i]>0.f) {
            meta_dataset.add_edge(start_node->id, colonies[i]->get_base_node(), 1-prox_start[i]);
            std::cout << "\t\t " << start_node->id << " -> " << colonies[i]->get_base_node() << " | cost= '" << 1-prox_start[i] << "'" << std::endl;
            }
        if (prox_end[i]>0.f) {
            meta_dataset.add_edge(colonies[i]->get_base_node(), end_node->id, 1-prox_end[i]);
            std::cout << "\t\t " << colonies[i]->get_base_node() << " -> " << end_node->id << " | cost= '" << 1-prox_end[i] << "' <-- :S no garantizado!!" << std::endl;
            }
        }

    auto matrix = colony_meta.get_proximity_matrix();
    for (auto i=0; i<colonies.size(); i++) {
        for (auto j=0; j<colonies.size(); j++) {
            if ((i!=j) && (matrix[i][j]>0.f)) {
                meta_dataset.add_edge(colonies[i]->get_base_node(), colonies[j]->get_base_node(), 1 - matrix[i][j]);
                std::cout << "\t\t " << colonies[i]->get_base_node() << " -> " << colonies[j]->get_base_node() << " | cost= '" << 1 - matrix[i][j] << "'" << std::endl;
                }
            }
        }
    memgraph meta_graph(meta_dataset);
    
    std::cout << std::endl << "7) Search for meta-path in meta-graph (MMAS)" << std::endl;
    AnCO::colony<algorithm::aco_mmas> search_colony(meta_graph, cfg.n_ants_per_colony, cfg.max_steps);
    search_colony.set_base_node(start_node->id);
    iterations = 0;


    success_meta success(end_node->id);
    while (++iterations < cfg.training_iterations) {
        search_colony.run(success);
        search_colony.update();
        colony_type::aco_algorithm_impl::update_graph(meta_graph);        
        }
    unsigned int max_length = 0;
    if (success.succesful_paths.size()) {
        std::cout << "\t>>!! THERE IS A PATH ******" << std::endl;
        // Select path with
        std::cout << "\t - candidate meta-paths" << std::endl;
        std::cout << "\t\t cost | path" << std::endl;
        for (auto it = success.succesful_paths.begin(); it!=success.succesful_paths.end(); ++it) {
            max_length = (std::max)(max_length, it->size()*cfg.max_steps);
            float cost = std::accumulate(it->begin(), it->end(), 0.f, [](float x, edge_ptr ptr){ return x + ptr->data.length;});
            std::cout << "\t\t" << cost << " | " << (*it->begin())->init;
            for (auto jj = it->begin(); jj!=it->end(); ++jj) {
                std::cout << " -> " << (*jj)->end;
                }
            std::cout << std::endl;
            }
        }
    else {
        std::cout << "\t>>!! NO PATH FOUND IN META-GRAPH" << std::endl;
        return 1;
        }
    std::cout << std::endl << "------------------------ end META-GRAPH ---------------------" << std::endl << std::endl;

    std::cout << "7) Search for actual path in the ORIGINAL graph" << std::endl;
    std::cout << "\t expected length: 'steps <= " << max_length << "'" << std::endl;
    
    std::cout << std::endl << "... press INTRO to continue" << std::endl; getchar();

    std::cout << "Done" << std::endl;
    getchar();
    return 0;
    }
