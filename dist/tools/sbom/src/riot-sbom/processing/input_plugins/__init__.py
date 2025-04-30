"""
Copyright (C) 2025 ML!PA Consulting GmbH

SPDX-License-Identifier: MIT

Authors:
    Daniel Lockau <daniel.lockau@ml-pa.com>
"""

import logging

__all__ = []

# try to load all included scanners to register them in the plugin registry

try:
    from . import scancode_scanner
    __all__.append('scancode_scanner')
except:
    logging.info("scancode_scanner.py could not be imported")
