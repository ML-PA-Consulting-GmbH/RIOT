"""
Copyright (C) 2025 ML!PA Consulting GmbH

SPDX-License-Identifier: MIT

Authors:
    Daniel Lockau <daniel.lockau@ml-pa.com>
"""

import json
import jsonschema
import logging
import pathlib
import tempfile
from typing import List
import unittest

from riot_sbom.processing.plugin_type import Plugin, FileOrStdoutResource
from riot_sbom.data.license_info import LicenseInfo, LicenseDeclarationType
from riot_sbom.data.copyright_info import CopyrightInfo, CopyrightDeclarationType
from riot_sbom.data.app_info import AppInfo
from riot_sbom.data.file_info import DigestType
from riot_sbom.data.package_info import PackageInfo
from riot_sbom.data.author_info import AuthorInfo, AuthorDeclarationType
from riot_sbom.data.checked_url import CheckedUrl
from riot_sbom.data.file_info import FileInfo, PackageRef


__all__ = ["MlpaJsonGenerator"]

class MlpaJsonGenerator(Plugin):
    def get_name(self):
        return "mlpa-json-generator"

    def get_description(self):
        return "Generates an ML!PA-format complying JSON output."

    def run(self, app_info: AppInfo, output_file_prefix: pathlib.Path) -> AppInfo:
        """
        Generates an ML!PA-format complying JSON output.
        The output is a JSON file that contains information about the package,
        its dependencies, and the source files used in the package.
        """
        def _get_copyright_string(copyrights: List[CopyrightInfo] | None) -> str:
            if not copyrights:
                return ""
            chosen_copyright = copyrights[0]
            copyright_string = f"{chosen_copyright.holder} {chosen_copyright.years}"
            return copyright_string
        def _get_license_string(licenses: List[LicenseInfo] | None) -> str:
            if not licenses:
                return ""
            chosen_license = licenses[0]
            return chosen_license.name
        json_output = {}
        json_output["packageName"] = app_info.app_package.name
        json_output["packageVersion"] = app_info.app_package.version
        json_output["packageLicense"] = _get_license_string(
            app_info.app_package.licenses)
        json_output["packageCopyright"] = _get_copyright_string(
            app_info.app_package.copyrights)
        json_output["dependencies"] = {}
        for dep in app_info.packages:
            if dep == app_info.app_package:
                logging.debug(f"Skipping package {dep} in dependencies list")
                continue
            json_output["dependencies"][dep.name] = {
                "version": dep.version,
                "source": dep.download_url.get() if dep.download_url else "",
                "type": "source",
                "license": _get_license_string(dep.licenses),
                "copyright": _get_copyright_string(dep.copyrights),
                "checksum": None
            }
        json_output["sourceFiles"] = {}
        for file in app_info.files:
            json_output["sourceFiles"][file.path.name] = {
                "version": file.package.resolve.version if file.package.resolve else "",
                "checksum": file.digests[DigestType.MD5],
                "license": _get_license_string(file.licenses),
                "copyright": _get_copyright_string(file.copyrights)
            }
        with FileOrStdoutResource(output_file_prefix, ".mlpa.json") as output_file:
            logging.info(f"Writing ML!PA JSON output to {output_file.name}")
            json.dump(json_output, output_file, indent=2)
        return app_info

class TestMlpaJsonGenerator(unittest.TestCase):
    def test_get_name(self):
        plugin = MlpaJsonGenerator()
        self.assertEqual(plugin.get_name(), "mlpa-json-generator")

    def test_get_description(self):
        plugin = MlpaJsonGenerator()
        self.assertEqual(plugin.get_description(), "Generates an ML!PA-format complying JSON output.")

    def test_run(self):
        schema = """
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "object",
  "properties": {
    "packageName": {
      "type": "string"
    },
    "packageVersion": {
      "type": "string"
    },
    "packageLicense": {
      "type": "string"
    },
    "packageCopyright": {
      "type": "string"
    },
    "dependencies": {
      "type": "object",
      "properties": {
        "packageName": {
          "type": "object",
          "properties": {
            "version": {
              "type": "string"
            },
            "source": {
              "type": "string"
            },
            "type": {
              "type": "string"
            },
            "license": {
              "type": "string"
            },
            "copyright": {
              "type": "string"
            },
            "checksum": {
              "type": "string"
            }
          },
          "required": [
            "version",
            "source",
            "type"
          ]
        }
      }
    },
    "sourceFiles": {
      "type": "object",
      "properties": {
        "filename": {
          "type": "object",
          "properties": {
            "version": {
              "type": "string"
            },
            "checksum": {
              "type": "string"
            },
            "license": {
              "type": "string"
            },
            "copyright": {
              "type": "string"
            }
          },
          "required": [
            "version",
            "checksum",
            "license"
          ]
        }
      }
    }
  },
  "required": [
    "packageName",
    "packageVersion",
    "dependencies"
  ]
}
"""
        app_info = AppInfo(
            build_dir=pathlib.Path("/path/to/build"),
            app_package=PackageInfo(
                name="example-app",
                version="1.0.0",
                licenses=[LicenseInfo(name="MIT",
                                      declaration_type=LicenseDeclarationType.EXACT_REFERENCE,
                                      text=None, url=None)],
                copyrights=[CopyrightInfo(holder="Example Holder",
                                          years="2025",
                                          declaration_type=CopyrightDeclarationType.TEXT_CONTAINED,
                                          url=None)],
                download_url=CheckedUrl("https://example.com/package.tar.gz"),
                authors=[AuthorInfo(name="John Doe",
                                    email="john@doe.com",
                                    declaration_type=AuthorDeclarationType.TEXT_TAGGED)],
                source_dir=pathlib.Path("/path/to/source"),
                supplier="Example Supplier"
            ),
            riot_package=PackageInfo(
                name="example-riot",
                version="1.0.0",
                licenses=[LicenseInfo(name="MIT",
                                      declaration_type=LicenseDeclarationType.EXACT_REFERENCE,
                                      text=None, url=None)],
                copyrights=[CopyrightInfo(holder="Example Holder",
                                          years="2025",
                                          declaration_type=CopyrightDeclarationType.TEXT_CONTAINED,
                                          url=None)],
                download_url=CheckedUrl("https://example.com/package.tar.gz"),
                authors=[AuthorInfo(name="John Doe",
                                    email="john@doe.com",
                                    declaration_type=AuthorDeclarationType.TEXT_TAGGED)],
                source_dir=pathlib.Path("/path/to/source"),
                supplier="Example Supplier"
            ),
            board_package=PackageInfo(
                name="example-board",
                version="1.0.0",
                licenses=[LicenseInfo(name="MIT",
                                      declaration_type=LicenseDeclarationType.EXACT_REFERENCE,
                                      text=None, url=None)],
                copyrights=[CopyrightInfo(holder="Example Holder",
                                          years="2025",
                                          declaration_type=CopyrightDeclarationType.TEXT_CONTAINED,
                                          url=None)],
                download_url=CheckedUrl("https://example.com/package.tar.gz"),
                authors=[AuthorInfo(name="John Doe",
                                    email="john@doe.com",
                                    declaration_type=AuthorDeclarationType.TEXT_TAGGED)],
                source_dir=pathlib.Path("/path/to/source"),
                supplier="Example Supplier"
            ),
            packages=[PackageInfo(
                name="example-pkg1",
                version="1.0.0",
                licenses=[LicenseInfo(name="MIT",
                                      declaration_type=LicenseDeclarationType.EXACT_REFERENCE,
                                      text=None, url=None)],
                copyrights=[CopyrightInfo(holder="Example Holder",
                                          years="2025",
                                          declaration_type=CopyrightDeclarationType.TEXT_CONTAINED,
                                          url=None)],
                download_url=CheckedUrl("https://example.com/package.tar.gz"),
                authors=[AuthorInfo(name="John Doe",
                                    email="john@doe.com",
                                    declaration_type=AuthorDeclarationType.TEXT_TAGGED)],
                source_dir=pathlib.Path("/path/to/source"),
                supplier="Example Supplier"
            ),PackageInfo(
                name="example-pkg2",
                version="1.0.0",
                licenses=[LicenseInfo(name="MIT",
                                      declaration_type=LicenseDeclarationType.EXACT_REFERENCE,
                                      text=None, url=None)],
                copyrights=[CopyrightInfo(holder="Example Holder",
                                          years="2025",
                                          declaration_type=CopyrightDeclarationType.TEXT_CONTAINED,
                                          url=None)],
                download_url=CheckedUrl("https://example.com/package.tar.gz"),
                authors=[AuthorInfo(name="John Doe",
                                    email="john@doe.com",
                                    declaration_type=AuthorDeclarationType.TEXT_TAGGED)],
                source_dir=pathlib.Path("/path/to/source"),
                supplier="Example Supplier"
            )],
            files=[]  # Replace with actual list of FileInfo objects
        )
        app_info.packages.append(app_info.app_package)
        app_info.packages.append(app_info.riot_package)
        app_info.packages.append(app_info.board_package)
        with tempfile.TemporaryDirectory() as temp_dir:
            temp_dir_path = pathlib.Path(temp_dir)
            for ii in range(len(app_info.packages)):
                source_file = temp_dir_path / f"source_file_{ii}.c"
                with open(source_file, 'w') as f:
                    f.write(f"// Source file {ii}")
                app_info.files.append(FileInfo(
                    path=source_file,
                    package=PackageRef(app_info.packages[ii]),
                    licenses=app_info.packages[ii].licenses,
                    copyrights=app_info.packages[ii].copyrights,
                    authors=app_info.packages[ii].authors
                ))
            plugin = MlpaJsonGenerator()
            output_file_prefix = temp_dir_path / "test_output"
            new_app_info = plugin.run(app_info, output_file_prefix)
            self.assertIsInstance(new_app_info, AppInfo)
            self.assertEqual(new_app_info, app_info)
            self.assertTrue(output_file_prefix.with_suffix(".mlpa.json").exists())
            with open(output_file_prefix.with_suffix(".mlpa.json"), 'rt') as f:
                json_data = json.load(f)
            self.assertEqual(json_data["packageName"], app_info.app_package.name)
            self.assertEqual(json_data["packageVersion"], app_info.app_package.version)
            self.assertEqual(json_data["packageLicense"], "MIT")
            self.assertEqual(json_data["packageCopyright"], "Example Holder 2025")
            self.assertIn("example-pkg1", json_data["dependencies"])
            self.assertIn("example-pkg2", json_data["dependencies"])
            for file in app_info.files:
                self.assertIn(file.path.name, json_data["sourceFiles"])
                self.assertEqual(json_data["sourceFiles"][file.path.name]["version"],
                                    file.package.resolve.version if file.package.resolve else "")
                self.assertEqual(json_data["sourceFiles"][file.path.name]["checksum"], file.digests[DigestType.MD5])
                self.assertEqual(json_data["sourceFiles"][file.path.name]["license"], "MIT")
                self.assertEqual(json_data["sourceFiles"][file.path.name]["copyright"], "Example Holder 2025")
            for pkg in app_info.packages:
                if pkg == app_info.app_package:
                    continue
                self.assertIn(pkg.name, json_data["dependencies"])
                self.assertEqual(json_data["dependencies"][pkg.name]["version"], pkg.version)
                self.assertEqual(json_data["dependencies"][pkg.name]["license"], "MIT")
                self.assertEqual(json_data["dependencies"][pkg.name]["copyright"], "Example Holder 2025")
                self.assertEqual(json_data["dependencies"][pkg.name]["checksum"], None)
            self.assertIsNone(jsonschema.validate(json_data, json.loads(schema)))


if __name__ == "__main__":
    unittest.main()
