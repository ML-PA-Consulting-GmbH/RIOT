"""
Copyright (C) 2025 ML!PA Consulting GmbH

SPDX-License-Identifier: MIT

Authors:
    Daniel Lockau <daniel.lockau@ml-pa.com>
"""

import logging
import unittest

from riot_sbom.processing.plugin_type import Plugin


__all__ = ["MlpaJsonGenerator"]

class MlpaJsonGenerator(Plugin):
    def get_name(self):
        return "mlpa-json-generator"

    def get_description(self):
        return "Generates an ML!PA-format complying JSON output."

    def run(self, app_info, output_file_prefix):

        return app_info

if __name__ == "__main__":
    unittest.main()
