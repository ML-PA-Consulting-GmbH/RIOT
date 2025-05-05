"""
Copyright (C) 2025 ML!PA Consulting GmbH

SPDX-License-Identifier: MIT

Authors:
    Daniel Lockau <daniel.lockau@ml-pa.com>
"""

from dataclasses import dataclass
from enum import Enum
import hashlib
from pathlib import Path
from typing import List, Tuple
import unittest

__all__ = ['DigestType', 'FileInfo']

class DigestType(Enum):
    """
    Different types of digests for a file.
    """
    SHA1 = 'sha1'
    MD5 = 'md5'
    SHA256 = 'sha256'
    SHA512 = 'sha512'
    SHA3_256 = 'sha3_256'
    SHA3_384 = 'sha3_384'
    SHA3_512 = 'sha3_512'

@dataclass
class FileInfo:
    path: Path
    package: str | None
    licenses: List[str]
    copyrights: List[str] | None
    authors: List[Tuple[str, str]] | None

    def get_digest(self, digest_type: DigestType) -> str:
        with open(self.path, 'rb') as f:
            return hashlib.file_digest(f, digest_type.value).hexdigest()


class TestFileInfo(unittest.TestCase):
    def test_get_digest(self):
        file = Path(__file__)
        # Create a FileInfo object
        file_info = FileInfo(path=file, package=None, licenses=[], copyrights=None, authors=None)
        for digest_type in DigestType:
            # Calculate the digest using the get_digest method
            digest = file_info.get_digest(digest_type)
            # Calculate the expected digest using hashlib
            expected_digest = hashlib.new(digest_type.value, file.read_bytes()).hexdigest()
            self.assertEqual(digest, expected_digest, f"Digest mismatch for {digest_type.value}")


if __name__ == '__main__':
    unittest.main()
