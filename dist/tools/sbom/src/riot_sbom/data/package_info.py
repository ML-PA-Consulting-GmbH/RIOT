"""
Copyright (C) 2025 ML!PA Consulting GmbH

SPDX-License-Identifier: MIT

Authors:
    Daniel Lockau <daniel.lockau@ml-pa.com>
"""

__all__ = ["PackageInfo"]

from dataclasses import dataclass
from pathlib import Path
from typing import List, Tuple

@dataclass
class PackageInfo:
    name: str
    supplier: str | None
    authors: List[Tuple[str, str]] | None
    version: str | None
    source_dir: Path | None
    url: str | None
    licenses: List[str] | None
    copyrights: List[str] | None
