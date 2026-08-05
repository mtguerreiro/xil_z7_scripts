import argparse
import json
import time
import pathlib
from dataclasses import dataclass, field

import xsdb

@dataclass
class CpuData:
    app         : str = ''
    elf         : str = ''

@dataclass
class ProgData:
    xsa         : str = ''
    platform    : str = ''
    bitstream   : str = ''
    fsbl        : str = ''
    cpu0        : None = None
    cpu1        : None = None


# --- Script arguments ---
parser = argparse.ArgumentParser(description="Programs a Zynq-7000 device")

parser.add_argument(
    "--ws", default="./build",
    help="Path to the workspace (default: ./build)"
)

parser.add_argument(
    "--config", default=None, required=True,
    help="JSON config file (default: None)"
)

args = parser.parse_args()
ws = args.ws
json_cfg = args.config

# --- Config file processing ---
with open(json_cfg, mode='r') as f:
    jdata = json.load(f)

data = ProgData()

# Gets xsa filename without the extesion
xsa_path = jdata.get('xsa')
xsa_fname = pathlib.Path(xsa_path).stem

print(f'XSA filename: {xsa_fname}')
data.xsa = xsa_fname
data.platform = jdata.get('plat_name')
    
if 'cpu0' in jdata:
    data.cpu0 = CpuData(app=jdata['cpu0']['app_name'])
    data.cpu0.elf = f'{ws}/{data.cpu0.app}/build/{data.cpu0.app}.elf'

if 'cpu1' in jdata:
    data.cpu1 = CpuData(app=jdata['cpu1']['app_name'])
    data.cpu1.elf = f'{ws}/{data.cpu1.app}/build/{data.cpu1.app}.elf'

data.bitstream = f'{ws}/{data.platform}/hw/sdt/{data.xsa}.bit'
data.fsbl = f'{ws}/{data.platform}/zynq_fsbl/build/fsbl.elf'

# --- Program ---
s = xsdb.start_debug_session()
s.connect()

s.targets("--set", filter="name =~ APU")
s.rst()
s.fpga(file=data.bitstream)

s.targets(2)
s.dow(data.fsbl)
time.sleep(0.25)
s.con()
time.sleep(0.25)
s.stop()
time.sleep(0.25)

if data.cpu0:
    s.dow(data.cpu0.elf)
    time.sleep(0.25)

if data.cpu1:
    s.targets(3)
    time.sleep(0.25)
    s.con()
    time.sleep(0.25)
    s.stop()
    time.sleep(0.25)
    s.dow(data.cpu1.elf)
    time.sleep(0.25)

if data.cpu0:
    s.targets(2)
    s.con()
    time.sleep(0.5)

if data.cpu1:
    s.targets(3)
    s.con()
