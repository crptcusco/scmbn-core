#ifndef CAMPAIGN_MANAGER_HPP
#define CAMPAIGN_MANAGER_HPP

#include <string>
#include <vector>
#include <memory>
#include "cbnetwork/cbnetwork.hpp"
#include "cbnetwork/network_factory.hpp"
#include "cbnetwork/experiment_strategies.hpp"

namespace cbnetwork {

class CampaignManager {
public:
    CampaignManager() = default;

    void run_campaign(const std::string& config_path);

private:
    void export_experiment_data(const std::string& base_path,
                                const ExperimentConfig& config,
                                std::shared_ptr<CBN> cbn,
                                const ExperimentResults& results);
};

} // namespace cbnetwork

#endif // CAMPAIGN_MANAGER_HPP
