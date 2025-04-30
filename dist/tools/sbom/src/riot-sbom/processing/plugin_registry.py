"""
Copyright (C) 2025 ML!PA Consulting GmbH

SPDX-License-Identifier: MIT

Authors:
    Daniel Lockau <daniel.lockau@ml-pa.com>
"""

from typing import Dict, List
from types import ModuleType
from pathlib import Path


class Plugin:
    """
    A base class for plugins.
    This class must be inherited by all plugins.
    """
    def __init__(self):
        """
        Initializes the plugin.
        This method should be overridden by subclasses to perform any
        necessary initialization.
        """
        raise NotImplementedError("Subclasses must implement this method")

    def get_name(self):
        """
        Returns the name of the plugin.
        This method should be overridden by subclasses to provide the
        plugin's name.
        """
        raise NotImplementedError("Subclasses must implement this method")

    def get_description(self):
        """
        Returns a description of the plugin.
        This method should be overridden by subclasses to provide the
        plugin's description.
        """
        raise NotImplementedError("Subclasses must implement this method")

    def run(self, app_info, output_file_prefix: Path):
        """
        Runs the plugin on the given app_info and output_file_prefix.
        This method should be overridden by subclasses to implement the
        plugin's functionality.

        For any plugins altering the app_info, the plugin should return
        a modified deep copy of the app_info. Otherwise, the original
        app_info should be returned.

        :param app_info: The application information to process.
        :param output_file_prefix: The prefix for the output files.
        :return: The processed application information.
        """
        raise NotImplementedError("Subclasses must implement this method")


_plugin_registry: Dict[str, Plugin] = {}

def _load_modules_from_directory(directory: Path) -> List[ModuleType]:
    """
    Load all modules from the specified directory.

    :param directory: The directory to load modules from.
    :type directory: str
    :return: A list of loaded modules.
    """
    import os
    import importlib.util

    modules = []
    for filename in os.listdir(directory: Path):
        if filename.endswith('.py') and filename != '__init__.py':
            module_name = filename[:-3]
            module_path = os.path.join(directory, filename)
            spec = importlib.util.spec_from_file_location(module_name, module_path)
            if spec is None or spec.loader is None:
                print(f"Could not load module {module_name} from {module_path}")
                continue
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            modules.append(module)
    return modules


def load_plugins_from_directory(directory: Path) -> None:
    """
    Load all plugins from the specified directory.

    :param directory: The directory to load plugins from.
    :type directory: str
    :return: A list of loaded plugins.
    """
    modules = _load_modules_from_directory(directory)
    for module in modules:
        for name, obj in vars(module).items():
            if isinstance(obj, type) and issubclass(obj, Plugin) and obj is not Plugin:
                try:
                    plugin_instance = obj()
                except Exception as e:
                    print(f"Failed to instantiate plugin {name}: {e}")
                    continue
                _plugin_registry[plugin_instance.get_name()] = plugin_instance
                print(f"Loaded plugin: {plugin_instance.get_name()} - {plugin_instance.get_description()}")


def print_plugin_list() -> None:
    """
    Print the list of available plugins.
    """
    print("Available plugins:")
    for name, plugin in _plugin_registry.items():
        print(f"PLUGIN: {name}")
        print(f"DESCRIPTION: {plugin.get_description()}")


def get_plugin_names() -> List[str]:
    """
    Get the names of all available plugins.

    :return: A list of plugin names.
    """
    return list(_plugin_registry.keys())


def get_plugin(name: str) -> Plugin:
    """
    Get a plugin by its name.

    :param name: The name of the plugin.
    :type name: str
    :return: The plugin instance.
    """
    if name not in _plugin_registry:
        raise KeyError(f"Plugin {name} not found.")
    return _plugin_registry[name]

if __name__ == "__main__":
    # Test module loading
    import sys
    directory = sys.argv[1] if len(sys.argv) > 1 else '.'
    loaded_modules = _load_modules_from_directory(directory)
    for module in loaded_modules:
        print(f"Loaded module: {module.__name__} with contents {dir(module)}")
