# SPDX-License-Identifier: MIT

import os
from pathlib import Path
from typing import List

from scancode.cli import run_scan as scancode_run_scan

def get_relative_posix_paths(file_paths: List[Path]) -> List[Path]:
    return [Path(os.path.relpath(f.absolute().as_posix(), Path.cwd().absolute().as_posix()))
            for f in file_paths]

def scan_files(
        file_paths: List[Path],
        n_procs=4,
        include_package_matching=False):
    scan_result = scancode_run_scan(
            get_relative_posix_paths(file_paths),
            strip_root=False,
            full_root=True,
            processes=n_procs,
            quiet=True,
            verbose=False,
            keep_temp_files=False,
            return_results=True,
            return_codebase=False,
            max_depth=0,
            copyright=True,
            license=True,
            email=True,
            url=True,
            package=include_package_matching,
            system_package=include_package_matching,
            generated=True,
            license_score=90)
    return scan_result

if __name__ == "__main__":
    import json
    import sys

    print("scancode results on this package:", file=sys.stderr)
    scancode_input = [f
                                  for f in Path(__file__).parent.iterdir()
                                  if f.name.endswith('.py')]
    scancode_input = [f
                                for f in Path('/usr/include/clang/18.1.3/include/').iterdir()
                                if f.name.endswith('.h')]
    print("Files to scan:", get_relative_posix_paths(scancode_input),
          file=sys.stderr)
    scancode_result = scan_files(
            scancode_input,
            include_package_matching=True)
    print(json.dumps(scancode_result, sort_keys=False, indent=2))
