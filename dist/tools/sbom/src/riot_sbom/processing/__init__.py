"""
Copyright (C) 2025 ML!PA Consulting GmbH

SPDX-License-Identifier: MIT

Authors:
    Daniel Lockau <daniel.lockau@ml-pa.com>
"""

import plugin_registry
import plugin_type
import input_plugins
import output_plugins

__all__ = [
    'plugin_registry',
    'plugin_type',
    'input_plugins',
    'output_plugins'
]

from pathlib import Path
import logging

for pkg in [input_plugins, output_plugins]:
    if not pkg.__file__:
        logging.error(f"Cannot load default plugins from {pkg.__name__}: __file__ is None")
        continue
    for mod in plugin_registry._load_modules_from_directory(Path(pkg.__file__).parent, False):
        plugin_registry._load_plugins_from_module(mod)
