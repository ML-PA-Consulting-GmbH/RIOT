__all__ = ["SpdxBuilder"]

from datetime import datetime
from typing import List

from spdx_tools.common.spdx_licensing import spdx_licensing
from spdx_tools.spdx.model import (
    Actor,
    ActorType,
    Checksum,
    ChecksumAlgorithm,
    CreationInfo,
    Document,
    ExternalPackageRef,
    ExternalPackageRefCategory,
    File,
    FileType,
    Package,
    PackagePurpose,
    PackageVerificationCode,
    Relationship,
    RelationshipType,
)
from spdx_tools.spdx.validation.document_validator import validate_full_spdx_document
from spdx_tools.spdx.validation.validation_message import ValidationMessage
from spdx_tools.spdx.writer.write_anything import write_file


class SpdxBuilder:
    def __init__(self, package_name):
        self.package_name = package_name

    def add_package(self, package_info):
        print(f"Adding package {package_info.name}")

    def add_file(self, file_info):
        print(f"Adding file {file_info.path}")

    def write(self, stream):
        print(f"Writing SPDX file for {self.package_name}")
