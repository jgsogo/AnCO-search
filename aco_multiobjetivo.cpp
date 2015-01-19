
#include <iostream>
#include "aco_multiobjetivo.h"


namespace AnCO {

    namespace algorithm {

        std::map<std::string, std::pair<int, float>> aco_multiobjetivo::objective_list;

        void print_path(aco_multiobjetivo::_t_ant_path::iterator begin, aco_multiobjetivo::_t_ant_path::iterator end) {
            std::cout << (*begin)->init;
            for (auto it = begin; it!=end; ++it) {
                std::cout << " -> " << (*it)->end;
                }            
            };
  

        // La elección de los edges está condicionada a los objetivos...
        edge_ptr aco_multiobjetivo::select_edge(const std::vector<edge_ptr>& feasible_edges, const unsigned int& pherom_id, std::vector<std::pair<std::string, int>>& objetivos) {
            // La elección de los edges tiene en cuenta lo siguiente (en este orden):
            // 1) Buscamos la feromona de alguno de los objetivos futuros
            // 2) Si tenemos un objetivo, caminamos en feromona ascendente
            // 3) Si no tenemos objetivo, random select.

            edge_ptr next;
            std::pair<int, float> next_metrics = std::make_pair(objetivos.size(), 0.f);

            // 1-2) Elegimos el camino que nos lleva hacia el máximo de feromona de alguno de nuestros objetivos en orden de prioridad
            for (auto fe = feasible_edges.begin(); fe != feasible_edges.end(); ++fe) {
                int obj_index = objetivos.size();
                //int pheromon = GLOBALS::n_colonies;
                for ( auto oo = 0; oo<obj_index; ++oo) {
                    float pheromon_value = (*fe)->data.pheromone[oo];
                    if ( pheromon_value > 0.01f) {
                        if ((oo < next_metrics.first) || ( (oo==next_metrics.first) && ( pheromon_value > next_metrics.second ) )) {
                            next = *fe;
                            next_metrics = std::make_pair(oo, pheromon_value);
                            break;
                            }
                        }
                    }
                }

            if (next) {
                // Elimino todos los objetivos menos prioritarios que el que he encontrado
                objetivos.erase(objetivos.begin() + next_metrics.first + 1, objetivos.end());
                // Elimino este objetivo si he llegado al nodo central del hormiguero
                if (objetivos[next_metrics.first].first == next->end) {
                    objetivos.erase(objetivos.begin() + next_metrics.first - 1);
                    }
                }
            else {
                next = aco_mmas::select_edge(feasible_edges, pherom_id);
                }
            return next;
            }

        void aco_multiobjetivo::select_paths(std::vector<std::pair<_t_ant_path, bool>>& tmp_paths) {

            // Selecciono el camino que más lejos haya llegado (el que más haya conseguido puntuar)
            //      static std::map<std::string, float> objective_list;

            // Creamos una lista de objetivos (primero el último, el más prioritario)
            std::vector<std::pair<std::string, int>> objetivos;
            for (auto oo = objective_list.rbegin(); oo != objective_list.rend(); ++oo) {
                objetivos.push_back( std::make_pair(oo->first, oo->second.first));
                }

            _t_ant_path selected;
            std::pair<int, float> selected_metric = std::make_pair(objetivos.size(), 0.f);
            std::vector<std::pair<_t_ant_path, bool>> selected_paths;
                       
            for (auto it=tmp_paths.begin(); it!=tmp_paths.end(); ++it) {
                if ( (*it->first.rbegin())->end == (objetivos.begin()->first)) {
                    std::cout << std::endl << "\t\t FOUND (" << it->first.size() << "): "; print_path(it->first.begin(), it->first.end()); std::cout << std::endl;
                    selected_paths.push_back( std::make_pair(it->first, true) );
                    }
                else {
                    int obj_index = objetivos.size();
                    for (auto jj = it->first.begin(); jj != it->first.end(); ++jj) {
                        for ( auto oo = 0; oo<obj_index; ++oo) {
                            float pheromon_value = (*jj)->data.pheromone[oo];
                            if ( pheromon_value > 0.01f) {
                                if ((oo < selected_metric.first) || ( (oo==selected_metric.first) && ( pheromon_value > selected_metric.second ) )) {
                                    selected = _t_ant_path(it->first.begin(), jj);
                                    selected_metric = std::make_pair(oo, pheromon_value);
                                    break;
                                    }
                                }
                            }
                        }
                    }
                }
            
            tmp_paths.clear();
            if (selected_metric.second != 0.f) {
                //std::cout << std::endl << "\t\t obj [" << selected_metric.first  << "]: "; print_path(selected.begin(), selected.end()); std::cout << std::endl;
                selected_paths.push_back(std::make_pair(selected, false));
                }
            tmp_paths = selected_paths;
            }

        // IDEM ACO_MMAS
        bool aco_multiobjetivo::run(graph& graph, const graph::_t_node_id& node, const unsigned int& pherom_id, _f_success& suc, std::vector<edge_ptr>& _path, const int& max_steps) {
            // Creamos una lista de objetivos (primero el último, el más prioritario)
            std::vector<std::pair<std::string, int>> objetivos;
            for (auto oo = objective_list.rbegin(); oo != objective_list.rend(); ++oo) {
                objetivos.push_back( std::make_pair(oo->first, oo->second.first));
                }

            std::set<graph::_t_node_id> visited;
            graph::_t_node_id current_node = node;
            visited.insert(current_node);
            int step = 0;
            edge_ptr edge;
            bool succeeded = false;
            do {
                // 1) Calcular los edges que son posibles
                std::vector<edge_ptr> feasible_edges;
                int n = aco_multiobjetivo::get_feasible_edges(graph, current_node, feasible_edges, visited);
                if (n == 0) {
                    break; // break. No more nodes to visit.
                    }

                // 2) Elegir uno
                edge_ptr edge = aco_multiobjetivo::select_edge(feasible_edges, pherom_id, objetivos);
                
                // 3) Añadir al path y actualizar variables.
                _path.push_back(edge);
                current_node = edge->end;
                visited.insert(current_node);
                succeeded = suc(edge) || !objetivos.size();
                ++step;
                }
            while(!succeeded && step<max_steps);
            return succeeded;
            } 


        }
    }     