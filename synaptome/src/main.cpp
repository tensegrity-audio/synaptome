#include "ofApp.h"
#include "ofMain.h"

int main(int argc, char** argv) {
	ofGLFWWindowSettings s;
	s.setGLVersion(3, 2);
	s.setSize(1280, 720);
	s.title = "Synaptome";
	auto win = ofCreateWindow(s);
	auto app = std::make_shared<ofApp>();
	app->setLaunchArguments(argc, argv);
	ofRunApp(win, app);
	ofRunMainLoop();
}
