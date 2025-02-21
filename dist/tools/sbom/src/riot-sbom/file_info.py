import hashlib
import re
from typing import Dict

__all__ = ['FileInfo']

class FileInfo:
    """
    SBOM information about a file.

    :ivar path: Path to the file.
    :vartype path: str
    :ivar sha1: SHA1 hash of the file.
    :vartype sha1: str
    :ivar package: Name of the package the file belongs to.
    :vartype package: str
    :ivar license: License of the file, if provided in the file.
    :vartype license: str
    :ivar copyright: Copyright of the file, if provided in the file.
    :vartype copyright: str
    """

    def __init__(self, path: str, digests: Dict[str, str], package: str | None, license: str | None, copyright: str | None):
        self.path = path
        self.digests = digests
        self.package = package
        self.license = license
        self.copyright = copyright

    @staticmethod
    def _get_copyright(line):
        copyright = None
        if 'copyright' in line.lower():
            copyright_matcher = re.compile(r'[Cc]opyright[ \t]*\([cC]\)[ \t]*([0-9]{4})(.*)')
            m = copyright_matcher.search(line)
            if m:
                copyright = m.group(1) + m.group(2)
                copyright.strip(' \t*/')

    @staticmethod
    def _get_sha1(path):
        with open(path, 'rb') as f:
            sha1 = hashlib.file_digest(f, 'sha1').hexdigest()
            f.seek(0)
            md5 = hashlib.file_digest(f, 'md5').hexdigest()
        return {'sha1': sha1, 'md5': md5}

    @staticmethod
    def _get_spdx_license(line):
        license = None
        if 'SPDX-License-Identifier:' in line:
            license_input = line.split('SPDX-License-Identifier:', 1)[1].strip()
            license_parts = []
            lic_part = re.compile(r'^[ \t]*([A-Za-z0-9\.-]+)(.*)$')
            lic_sep = re.compile(r'^[ \t]+(AND|OR|WITH)(.*)')
            while license_input:
                m = lic_part.match(license_input)
                if m:
                    license_parts.append(m.group(1))
                    license_input = m.group(2)
                m = lic_sep.match(license_input)
                if m:
                    license_parts.append(m.group(1))
                    license_input = m.group(2)
                else:
                    break
            license = ' '.join(license_parts)
        return license

    @staticmethod
    def _collect_license_information(line):
        common_licenses = {
            "MIT": "MIT",
            "GNU General Public License v2.0": "GPL-2.0",
            "GPL-2.0": "GPL-2.0",
            "GNU General Public License v3.0": "GPL-3.0",
            "GPL-3.0": "GPL-3.0",
            "GNU Lesser General Public License v2.1": "LGPL-2.1",
            "LGPL-2.1": "LGPL-2.1",
            "GNU Lesser General Public License v3.0": "LGPL-3.0",
            "LGPL-3.0": "LGPL-3.0",
            "Apache License 2.0": "Apache-2.0",
            "Apache-2.0": "Apache-2.0",
            "BSD 2-Clause": "BSD-2-Clause",
            "BSD-2-Clause": "BSD-2-Clause",
            "BSD-3-Clause": "BSD-3-Clause",
            "BSD 3-Clause": "BSD-3-Clause",
            "Mozilla Public License 2.0": "MPL-2.0",
            "MPL-2.0": "MPL-2.0",
            "Common Development and Distribution License 1.0": "CDDL-1.0",
            "CDDL-1.0": "CDDL-1.0",
            "Eclipse Public License 2.0": "EPL-2.0",
            "EPL-2.0": "EPL-2.0",
            "GNU Affero General Public License v3.0": "AGPL-3.0",
            "AGPL-3.0": "AGPL-3.0",
            "Unlicense": "Unlicense"
        }
        license_info_matches = set()
        if 'license' in line.lower():
            for license in common_licenses:
                if license in line:
                    license_info_matches.add(common_licenses[license])
        return list(license_info_matches)

    @staticmethod
    def _infer_license_from_snippets(license_info_snippets):
        license = " AND ".join(license_info_snippets)
        return license

    @classmethod
    def from_parsed_content_and_package(cls, path: str, package_data: dict | None):
        """
        Create a FileInfo object from parsed content and package data.

        :param path: Path to the file.
        :type path: str
        :param package_data: Package data from the build scanner.
        :type package_data: dict | None
        :return: A FileInfo object.
        :rtype: FileInfo
        """
        sha1 = cls._get_sha1(path)
        spdx_license = None
        copyright = None
        license_info_snippets = []
        package_name = package_data['name'] if package_data else None
        with open(path, 'rt') as f:
            while not copyright or not spdx_license:
                line = f.readline()
                if not line:
                    break
                license_info_snippets.extend(cls._collect_license_information(line))
                spdx_license = cls._get_spdx_license(line)
                copyright = cls._get_copyright(line)
        if spdx_license:
            license = spdx_license
        else:
            license = cls._infer_license_from_snippets(license_info_snippets)
        return cls(path, sha1, package_name, license, copyright)

    def __str__(self):
        return f'FileInfo(path={self.path}, sha1={self.digests}, package={self.package}, license={self.license}, copyright={self.copyright})'
