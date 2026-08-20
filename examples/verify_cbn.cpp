#include "cbnetwork/cbnetwork.hpp"
#include "cbnetwork/directededge.hpp"
#include "cbnetwork/internalvariable.hpp"
#include "cbnetwork/localnetwork.hpp"
#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>

using namespace cbnetwork;

int main(int argc, char **argv) {
  std::freopen("verificacion_debug.log", "w", stdout);

  std::string input_json = "experiments/mix/red_auditoria.json";
  if (argc > 1)
    input_json = argv[1];

  auto cbn = CBN::load_network_from_json(input_json);
  if (!cbn) {
    std::cout << "Error loading JSON" << std::endl;
    return 1;
  }

  cbn->find_local_attractors_sequential();
  cbn->find_compatible_pairs_parallel();

  std::cout << "\n--- DIAGNOSTICO DE COMPATIBILIDAD ---" << std::endl;
  for (const auto &edge : cbn->l_directed_edges) {
    std::cout << "Relacion entre Net " << edge->input_local_network << " y Net "
              << edge->output_local_network << std::endl;
    // Accedemos a la estructura donde se guardan los pares compatibles
    int total_pairs = edge->d_comp_pairs_attractors_by_value[0].size() +
                      edge->d_comp_pairs_attractors_by_value[1].size();
    std::cout << "Pares compatibles encontrados: " << total_pairs << std::endl;
  }

  // --- DIAGNOSTICO DE REDES ---
  for (const auto &net : cbn->l_local_networks) {
    std::cout << "Red " << net->index << " tiene " << net->attractor_count
              << " atractores locales." << std::endl;
  }

  cbn->mount_stable_attractor_fields();

  std::cout << "SISTEMA COMPLETO CARGADO." << std::endl;
  std::cout << "Total de Campos de Atractores encontrados: "
            << cbn->get_n_attractor_fields() << std::endl;

  return 0;
}