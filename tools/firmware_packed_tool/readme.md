# 🛠️ LF-OpenBMC Firmware Pack Tool

A Python command-line utility for generating PLDM-compliant firmware update packages.

This tool:
- Updates metadata in a JSON template
- Injects the correct `compatible` string and firmware `version`
- Invokes the `pldm_fwup_pkg_creator.py` script to create the firmware package

---

## 📦 Features

- Supports multiple device types (CPLD, BIOS, VR, Retimer, etc.)
- Validates input firmware file and compatible string
- Automatically names the output `.pldm` package if not specified

---

## 📦 Installation

Before running the tool, make sure required Python dependencies are installed:

```bash
python3 -m pip install bitarray
```

---

## 🧾 Usage

```bash
python3 packed_tool.py -c <compatible> -i <input_file> [-v <version>] [-o <output_file>]
```
---

## Required arguments
-c, --compatible: Target device identifier. Supported values include:
- harma-mb-cpld
- harma-scm-cpld
- harma-aegis-cpld
- harma-bios
- harma-isl-vcore0
- harma-isl-vcore1
- harma-isl-pvdd11
- harma-xdpe-vcore0
- harma-xdpe-vcore1
- harma-xdpe-pvdd11
- harma-retimer

-i, --input: Path to the input binary firmware file.

Optional arguments
-v, --version: Firmware version string (default: 1.0.0)

-o, --output: Custom output filename (default: <compatible>_<version>.pldm)

## 🔧 Example

``` bash
python3 packed_tool.py \
  -c harma-mb-cpld \
  -i my_cpld_firmware.bin \
  -v 2.1.0
```

This will:
Set the version to 2.1.0 in the metadata
Embed the correct compatible string for harma-mb-cpld
Produce the package: harma-mb-cpld_2.1.0.pldm
