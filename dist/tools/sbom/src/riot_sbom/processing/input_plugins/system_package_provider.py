"""
Copyright (C) 2025 ML!PA Consulting GmbH

SPDX-License-Identifier: MIT

Authors:
    Daniel Lockau <daniel.lockau@ml-pa.com>
"""

import os
import pathlib
import subprocess

def _find_system_package_for_file(file_path: pathlib.Path) -> str | None:
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
                return dpkg_query.stdout.split(":")[0]
        else:
            raise NotImplementedError(
                f"System package detection for this OS is not implemented. lsb_release output:\n{lsb_release_info.stdout}")
