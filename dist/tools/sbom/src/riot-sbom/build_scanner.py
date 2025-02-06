#!/usr/bin/env python3

import os
import sys
import json
import subprocess
import argparse
from pathlib import Path

class BuildScanner(object):
    def __init__(self, app_dir):
        self.app_dir = app_dir
        self.app_name = None
        self.package_data = []
        self.file_data = []

    def run(self):
        pass

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
