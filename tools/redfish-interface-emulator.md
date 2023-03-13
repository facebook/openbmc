How to use the Redfish Emualtor tarball

Extract the tarball to a laptop (may also work on devserver)
Start a Python virtual environment (venv)
Change to the root of the Redfish-Interface-Emulator directory and execute

   python3 emulator.py

If the underlying JSON files in the api_emulator/redfish/static directory are
valid, the following text should be presented.

INFO:root:Init ResourceDictionary.
* Use HTTP
* Running in Redfish mode
* Serving Flask app 'g'
* Debug mode: off
INFO:werkzeug:WARNING: This is a development server. Do not use it in a production deployment. Use a production WSGI server instead.
* Running on all addresses (0.0.0.0)
* Running on http://127.0.0.1:5000
* Running on http://100.108.64.43:5000
INFO:werkzeug:Press CTRL+C to quit


To access the web service which was started, open a browser window and browse to http://127.0.0.1:5000 to access the root of
the Redfish tree.

If any errors are found in the JSON files in the static directory, then an error indicating which files have errors are shown
in red.

