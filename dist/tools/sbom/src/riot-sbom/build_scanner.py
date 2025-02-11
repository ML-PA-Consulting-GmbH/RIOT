#!/usr/bin/env python3

import os
import json
import logging
import re
import subprocess
import tempfile
from pathlib import Path

class BuildScanner(object):
    def __init__(self, app_dir):
        self.app_dir = app_dir
        self.app_name = None
        self.package_data = []
        self.file_data = []

    def run(self):
        pkg_info = BuildScanner._run_pkg_info(self.app_dir)
        self.app_name = pkg_info['application']
        with tempfile.TemporaryDirectory() as tempdir:
            trace_file = os.path.join(tempdir, 'trace.log')
            BuildScanner._run_traced_build(self.app_dir,
                                           trace_file)
            file_paths = BuildScanner._parse_trace_file(trace_file)
        self.package_data = BuildScanner._complete_package_data(pkg_info['packages'],
                                                                file_paths)
        self.file_data = BuildScanner._build_file_data(file_paths,
                                                       self.app_dir,
                                                       self.package_data)

    def get_app_name(self):
        if not self.app_name:
            raise RuntimeError('Scanner must be run before retrieving data')
        return self.app_name

    def get_package_data(self):
        if not self.package_data:
            raise RuntimeError('Scanner must be run before retrieving data')
        return self.package_data

    def get_file_data(self):
        if not self.file_data:
            raise RuntimeError('Scanner must be run before retrieving data')
        return self.file_data

    @staticmethod
    def _run_pkg_info(app_dir):
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
                            file_paths.add(path)
        return sorted(file_paths)

    @staticmethod
    def _build_file_data(file_paths: list, app_dir: str, package_data: list):
        logging.info('Building file data')
        file_data = []
        for file_path in file_paths:
            file_info = BuildScanner._get_file_info(file_path, app_dir, package_data)
            file_data.append(file_info)
        return file_data

    @staticmethod
    def _complete_package_data(pkg_data: list, file_paths: list):
        logging.info('Completing package data')
        package_data = pkg_data
        for file in file_paths:
            for package in package_data:
                if file.startswith(package['path']):
                    break
                else:
                    package_data.append({
                        'name': 'unknown',
                        'basedir': 'unknown',
                        'url': None,
                        'version': None,
                        'license': 'unknown'
                    })
        return package_data

    @staticmethod
    def _get_file_info(file_path: str, app_dir: str, package_data: list):
        file_info = {
            'path': file_path,
            'package': None,
            'license': None
        }
        for package in package_data:
            if file_path.startswith(package['path']):
                file_info['package'] = package['name']
                break
        return file_info
