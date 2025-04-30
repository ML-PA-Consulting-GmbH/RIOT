"""
Copyright (C) 2025 ML!PA Consulting GmbH

SPDX-License-Identifier: MIT

Authors:
    Daniel Lockau <daniel.lockau@ml-pa.com>
"""

from .processing.build_scanner import BuildScanner
import .processing import plugin_registry
import pathlib

def run_build(app_dir: pathlib.Path):
    """
    Run the build process for a RIOT application.

    :param app_dir: The directory of the RIOT application.
    """
    if not app_dir.is_dir():
        raise ValueError(f'{app_dir} is not a directory')
    if not app_dir.joinpath('Makefile').is_file():
        raise ValueError(f'{app_dir} does not contain a Makefile')
    scanner = BuildScanner(app_dir)
    scanner.run()
    return scanner.get_app_info()

def load_app_info(file: pathlib.Path):
    """
    Load application information from a file.

    :param file: The file to load the application information from.
    """
    if not file.is_file():
        raise ValueError(f'{file} is not a file')
    # TODO
    return None

def save_app_info(app_info, file: pathlib.Path):
    """
    Save application information to a file.

    :param app_info: The application information to save.
    :param file: The file to save the application information to.
    """
    if not file.parent.is_dir():
        raise ValueError(f'{file.parent} is not a directory. Cannot create output file.')
    # TODO

def run_plugin(app_info, plugin: str, output_file_prefix: pathlib.Path):
    """
    Run a processing plugin plugin on the application information.

    :param app_info: The application information to process.
    :param plugin: The name of the plugin to run.
    :param output_file: The file to write the output to.
    """
    plugin_instance = plugin_registry.get_plugin(plugin)
    if plugin_instance is None:
        raise ValueError(f'Plugin {plugin} not found')
    plugin_instance.run(app_info, output_file_prefix)
