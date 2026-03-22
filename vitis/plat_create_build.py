import argparse

import logging

import vitis
import os
import sys
import pathlib
import shutil
import zipfile

# --- Logging config ---
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

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
    "--xsa",
    help="Platform (XSA) file (example: path/to/plat.xsa)"
)

parser.add_argument(
    "--plat_name", default="platform",
    help="Name of the new platform project (default: platform)"
)

parser.add_argument(
    "--c0_name", default="cpu0_app",
    help="Name of the new core 0 project (default: cpu0_app)"
)

parser.add_argument(
    "--c1_name", default=None,
    help="Name of the new core 1 project (default: None)"
)

args = parser.parse_args()

xsa_file = args.xsa
platform_name = args.plat_name
cpu0_app_name = args.c0_name
cpu1_app_name = args.c1_name
ws = args.ws

# --- Workspace and projects ---
cpu0_name = 'cpu0'
cpu1_name = 'cpu1'

# Create the workspace
pathlib.Path(ws).mkdir(exist_ok=True)

# Start the client and sets the worskapce
client = vitis.create_client()
client.set_workspace(f"{ws}/")

# Checks if the there is any platform in the workspace
plats = client.list_platform_components().platformComponent

if plats == []:
    logger.info(f"No platform found in {ws}. Creating platform {platform_name} with XSA {xsa_file}.")
    platform = client.create_platform_component(name=platform_name, hw_design=xsa_file)

    fsbl = platform.get_domain('zynq_fsbl')

    platform.add_domain(name=cpu0_name, cpu='ps7_cortexa9_0', os='standalone')
    cpu0 = platform.get_domain(cpu0_name)

    if cpu1_app_name is not None:
        platform.add_domain(name=cpu1_name, cpu='ps7_cortexa9_1', os='standalone')
        cpu1 = platform.get_domain(cpu1_name)
    
    platform.build()
    platform_xpfm = client.find_platform_in_repos(platform_name)

    cpu0_app = client.create_app_component(name=cpu0_app_name, platform=platform_xpfm, domain=cpu0_name, template='hello_world')
    if cpu1_app_name is not None:
        cpu1_app = client.create_app_component(name=cpu1_app_name, platform=platform_xpfm, domain=cpu1_name, template='hello_world')
    else:
        cpu1_app = None
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

    cpu0_app = client.get_component(cpu0_app_name)
    if cpu1_app_name is not None:
        cpu1_app = client.get_component(cpu1_app_name)
    else:
        cpu1_app = None

# --- Build applications ---
cpu0_app.clean()
cpu0_app.build()

if cpu1_app_name is not None:
    cpu1_app.clean()
    cpu1_app.build()

vitis.dispose()
