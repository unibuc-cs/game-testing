### Installation

You can test locally the snippets of code provided by installing:
-	Pybind11, Python 3.7.
Preferably download the PDBs for your version and let Visual Studio know about them such that you can debug crashes etc.
-	Tesseract for Windows (put the path of the executable in PATH).
-	Install the following packages for python using pip:  PyTesseract, opencv2, opencv2-contrib, json, json pickler, flask, requests.

### How to test the Python code only

-	Start  with LayerUtils.py file by running its main functions. If everything works, go to the next level.
-	Start the main_flask_server.py first, then main_flask_client. If everything works, it means that Python codes are correct at the communication level.
-	Start the main_flask_server.py and, as client this time, run the TestApp.
