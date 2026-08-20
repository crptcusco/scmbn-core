#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include "cbnetwork/cbnetwork.hpp"

using namespace cbnetwork;
using namespace std::chrono;

struct ExperimentParams {
    int n_samples = 10;
    int v_topology = 1; // 1: Complete, 3: Cycle, 4: Path
    int n_networks = 5;
    int n_var_network = 8;
    int n_input_variables = 2;
    int n_output_variables = 2;
    int n_max_of_clauses = 3;
    int n_max_of_literals = 2;
    std::string output_file = "results.csv";
    bool save_fields = false;
};

void print_usage(char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --samples <int>         Number of samples (default: 10)" << std::endl;
    std::cout << "  --topology <int>        Topology ID (1:Complete, 3:Cycle, 4:Path) (default: 1)" << std::endl;
    std::cout << "  --networks <int>        Number of local networks (default: 5)" << std::endl;
    std::cout << "  --vars <int>            Variables per local network (default: 8)" << std::endl;
    std::cout << "  --inputs <int>          Input signals per network (default: 2)" << std::endl;
    std::cout << "  --outputs <int>         Output variables per network (default: 2)" << std::endl;
    std::cout << "  --output_file <string>  CSV output filename (default: results.csv)" << std::endl;
    std::cout << "  --save_fields           Export global attractor fields to JSON (default: false)" << std::endl;
}

int main(int argc, char** argv) {
    ExperimentParams params;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--samples" && i + 1 < argc) params.n_samples = std::stoi(argv[++i]);
        else if (arg == "--topology" && i + 1 < argc) params.v_topology = std::stoi(argv[++i]);
        else if (arg == "--networks" && i + 1 < argc) params.n_networks = std::stoi(argv[++i]);
        else if (arg == "--vars" && i + 1 < argc) params.n_var_network = std::stoi(argv[++i]);
        else if (arg == "--inputs" && i + 1 < argc) params.n_input_variables = std::stoi(argv[++i]);
        else if (arg == "--outputs" && i + 1 < argc) params.n_output_variables = std::stoi(argv[++i]);
        else if (arg == "--output_file" && i + 1 < argc) params.output_file = argv[++i];
        else if (arg == "--save_fields") params.save_fields = true;
        else if (arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
    }

    std::cout << "Starting experiment: " << params.n_samples << " samples, "
              << params.n_networks << " networks, " << params.n_var_network << " vars." << std::endl;

    std::ofstream out(params.output_file);
    out << "sample_id,topology,n_networks,n_vars,step1_time,step2_time,step3_time,total_time,attractors,pairs,fields\n";
    out.close();

    for (int s = 0; s < params.n_samples; ++s) {
        auto start_total = high_resolution_clock::now();

        auto cbn = CBN::cbn_generator(
            params.v_topology,
            params.n_networks,
            params.n_var_network,
            params.n_input_variables,
            params.n_output_variables,
            params.n_max_of_clauses,
            params.n_max_of_literals
        );

        auto s1_start = high_resolution_clock::now();
        cbn->find_local_attractors_parallel();
        auto s1_end = high_resolution_clock::now();

        auto s2_start = high_resolution_clock::now();
        cbn->find_compatible_pairs_parallel();
        auto s2_end = high_resolution_clock::now();

        auto s3_start = high_resolution_clock::now();
        cbn->mount_stable_attractor_fields_parallel();
        auto s3_end = high_resolution_clock::now();

        auto end_total = high_resolution_clock::now();

        double t1 = duration<double>(s1_end - s1_start).count();
        double t2 = duration<double>(s2_end - s2_start).count();
        double t3 = duration<double>(s3_end - s3_start).count();
        double tt = duration<double>(end_total - start_total).count();

        int n_attr = 0;
        for(auto& net : cbn->l_local_networks) n_attr += net->attractor_count;
        int n_pairs = 0;
        for(auto& edge : cbn->l_directed_edges) {
            n_pairs += edge->d_comp_pairs_attractors_by_value[0].size();
            n_pairs += edge->d_comp_pairs_attractors_by_value[1].size();
        }
        int n_fields = cbn->d_attractor_fields.size();

        if (params.save_fields) {
            std::string fields_file = "fields_sample_" + std::to_string(s + 1) + ".json";
            cbn->save_attractor_fields_to_json(fields_file);
        }

        std::ofstream log(params.output_file, std::ios_base::app);
        log << s + 1 << "," << params.v_topology << "," << params.n_networks << "," << params.n_var_network << ","
            << t1 << "," << t2 << "," << t3 << "," << tt << "," << n_attr << "," << n_pairs << "," << n_fields << "\n";
        log.flush();
        log.close();

        if ((s + 1) % 10 == 0 || s == 0) {
            std::cout << "Sample " << s + 1 << "/" << params.n_samples << " completed." << std::endl;
        }
    }

    std::cout << "Experiment completed. Results saved to " << params.output_file << std::endl;
    return 0;
}
