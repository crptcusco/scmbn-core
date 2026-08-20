#ifndef CAMPAIGN_MANAGER_HPP
#define CAMPAIGN_MANAGER_HPP

#include <string>
#include <vector>
#include <memory>
#include "cbnetwork/cbnetwork.hpp"
#include "cbnetwork/network_factory.hpp"

namespace cbnetwork {

struct CampaignExecutionResults {
    double p1_ms = 0.0;
    double p2_ms = 0.0;
    double p3_ms = 0.0;
    double total_ms = 0.0;
    long max_rss_kb = 0;
    size_t global_attractors_count = 0;
    bool success = false;
};

class CampaignManager {
public:
    CampaignManager() = default;

    void run_campaign(const std::string& config_path);

private:
    void export_experiment_data(const std::string& base_path,
                                const ExperimentConfig& config,
                                std::shared_ptr<CBN> cbn,
                                const CampaignExecutionResults& results);
};

} // namespace cbnetwork

#endif // CAMPAIGN_MANAGER_HPP
