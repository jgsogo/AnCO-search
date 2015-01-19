
#pragma once

#include "jgsogo/AnCO/algorithm/aco_mmas.h"
//#include "jgsogo/AnCO/colony/success.h"
#include "jgsogo/AnCO/graph/graph.h"

namespace AnCO {

    /*
    // Función de éxito
    struct success_multiobjetivo : public algorithm::success {
        virtual void new_ant() {};
        virtual bool operator()(edge_ptr ptr) {
            return false; // Las hormigas corren por la red hasta el final (pero buscando su feromona como en MMAS)
            };
        };
    */

    namespace algorithm {

        class aco_multiobjetivo : public aco_mmas {
            public:
                static void select_paths(std::vector<std::pair<_t_ant_path, bool>>& tmp_paths);
                static edge_ptr select_edge(const std::vector<edge_ptr>& feasible_edges, const unsigned int& pherom_id, std::vector<std::pair<std::string, int>>& objetivos);

                // Ejecución del algoritmo
                static bool run(    /*const*/ graph& graph,         // [in] grafo en el que me muevo
                                    const graph::_t_node_id& node,  // [in] nodo inicial
                                    const unsigned int& pherom_id,  // [in] id feromona
                                    _f_success& suc,                // [in] función para determinar si he encontrado el destino
                                    std::vector<edge_ptr>& _path,   // [out] camino seguido
                                    const int& max_steps = 100);    // [in] número máximo de pasos

                static std::map<std::string, std::pair<int, float>> objective_list; //! TODO:  lista de objetivos con puntuación e identificación de feromona, por supuesto esto no puede ser static!!!

            };


        }
    }     