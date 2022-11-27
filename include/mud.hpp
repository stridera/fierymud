#pragma once

#include "structs.hpp"
#include "zone.hpp"

#include <vector>

class Mud {
    std::vector<DescriptorData> descriptors;
    std::vector<ZoneData> world;

  public:
    Mud();
    ~Mud();

    void run();
}