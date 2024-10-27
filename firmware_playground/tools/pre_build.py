import subprocess
import os

directory_path = os.path.join(os.getcwd(), 'files')
conversion_script = os.path.join(os.getcwd(), 'tools', 'convert_files_to_headers.py')
subprocess.run(['python', conversion_script, directory_path], check=True)


dash_webpage_script = os.path.join(os.getcwd(), 'tools', 'html2cpp.py')


subprocess.run(['python', dash_webpage_script, "tools/dash_webpage.html",  "-o=.pio/libdeps/esp32s2ucp/ESP-dash/src/dash_webpage.h"], check=True)



