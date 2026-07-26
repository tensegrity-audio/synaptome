#include <exception>
#include <iostream>

#include "circuit_trace_contract.cpp"

int main() {
    try {
        if (!circuit_trace_contract::RunEightDirectionMotionScenario()) {
            std::cerr << "[layer_authoring_native] FAIL scenario returned false\n";
            return 1;
        }
    } catch (const std::exception& error) {
        std::cerr << "[layer_authoring_native] FAIL " << error.what() << "\n";
        return 1;
    }
    std::cout << "[layer_authoring_native] PASS eight_direction_motion\n";
    return 0;
}
