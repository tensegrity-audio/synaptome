#include "ofApp.h"
#include "ofMain.h"
#include "runtime/BuiltinElementContractExporter.h"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
	ofGLFWWindowSettings s;
	s.setGLVersion(3, 2);
	s.setSize(1280, 720);
	s.title = "Synaptome";
	auto win = ofCreateWindow(s);
	if ((argc == 3 || argc == 4) &&
		std::string(argv[1]) == "--export-builtin-element-contracts") {
		std::string error;
		if (!synaptome::runtime::exportBuiltinElementParameterContracts(
				argv[2],
				argc == 4 ? argv[3] : std::string(),
				error)) {
			std::cerr << "[element-contract-export] FAIL "
				<< error << "\n";
			return 1;
		}
		std::cout << "[element-contract-export] PASS "
			<< argv[2] << "\n";
		return 0;
	}
	if (argc == 3 &&
		std::string(argv[1]) == "--validate-builtin-element-contracts") {
		std::size_t validatedTypes = 0;
		std::size_t validatedAssets = 0;
		std::string error;
		if (!synaptome::runtime::
				validateBuiltinElementParameterContracts(
					argv[2],
					validatedTypes,
					validatedAssets,
					error)) {
			std::cerr << "[element-contract-validation] FAIL "
				<< error << "\n";
			return 1;
		}
		std::cout << "[element-contract-validation] PASS "
			<< validatedTypes << " types, "
			<< validatedAssets << " assets\n";
		return 0;
	}
	auto app = std::make_shared<ofApp>();
	app->setLaunchArguments(argc, argv);
	ofRunApp(win, app);
	ofRunMainLoop();
}
