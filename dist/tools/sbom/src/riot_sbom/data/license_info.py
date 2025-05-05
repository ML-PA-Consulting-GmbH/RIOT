"""
Copyright (C) 2025 ML!PA Consulting GmbH

SPDX-License-Identifier: MIT

Authors:
    Daniel Lockau <daniel.lockau@ml-pa.com>
"""

__all__ = ["LicenseInfo"]

from dataclasses import dataclass
from enum import Enum

class DeclarationType(Enum):
    """
    Different types of declarations for a license.
    A package or file can contain the license text, reference a license by
    defined name or have an inexact match to a license. If no match exists,
    a license can still be derived for a file from the package license or
    for a package from its contained files.
    """
    CONTAINED = 0
    REFERENCED = 1
    MATCHED = 2
    DERIVED = 3

@dataclass
class LicenseInfo:
    name: str
    supplier: str | None
    authors: List[Tuple[str, str]] | None
    version: str | None
    source_dir: Path | None
    url: str | None
    licenses: List[str] | None
    copyrights: List[str] | None
