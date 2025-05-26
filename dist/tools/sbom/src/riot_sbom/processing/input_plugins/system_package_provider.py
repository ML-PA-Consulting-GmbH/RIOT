"""
Copyright (C) 2025 ML!PA Consulting GmbH

SPDX-License-Identifier: MIT

Authors:
    Daniel Lockau <daniel.lockau@ml-pa.com>
"""

if __name__ == "__main__":
    # update search path for local testing
    import sys
    import pathlib
    sys.path.insert(0,pathlib.Path(__file__).absolute().parents[3].as_posix())

import logging
import os
import pathlib
import subprocess
from typing import Dict

from riot_sbom.processing.plugin_type import Plugin
from riot_sbom.data.package_info import PackageInfo, PackageReference
from riot_sbom.data.app_info import AppInfo

def _find_system_package_for_file(file_path: pathlib.Path) -> Dict[str, str] | None:
    """
    Finds the system package for a given file path, if any.
    """
    if os.name == "nt":
        raise NotImplementedError("Windows is not supported yet")
    elif os.name == "posix":
        lsb_release_info = subprocess.run(
            ["lsb_release", "-a"],
            capture_output=True,
            text=True,
            check=False,
        )
        if lsb_release_info.returncode != 0:
            raise RuntimeError("lsb_release command failed")
        if "Ubuntu" in lsb_release_info.stdout:
            # For Ubuntu, use dpkg to find the package
            dpkg_query = subprocess.run(
                ["dpkg", "-S", str(file_path)],
                capture_output=True,
                text=True,
                check=False,
            )
            if dpkg_query.returncode == 0:
                # TODO: add more info
                return {'name': dpkg_query.stdout.split(":")[0], 'supplier': 'Ubuntu'}
        else:
            raise NotImplementedError(
                f"System package detection for this OS is not implemented. lsb_release output:\n{lsb_release_info.stdout}")

class SystemPackageProvider(Plugin):
    def get_name(self):
        return "system-package-provider"

    def get_description(self):
        return "Will attempt to provide system package references for source files with no package relation."

    def run(self, app_info: AppInfo, _):
        for file in app_info.files:
            if not file.package and file.path.exists():
                system_package = _find_system_package_for_file(file.path)
                if system_package:
                    logging.debug(f"Found system package '{system_package}' for file: {file.path}")
                    package_ref = PackageReference(system_package['name'])
                    if package_ref not in app_info.packages:
                        app_info.packages[package_ref] = PackageInfo(
                            name=system_package['name'],
                            version=None,
                            supplier=system_package['supplier'],
                            authors=None,
                            source_dir=None,
                            download_url=None,
                            licenses=None,
                            copyrights=None,
                        )
                    file.package = package_ref
                else:
                    logging.debug(f"No system package found for file: {file.path}")

# TODO unittest
