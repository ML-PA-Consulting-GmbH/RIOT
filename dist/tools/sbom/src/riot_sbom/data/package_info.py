"""
Copyright (C) 2025 ML!PA Consulting GmbH

SPDX-License-Identifier: MIT

Authors:
    Daniel Lockau <daniel.lockau@ml-pa.com>
"""

__all__ = ["PackageInfo"]

from dataclasses import dataclass
from pathlib import Path
from typing import List

from .author_info import AuthorInfo
from .copyright_info import CopyrightInfo
from .license_info import LicenseInfo
from .checked_url import CheckedUrl

@dataclass
class PackageInfo:
    name: str
    supplier: str | None
    authors: List[AuthorInfo] | None
    version: str | None
    source_dir: Path | None
    download_url: CheckedUrl | None
    licenses: List[LicenseInfo] | None
    copyrights: List[CopyrightInfo] | None
