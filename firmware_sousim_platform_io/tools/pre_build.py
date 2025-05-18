import subprocess
import os

from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()
git_hash = subprocess.check_output(["git", "rev-parse", "--short", "HEAD"]).strip().decode('utf-8')
env.Append(CPPDEFINES=[("GIT_HASH", '\\"{}\\"'.format(git_hash))])
print("Git hash:", git_hash)


dash_webpage_script = os.path.join(os.getcwd(), 'tools', 'html2cpp.py')
subprocess.run(['python', dash_webpage_script, "tools/dash_webpage.html",  "-o=.pio/libdeps/esp32s2ucp/ESP-dash/src/dash_webpage.h"], check=True)

directory_path = os.path.join(os.getcwd(), 'files')
conversion_script = os.path.join(os.getcwd(), 'tools', 'convert_files_to_headers.py')
subprocess.run(['python', conversion_script, directory_path], check=True)
