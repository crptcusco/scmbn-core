#include "cbnetwork/cbnetwork.hpp"
#include "cbnetwork/customtext.hpp"
#include "cbnetwork/globalnetwork.hpp"
#include <iostream>
#include <memory>

using namespace cbnetwork;

int main() {
  // Parameters for a small CBN
  int v_topology = 1; // Complete
  int n_networks = 3;
  int n_var_network = 3;
  int n_input_variables = 1;
  int n_output_variables = 1;

  CustomText::make_title("Parity Test: Python to C++ Translation");

  // Generate CBN
  auto o_cbn = CBN::cbn_generator(v_topology, n_networks, n_var_network,
                                  n_input_variables, n_output_variables);

  // Run analysis pipeline
  o_cbn->find_local_attractors_parallel();
  o_cbn->find_compatible_pairs_parallel();
  o_cbn->mount_stable_attractor_fields_parallel();

  // Si necesitas verificar el conteo, usa el getter público:
  int n_fields = o_cbn->get_n_attractor_fields();
  std::cout << "Campos de atractores detectados: " << n_fields << std::endl;

  // Verify GlobalNetwork
  GlobalNetwork::generate_global_network(o_cbn);

  // Display results
  o_cbn->show_resume();
  o_cbn->show_description();
  o_cbn->show_global_scenes();
  o_cbn->show_stable_attractor_fields();
  o_cbn->show_local_attractors_dictionary();

  std::cout << "Parity Test Completed Successfully!" << std::endl;

  return 0;
}
