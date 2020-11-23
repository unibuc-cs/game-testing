// TestApp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <pybind11/embed.h>
namespace py = pybind11;
using namespace py::literals;
using namespace  std;

// Put here the paths to the Screen shot sample and package module. TODO: should get through an env variable instead
static const char* PATH_TO_A_SCREENSHOT_SAMPLE = "C:/Users/Ciprian/OneDrive - University of Bucharest, Faculty of Mathematics and Computer Science/WORK_PROGRESS/Ubi/GameStateChecker/unitTestResources/p1.png";
static const char* PATH_TO_PACKAGE_MODULE = "C:/Users/Ciprian/OneDrive - University of Bucharest, Faculty of Mathematics and Computer Science/WORK_PROGRESS/Ubi/GameStateChecker";

class CPPLayerToGameStateChecker
{
public:
    void init()
    {
        py::module sys = py::module::import("sys");
    	
        /* // Just a small thing to get my own libs instead of the systems - IF NEEDED
		sys.attr("path").attr("insert")(0, "C:/Users/Ciprian/Anaconda3");
		sys.attr("path").attr("insert")(0, "C:/Users/Ciprian/Anaconda3/Lib");
		sys.attr("path").attr("insert")(0, "C:/Users/Ciprian/Anaconda3/DLLs");
		sys.attr("path").attr("insert")(0, "C:/Users/Ciprian/Anaconda3/Lib/site-packages");
		sys.attr("path").attr("insert")(0, "C:/Users/Ciprian/Anaconda3/Lib/site-packages/win32");
		sys.attr("path").attr("insert")(0, "C:/Users/Ciprian/Anaconda3/Lib/site-packages/win32/lib");
		sys.attr("path").attr("insert")(0, "C:/Users/Ciprian/Anaconda3/Lib/site-packages/pythonwin");
		    py::print(sys.attr("path"));
		*/

    	// Inject programatically where the package was installed
        sys.attr("path").attr("insert")(0, PATH_TO_PACKAGE_MODULE);
       // py::print(sys.attr("path"));
    	    	
    	// Import the module
        py::module flask_client = py::module::import("main_flask_client");
        py::object checkerClientClass = flask_client.attr("GameStateChecker_Client");

    	// Get and cache function addresses
        m_visuals_func_check = checkerClientClass.attr("check_visuals_onScreenshot");
    }

	// TODO: now these functions use a full path to the screenshot image, but can easily use a full byte code, it was easier to test like this instead
	void checkWeaponCrossPresence(const char* pathToScreenShot)
    {
    	// Fill parameters and context for test
        py::dict testContext = py::dict("requestCategory"_a = "UI", "requestFunc"_a = "checkWeaponCross", "screenshotsCount"_a = 1);
        py::dict expectedAnswer = py::dict("boolResult"_a = "True");

    	// Get back the result and output it
        std::string result = py::cast<std::string>(m_visuals_func_check(pathToScreenShot, testContext, expectedAnswer));
        cout << "Test result output: " << result << endl;
    }

    // Check if ammo text displayed on screen in sync with the game information.
    void checkIsAmmoVisualInSyncWithCode(const char* pathToScreenShot)
    {
        // Fill parameters and context for test
        py::list bbox_obj(4); //  Bbox where should the text appear on screen
        bbox_obj[0] = 912; bbox_obj[1] = 1015; bbox_obj[2] = 79; bbox_obj[3] = 49;
    
		py::dict testContext = py::dict("requestCategory"_a = "UI", "requestFunc"_a = "checkAmmoSyncText", "screenshotsCount"_a = 1,
										"bbox"_a = bbox_obj, 
										"textColorValueInHSV_min"_a = 129, //  # What color range are we expecting for text to have in the HSV space ?
										"textColorValueInHSV_max"_a = 130);

        py::dict expectedAnswer = py::dict("intResult"_a = 50); // How much should the ammo be on the screen ?
        
        // Get back the result and output it
        std::string result = py::cast<std::string>(m_visuals_func_check(pathToScreenShot, testContext, expectedAnswer));
        cout << "Test result output: " << result << endl;    	
    }
        
private:

	// Cache here all the functions for UI, animations, etc..
    py::object m_visuals_func_check;
};

int main()
{
    py::scoped_interpreter guard{};
	
 
	// Checks at runtime:
    CPPLayerToGameStateChecker cppCheckLayer;
    cppCheckLayer.init();

	/// Later, in the game loop...
    while(true)
    {
        const char* pathToScreenshot = PATH_TO_A_SCREENSHOT_SAMPLE;
        cppCheckLayer.checkWeaponCrossPresence(pathToScreenshot);
        cppCheckLayer.checkIsAmmoVisualInSyncWithCode(pathToScreenshot);

    	//....

        break;
    }
	
	return 0;
}

