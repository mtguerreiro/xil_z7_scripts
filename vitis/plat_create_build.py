import argparse
import logging

import vitis
import os
import sys
import pathlib
import shutil
import zipfile

import json
from dataclasses import dataclass, field

@dataclass
class AppData:
    processor   : str = ''
    domain_name : str = ''
    domain      : str = ''
    app         : str = ''
    app_name    : str = 'app'
    os          : str = 'standalone'
    template    : str = 'hello_world'
    cmake       : dict = field(default_factory=dict)
    options     : dict = field(default_factory=dict)


# --- Logging config ---
logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

if not logger.handlers:
    handler = logging.StreamHandler()
    formatter = logging.Formatter('%(name)s - %(levelname)s - %(message)s')
    handler.setFormatter(formatter)
    logger.addHandler(handler)
        
# --- Script arguments ---
parser = argparse.ArgumentParser(description="Creates and builds a Zynq-7000 project")

parser.add_argument(
    "--ws", default="./build",
    help="Path to the workspace (default: ./build)"
)

parser.add_argument(
    "--config", default=None,
    help="JSON config file (default: None)"
)

args = parser.parse_args()
ws = args.ws
json_cfg = args.config

cpu0 = None
cpu1 = None

# --- Config file processing ---
if json_cfg:
    with open(json_cfg, mode='r') as f:
        jdata = json.load(f)

    xsa_file = jdata.get('xsa')
    platform_name = jdata.get('plat_name')

    if 'cpu0' in jdata:
        cpu0 = AppData(**jdata['cpu0'])
        cpu0.domain_name = 'cpu0'
        cpu0.processor = 'ps7_cortexa9_0'

    if 'cpu1' in jdata:
        cpu1 = AppData(**jdata['cpu1'])
        cpu1.domain_name = 'cpu1'
        cpu1.processor = 'ps7_cortexa9_1'

cpus = [cpu0, cpu1]

# --- Process cmake options ---
def process_cpu_cmake(cpu, ws):
    if 'config' in cpu.cmake:
        pathlib.Path(f"{ws}/{cpu.app_name}/src/UserConfig.cmake").unlink()
        f = shutil.copy(f"{cpu.cmake['config']}", f'{ws}/{cpu.app_name}/src')
        pathlib.Path(f).rename(f"{ws}/{cpu.app_name}/src/UserConfig.cmake")

    add_text = ""
    if 'path_vars' in cpu.cmake:
        add_text += "\n"
        for v, p in cpu.cmake['path_vars'].items():
            path = pathlib.Path(p)
            p = p if path.is_absolute() is True else path.resolve()
            add_text += f"set({v} \"{p}\")\n"

    if 'include' in cpu.cmake:
        add_text += "\n"
        add_text += f"include(\"{cpu.cmake['include']}\")\n"

    if add_text != "":
        with open(f"{ws}/{cpu.app_name}/src/UserConfig.cmake", "r") as f:
            lines = f.readlines()

        with open(f"{ws}/{cpu.app_name}/src/UserConfig.cmake", "w") as f:
            end_section = "###   END OF USER SETTINGS SECTION ###"
            for line in lines:
                if end_section in line:
                    f.write(f"{add_text}\n")
                f.write(line)


# --- Process params ---
def process_os_params(domain, params):
    for p, v in params.items():
        domain.set_config(option='os', param=p, value=v)

def process_lib_params(domain, libs):
    curr_libs = {l['name'] for l in domain.get_libs()}
    for lib in libs:
        if lib not in curr_libs:
            domain.set_lib(lib)
        for p, v in libs[lib].items():
            domain.set_config(option='lib', param=p, value=v, lib_name=lib)

# --- Workspace and projects ---
cpu0_name = 'cpu0'
cpu1_name = 'cpu1'

# Create the workspace
pathlib.Path(ws).mkdir(exist_ok=True)

# Start the client and set the worskapce
client = vitis.create_client()
client.set_workspace(f"{ws}/")

# Check if the there is any platform in the workspace
plats = client.list_platform_components().platformComponent

if plats == []:
    logger.info(f"No platform found in {ws}. Creating platform {platform_name} with XSA {xsa_file}.")
    platform = client.create_platform_component(name=platform_name, hw_design=xsa_file)

    fsbl = platform.get_domain('zynq_fsbl')

    for cpu in cpus:
        if cpu is None:
            continue
                    
        platform.add_domain(name=cpu.domain_name, cpu=cpu.processor, os=cpu.os)
        cpu.domain = platform.get_domain(cpu.domain_name)

        if cpu.options:
            if 'os' in cpu.options:
                process_os_params(cpu.domain, cpu.options['os'])

            if 'lib' in cpu.options:
                process_lib_params(cpu.domain, cpu.options['lib'])
    
    platform.build()
    platform_xpfm = client.find_platform_in_repos(platform_name)

    for cpu in cpus:
        if cpu is None:
            continue

        cpu.app = client.create_app_component(name=cpu.app_name, platform=platform_xpfm, domain=cpu.domain_name, template=cpu.template)
        if cpu.cmake:
            process_cpu_cmake(cpu, ws)

else:
    # Loads the existing platform
    platform_path = plats[0].platform_location
    platform_name = plats[0].platform_name

    logger.info(f"Building existing platform {platform_name} found in {ws}")
    if xsa_file:
        logger.warning(f"XSA file {xsa_file} given but it will be ignored since platform {platform_name} was found in {ws}. To update the XSA file delete the workspace and run this script again.")
          
    client.add_platform_repos(platform_path)
    platform = client.get_component(platform_name)
    platform.build()

    for cpu in cpus:
        if cpu is None:
            continue

        cpu.domain = platform.get_domain(cpu.domain_name)
        cpu.app = client.get_component(cpu.app_name)
        if cpu.options:
            if 'os' in cpu.options:
                process_os_params(cpu.domain, cpu.options['os'])

            if 'lib' in cpu.options:
                process_lib_params(cpu.domain, cpu.options['lib'])

# --- Build applications ---
for cpu in cpus:
    if cpu is None:
        continue
    cpu.app.clean()
    cpu.app.build()

vitis.dispose()
