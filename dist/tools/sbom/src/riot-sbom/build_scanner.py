#!/usr/bin/env python3

import os
import json
import logging
import pathlib
import re
import subprocess
import tempfile
import unittest

class BuildScanner(object):
    def __init__(self, app_dir: pathlib.Path):
        self.__app_dir = app_dir
        self.__app_data = None
        self.__riot_data = None
        self.__external_module_data = None
        self.__package_data = None
        self.__file_data = None

    def run(self):
        sbom_input = BuildScanner._run_sbom_input(self.__app_dir.as_posix())
        self.__app_data = sbom_input['application']
        self.__riot_data = sbom_input['riot']
        self.__external_module_data = sbom_input['external_modules']
        with tempfile.TemporaryDirectory() as tempdir:
            trace_file = os.path.join(tempdir, 'trace.log')
            BuildScanner._run_traced_build(self.__app_dir.as_posix(),
                                           trace_file)
            file_paths = BuildScanner._parse_trace_file(trace_file)
        self.__package_data = BuildScanner._complete_package_data(
            sbom_input, file_paths)
        self.__file_data = BuildScanner._build_file_data(
            sbom_input, file_paths, self.package_data)

    def get_prop(self, prop_ref, prop_name):
        if prop_ref is None:
            raise RuntimeError(f'Scanner must be run before retrieving "{prop_name}"')
        return prop_ref

    app_data = property(lambda self: self.get_prop(self.__app_data, 'app_data'))
    riot_data = property(lambda self: self.get_prop(self.__riot_data, 'riot_data'))
    external_module_data = property(lambda self: self.get_prop(self.__external_module_data, 'external_module_data'))
    package_data = property(lambda self: self.get_prop(self.__package_data, 'package_data'))
    file_data = property(lambda self: self.get_prop(self.__file_data, 'file_data'))

    @staticmethod
    def _run_sbom_input(app_dir):
        logging.info('Retrieving package information for build')
        pkg_info_run = subprocess.run(['make', 'info-sbom-input'],
                                      cwd=app_dir,
                                      stdout=subprocess.PIPE,
                                      stderr=subprocess.PIPE)
        if pkg_info_run.returncode != 0:
            raise RuntimeError('Failed to run make info-sbom')
        output = pkg_info_run.stdout.decode('utf-8')
        start_marker = ">>>> BEGIN SBOM INPUT"
        end_marker = "<<<< END SBOM INPUT"
        start_idx = output.find(start_marker)
        end_idx = output.find(end_marker)
        if start_idx == -1 or end_idx == -1:
            raise RuntimeError('Failed to find SBOM input markers')
        sbom_input = output[start_idx + len(start_marker):end_idx]
        pkg_info = json.loads(sbom_input)
        return pkg_info

    @staticmethod
    def _run_traced_build(app_dir: str, trace_file: str):
        logging.info('Retrieving file information for build (this may take a while)')
        clean_cmd = ["make", "-j", "clean"]
        clean_run = subprocess.run(clean_cmd,
                cwd=app_dir,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE)
        if clean_run.returncode != 0:
            raise RuntimeError(f'Failed to run make clean (stderr="{clean_run.stderr.decode("utf-8")}")')
        build_cmd = ["strace", "-f",
                    "-e", "trace=openat,open",
                    "-e", "quiet=attach,exit,path-resolution,personality,thread-execve,superseded",
                    "-o", f"{trace_file}", "make", "-j"]
        trace_run = subprocess.run(build_cmd,
                cwd=app_dir,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE)
        if trace_run.returncode != 0:
            raise RuntimeError(f'Failed to run make trace-build (stderr="{trace_run.stderr.decode("utf-8")}")')

    @staticmethod
    def _parse_trace_file(trace_file: str):
        logging.info('Parsing trace file')
        file_paths = set()
        path_matcher = re.compile(r'[^"]*"(.*)"[^"]*')
        file_matcher = re.compile(r'.*\.(h|hpp|c|cpp|hxx|cxx|c\+\+|s|asm)$')
        with open(trace_file, 'r') as f:
            for line in f:
                if 'openat(' in line or "open(" in line:
                    match = path_matcher.match(line)
                    if match:
                        path = match.group(1)
                        if file_matcher.match(path):
                            path = str(pathlib.Path(path).resolve())
                            # ignore temporary files and only consider regular files
                            if (not path.startswith('/tmp/')
                                and os.path.isfile(path)):
                                file_paths.add(path)
        return sorted(file_paths)

    @staticmethod
    def _complete_package_data(sbom_input, file_paths: list):
        logging.info('Completing package data')
        package_data = [pkg for pkg in sbom_input['packages']]
        unresolved_packages = set()
        for file in file_paths:
            if (
                file.startswith(sbom_input['application']['source_dir'])
                or file.startswith(sbom_input['application']['build_dir'])
                or file.startswith(sbom_input['riot']['source_dir'])
                or any(file.startswith(package['source_dir'])
                       for package in package_data)
                or any(file.startswith(ext_mod['source_dir'])
                       for ext_mod in sbom_input['external_modules'])
                ):
                continue
            # package not known, try to infer from path
            # NOTE this is an ugly temporary solution
            packag_reg = [
                # TODO complete the information below
                {
                    'name': 'picolibc',
                    'matcher': re.compile(r'^(.*/picolibc)/.*$'),
                    'url': "https://github.com/picolibc/picolibc",
                    'version': None,
                    'license': None # file dependent
                },
                {
                    'name': 'newlib',
                    'matcher': re.compile(r'^(.*/newlib)/.*$'),
                    'url': "https://sourceware.org/newlib/",
                    'version': None,
                    'license': None # file dependent
                },
                {
                    'name': 'libgcc',
                    'matcher': re.compile(r'^(.*/lib/gcc)/.*$'),
                    'url': "https://gcc.gnu.org/",
                    'version': None,
                    'license': "GPL-3.0-or-later WITH GCC-exception-3.1"
                },
                {
                    'name': 'clang',
                    'matcher': re.compile(r'^(.*/lib/clang).*$'),
                    'url': "https://clang.llvm.org/",
                    'version': None,
                    'license': "Apache-2.0 WITH LLVM-exception"
                },
                {
                    'name': 'llvm',
                    'matcher': re.compile(r'^(.*/lib/llvm).*$'),
                    'url': "https://llvm.org/",
                    'version': None,
                    'license': "Apache-2.0 WITH LLVM-exception"
                }
            ]
            file_has_pkg = False
            for reg in packag_reg:
                match = reg['matcher'].match(file)
                if match:
                    package_data.append({
                        'name': reg['name'],
                        'source_dir': match.group(1),
                        'url': reg['url'],
                        'version': reg['version'],
                        'license': reg['license']
                    })
                    file_has_pkg = True
                    break
            if not file_has_pkg:
                unresolved_packages.add(os.path.dirname(file))
        if unresolved_packages:
            logging.warning(f'Unresolved packages for files in: {unresolved_packages}')
        return package_data

    @staticmethod
    def _build_file_data(sbom_input, file_paths, package_data: list):
        logging.info('Building file data')
        file_data = []
        for file_path in file_paths:
            file_info = BuildScanner._get_file_info(file_path, sbom_input, package_data)
            file_data.append(file_info)
        return file_data

    @staticmethod
    def _get_file_info(file_path, sbom_input, package_data):
        file_info = {
            'path': file_path,
            'package': 'unknown'
        }
        if file_path.startswith(sbom_input['riot']['source_dir']):
            file_info['package'] = "RIOT OS"
        # package data overrides RIOT data
        for package in package_data:
            if file_path.startswith(package['source_dir']):
                file_info['package'] = package['name']
                break
        for ext_mod in sbom_input['external_modules']:
            if file_path.startswith(ext_mod['source_dir']):
                file_info['package'] = ext_mod['name']
                break
        if file_path.startswith(sbom_input['application']['source_dir']):
            file_info['package'] = sbom_input['application']['name']
        return file_info

class BuildScannerTest(unittest.TestCase):

    def test_run(self):
        riot_dir = pathlib.Path(__file__).parents[5]
        app_dir = riot_dir.joinpath('tests', 'net', 'nanocoap_cli')
        scanner = BuildScanner(app_dir)
        self.assertRaises(RuntimeError, lambda: scanner.app_data)
        self.assertRaises(RuntimeError, lambda: scanner.riot_data)
        self.assertRaises(RuntimeError, lambda: scanner.external_module_data)
        self.assertRaises(RuntimeError, lambda: scanner.package_data)
        self.assertRaises(RuntimeError, lambda: scanner.file_data)
        old_board = os.environ.get('BOARD')
        os.environ['BOARD'] = 'native64'
        scanner.run()
        if old_board is not None:
            os.environ['BOARD'] = old_board
        else:
            del os.environ['BOARD']
        self.assertEqual(scanner.app_data['name'], 'tests_nanocoap_cli')
        self.assertIsInstance(scanner.riot_data, dict)
        self.assertEqual(len(scanner.external_module_data), 0)
        self.assertGreater(len(scanner.package_data), 0)
        self.assertGreater(len(scanner.file_data), 0)

if __name__ == '__main__':
    unittest.main()
