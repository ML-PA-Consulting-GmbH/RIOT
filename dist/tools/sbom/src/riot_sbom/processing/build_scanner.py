"""
Copyright (C) 2025 ML!PA Consulting GmbH

SPDX-License-Identifier: MIT

Authors:
    Daniel Lockau <daniel.lockau@ml-pa.com>
"""

import os
import json
import logging
import pathlib
import re
import subprocess
import tempfile
import unittest

from ..data.app_info import AppInfo
from ..data.package_info import PackageInfo
from ..data.file_info import FileInfo
from ..data.checked_url import CheckedUrl
from ..data.license_info import LicenseInfo, LicenseDeclarationType

__all__ = ["BuildScanner"]

class BuildScanner(object):
    def __init__(self, app_dir: pathlib.Path):
        """
        Initialize the build scanner with the given application directory.
        :param app_dir: The path to the application directory.
        :type app_dir: pathlib.Path
        """
        self.__app_dir = app_dir
        self.__app_data = None
        self.__riot_data = None
        self.__external_module_data = None
        self.__package_data = None
        self.__file_data = None

    def __get_prop(self, prop_ref, prop_name):
        if prop_ref is None:
            raise RuntimeError(f'Scanner must be run before retrieving "{prop_name}"')
        return prop_ref

    app_data = property(lambda self: self.__get_prop(self.__app_data, 'app_data'))
    riot_data = property(lambda self: self.__get_prop(self.__riot_data, 'riot_data'))
    board_data = property(lambda self: self.__get_prop(self.__board_data, 'board_data'))
    external_module_data = property(lambda self: self.__get_prop(self.__external_module_data, 'external_module_data'))
    package_data = property(lambda self: self.__get_prop(self.__package_data, 'package_data'))
    file_data = property(lambda self: self.__get_prop(self.__file_data, 'file_data'))

    def run(self):
        """
        Executes the build scanning process.
        This method performs the following steps:
        1. Runs the SBOM input process to gather application, RIOT, and external module data.
        2. Creates a temporary directory to store the trace log file.
        3. Runs the traced build process and parses the trace file to get file paths.
        4. Builds the application information data using the SBOM input, file paths, and package data.
        """
        sbom_input = BuildScanner._run_sbom_input(self.__app_dir.as_posix())
        self.__app_data = sbom_input['application']
        self.__riot_data = sbom_input['riot']
        self.__board_data = sbom_input['board']
        self.__external_module_data = sbom_input['external_modules']
        names = set()
        names.add(self.__app_data['name'])
        names.add(self.__board_data['name'])
        names.update([mod['name'] for mod in self.__external_module_data])
        if len(names) != len(self.__external_module_data) + 2:
            logging.info('Duplicate names found in set of application, board and external modules. Prepending board name to application name.')
            self.__app_data['name'] = f"APPLICATION_{self.__app_data['name']}"
            self.__board_data['name'] = f"BOARD_{self.__board_data['name']}"
        with tempfile.TemporaryDirectory() as tempdir:
            trace_file = os.path.join(tempdir, 'trace.log')
            BuildScanner._run_traced_build(self.__app_dir.as_posix(),
                                           trace_file)
            file_paths = BuildScanner._parse_trace_file(trace_file)
        self.__file_data = BuildScanner._build_file_data(
            sbom_input, file_paths, self.package_data)

    def get_app_info(self) -> AppInfo:
        """
        Build and return an AppInfo object which can be used in follow-up
        processing steps.

        :return: A dictionary containing the application information.
        :rtype: AppInfo
        """
        app_info = AppInfo(
            build_dir=self.app_data['build_dir'],
            app_package=PackageInfo(
                name=self.app_data['name'],
                source_dir=self.app_data['source_dir'],
                version=None,
                licenses=None,
                download_url=None,
                copyrights=None,
                authors=None,
                supplier=None
            ),
            riot_package=PackageInfo(
                name=self.riot_data['name'],
                source_dir=self.riot_data['source_dir'],
                version=self.riot_data['version'],
                licenses=[LicenseInfo(name=self.riot_data['license'],
                    declaration_type=LicenseDeclarationType.EXACT_REFERENCE,
                    text=None,
                    url=None)],
                download_url=CheckedUrl(self.riot_data['url']),
                copyrights=None,
                authors=None,
                supplier=None
            ),
            board_package=PackageInfo(
                name=self.board_data['name'],
                source_dir=self.board_data['source_dir'],
                version=None,
                licenses=None,
                download_url=None,
                copyrights=None,
                authors=None,
                supplier=None),
            packages=[],
            files=[]
        )
        app_info.packages.append(app_info.app_package)
        app_info.packages.append(app_info.riot_package)
        app_info.packages.append(app_info.board_package)
        for ext_mod in self.external_module_data:
            app_info.packages.append(PackageInfo(
                name=ext_mod['name'],
                source_dir=ext_mod['source_dir'],
                version=None,
                licenses=None,
                download_url=None,
                copyrights=None,
                authors=None,
                supplier=None
            ))
        for ext_pkg in self.package_data:
            app_info.packages.append(PackageInfo(
                name=ext_pkg['name'],
                source_dir=ext_pkg['source_dir'],
                version=ext_pkg['version'],
                licenses=[LicenseInfo(name=ext_pkg['license'],
                                      declaration_type=LicenseDeclarationType.EXACT_REFERENCE,
                                      text=None,
                                      url=None)],
                download_url=CheckedUrl(ext_pkg['url']),
                copyrights=None,
                authors=None,
                supplier=None
            ))
        for file in self.file_data:
            app_info.files.append(FileInfo(
                path=file['path'],
                package=file['package'],
                licenses=None,
                copyrights=None,
                authors=None))
        return app_info

    @staticmethod
    def _run_sbom_input(app_dir):
        """
        Retrieve package information for build by running 'make info-sbom-input' in the specified application directory.
        This function runs a subprocess to execute the 'make info-sbom-input' command in the given directory,
        captures its output, and extracts the SBOM (Software Bill of Materials) input from the output.

        :param str app_dir: The directory where the 'make info-sbom-input' command should be executed.
        :returns: A dictionary containing the package information extracted from the SBOM input.
        :rtype: dict
        :raises RuntimeError: If the 'make info-sbom-input' command fails or if the SBOM input markers are not found in the output.
        """
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
        return json.loads(sbom_input)

    @staticmethod
    def _run_traced_build(app_dir: str, trace_file: str):
        """
        Run a traced build of the application in the specified directory.

        This function performs a clean build followed by a traced build using `strace`.
        The traced build logs file access operations to the specified trace file.

        :param app_dir: The directory of the application to build.
        :type app_dir: str
        :param trace_file: The file where the trace output will be saved.
        :type trace_file: str
        :raises RuntimeError: If the clean or build process fails.
        """
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
        """
        Parses a trace file to extract and resolve file paths.

        This function reads a trace file line by line, looking for lines that contain
        file open operations (`openat` or `open`). It extracts the file paths from these
        lines, resolves them to absolute paths, and filters out temporary files and
        non-regular files.

        :param trace_file: The path to the trace file to be parsed.
        :type trace_file: str
        :return: A sorted list of unique file paths found in the trace file.
        :rtype: list[str]
        """
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
    def _build_file_data(sbom_input, file_paths, package_data: list):
        """
        Build file data for the given file paths.

        :param sbom_input: The input data for the SBOM (Software Bill of Materials).
        :type sbom_input: dict
        :param file_paths: A list of file paths to process.
        :type file_paths: list
        :param package_data: A list of package data.
        :type package_data: list
        :return: A list of file data dictionaries.
        :rtype: list
        """
        logging.info('Building file data')
        file_data = []
        for file_path in file_paths:
            file_info = BuildScanner._match_package_for_file(file_path, sbom_input, package_data)
            file_data.append(file_info)
        return file_data

    @staticmethod
    def _match_package_for_file(file_path, sbom_input, package_data):
        """
        Retrieve package name for a file, if possible.
        This function attempts to match a file path to a known package source directory.

        :param file_path: The path to the file being analyzed.
        :type file_path: str
        :param sbom_input: The input data containing information about RIOT, external modules, and the application.
        :type sbom_input: dict
        :param package_data: A list of package data dictionaries, each containing 'source_dir' and 'name' keys.
        :type package_data: list of dict
        :return: A dictionary containing the file path and the associated package name.
        :rtype: dict
        """
        file_info = {
            'path': file_path,
            'package': None
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
        self.assertRaises(RuntimeError, lambda: scanner.board_data)
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
        self.assertEqual(scanner.board_data['name'], 'native64')
        self.assertIsInstance(scanner.riot_data, dict)
        self.assertEqual(len(scanner.external_module_data), 0)
        self.assertGreater(len(scanner.package_data), 0)
        self.assertGreater(len(scanner.file_data), 0)


if __name__ == '__main__':
    unittest.main()
