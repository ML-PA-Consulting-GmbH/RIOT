"""
Copyright (C) 2025 ML!PA Consulting GmbH

SPDX-License-Identifier: MIT

Authors:
    Daniel Lockau <daniel.lockau@ml-pa.com>
"""

from dataclasses import dataclass
import logging
import multiprocessing
import os
from pathlib import Path
import re
from typing import List, Tuple, Dict
from types import SimpleNamespace


__all__ = ['scanner_select', 'scanner_name', 'scan_files',
           'find_system_packages_for_files', 'FileScanResult']


_statics = SimpleNamespace()
_statics.scanners = {}
_statics.selected_scanner_name = 'scancode'

@dataclass
class FileScanResult:
    file_path: Path
    scan_success: bool
    licenses: List[str] | None
    spdx_license_identifier: str | None
    copyrights: List[str] | None
    authors: List[Tuple[str, str]] | None # (name, email)


def scanner_select(name: str):
    if name not in _statics.scanners:
        raise ValueError(f"Unknown scanner name: {name}")
    _statics.selected_scanner_name = name


def scanner_name():
    return _statics.selected_scanner_name


def get_relative_posix_paths(file_paths: List[Path]) -> List[Path]:
    return [Path(os.path.relpath(f.absolute().as_posix(), Path.cwd().absolute().as_posix()))
            for f in file_paths]


def _scancode_scan_file(pth: Path):
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
            package=False,
            system_package=False,
            generated=True,
            license_score=90,
            unknown_licenses=False)


def scan_files_with_scancode(
        file_paths: List[Path],
        n_procs=os.cpu_count()) -> Dict[Path, FileScanResult]:
    # the way scancode is built, we have to pass the files individually
    # and not as a list of paths
    logging.debug("Scanning files with scancode")
    author_matcher = re.compile(r'([^<]*)? *(<.*>)?')
    with multiprocessing.Pool(n_procs) as pool:
        file_scan_result = pool.starmap(
                _scancode_scan_file,
                [(str(pth), ) for pth in file_paths])
    scan_results = {}
    for pth, scan_result in file_scan_result:
        if not scan_result[0]:
            file_result = FileScanResult(
                    file_path=pth,
                    scan_success=False,
                    licenses=None,
                    copyrights=None,
                    spdx_license_identifier=None,
                    authors=None)
        else:
            file_result = FileScanResult(
                    file_path=pth,
                    scan_success=True,
                    licenses=None,
                    copyrights=None,
                    spdx_license_identifier=None,
                    authors=None)
            scan_result = scan_result[1]
            if (scan_result.get('files') and
                    scan_result.get('files')[0].get('copyrights')):
                file_result.copyrights = [
                        c['copyright']
                        for c in scan_result['files'][0]['copyrights']]
            if (scan_result.get('files') and
                    scan_result.get('files')[0].get('authors')):
                authors = []
                for c in scan_result['files'][0]['authors']:
                    m = author_matcher.match(c['author'])
                    if m:
                        name = m.group(1)
                        if name:
                            name = name.strip(" \t\"")
                        email = m.group(2)
                        if email:
                            email = email.strip(' \t<>')
                        authors.append((name, email))
                file_result.authors = authors
            if scan_result.get('license_detections'):
                file_result.licenses = [
                        l['license_expression_spdx']
                        for l in scan_result['license_detections']]
            if (scan_result.get('files') and
                    scan_result.get('files')[0]
                    .get('detected_license_expression_spdx')):
                file_result.spdx_license_identifier = (
                        scan_result
                        .get('files')[0]
                        .get('detected_license_expression_spdx')
                        .replace('\n', ''))
        scan_results[pth] = file_result
    return scan_results

_statics.scanners['scancode'] = scan_files_with_scancode


def scan_files(file_paths: List[Path],
               n_procs=os.cpu_count()) -> Dict[Path, FileScanResult]:
    return _statics.scanners[_statics.selected_scanner_name](
            file_paths,
            n_procs)

def find_system_packages_for_files(file_paths: List[Path]):

    pass

if __name__ == "__main__":
    import pprint
    pprinter = pprint.PrettyPrinter(indent=4).pprint
    scanner_select('scancode')
    scancode_input = [f
                                  for f in Path(__file__).parent.iterdir()
                                  if f.name.endswith('.py')]
    scancode_result = scan_files(
            scancode_input,
            n_procs=os.cpu_count())
    pprinter(scancode_result)
