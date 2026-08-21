#include <iostream>
#include <string>
#include "cbnetwork/campaign_manager.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <campaign_config.json>" << std::endl;
        return 1;
    }

    std::string config_path = argv[1];
    std::cout << "Autonomous Campaign Execution System" << std::endl;
    std::cout << "Loading configuration: " << config_path << std::endl;

    cbnetwork::CampaignManager manager;
    manager.run_campaign(config_path);

    std::cout << "Campaign completed." << std::endl;
    return 0;
}
