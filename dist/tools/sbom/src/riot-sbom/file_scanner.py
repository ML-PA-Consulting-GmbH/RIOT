# SPDX-License-Identifier: MIT

import os
from pathlib import Path
from typing import List
from types import SimpleNamespace
import logging


__all__ = ['scanner_select', 'scanner_name',
           'scan_files']


_statics = SimpleNamespace()
_statics.scanners = {}
_statics.selected_scanner_name = 'scancode'


def scanner_select(name: str):
    if name not in _statics.scanners:
        raise ValueError(f"Unknown scanner name: {name}")
    _statics.selected_scanner_name = name


def scanner_name():
    return _statics.selected_scanner_name


def get_relative_posix_paths(file_paths: List[Path]) -> List[Path]:
    return [Path(os.path.relpath(f.absolute().as_posix(), Path.cwd().absolute().as_posix()))
            for f in file_paths]


def _scancode_scan_file(pth: Path, include_package_matching=False):
    from scancode.cli import run_scan as scancode_run_scan
    return pth, scancode_run_scan(
            pth,
            strip_root=False,
            full_root=True,
            processes=-1,
            quiet=True,
            verbose=False,
            keep_temp_files=False,
            return_results=True,
            return_codebase=False,
            max_depth=1,
            copyright=True,
            license=True,
            email=False,
            url=False,
            package=include_package_matching,
            system_package=include_package_matching,
            generated=True,
            license_score=90,
            unknown_licenses=False)


def scan_files_with_scancode(
        file_paths: List[Path],
        n_procs=os.cpu_count(),
        include_package_matching=False):
    # the way scancode is built, we have to pass the files individually
    # and not as a list of paths
    import multiprocessing
    logging.debug("Scanning files with scancode")
    with multiprocessing.Pool(n_procs) as pool:
        file_scan_result = pool.starmap(
                _scancode_scan_file,
                [(str(pth), include_package_matching) for pth in file_paths])
    scan_results = dict(file_scan_result)
    return scan_results

_statics.scanners['scancode'] = scan_files_with_scancode


def scan_files(file_paths: List[Path],
               n_procs=os.cpu_count(),
               include_package_matching=False):
    return _statics.scanners[_statics.selected_scanner_name](
            file_paths,
            n_procs,
            include_package_matching)


if __name__ == "__main__":
    import json

    logging.basicConfig(level=logging.DEBUG)
    scanner_select('scancode')
    scancode_input = [f
                                  for f in Path(__file__).parent.iterdir()
                                  if f.name.endswith('.py')]
    scancode_input = [f
                                  for f in Path('/home/daniel_lockau/src/rdpd-dft-rtos/modules/mlpa_self_test').iterdir()
                                  if f.name.endswith('.c') or f.name.endswith('.h')]
    scancode_result = scan_files(
            scancode_input,
            n_procs=os.cpu_count(),
            include_package_matching=False)
    json_conv_result = {str(k): v for k, v in scancode_result.items()}
    print(json.dumps(json_conv_result, sort_keys=False, indent=2))
