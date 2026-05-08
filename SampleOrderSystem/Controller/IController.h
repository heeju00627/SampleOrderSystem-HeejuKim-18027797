#pragma once
#include <string>
#include <vector>

namespace Controller {

class IController {
public:
    virtual ~IController() = default;
    virtual void run() = 0;
    virtual bool handleCommand(const std::string& command,
                               const std::vector<std::string>& args) = 0;
};

} // namespace Controller
