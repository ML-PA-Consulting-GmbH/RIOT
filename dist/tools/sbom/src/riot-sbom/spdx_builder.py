__all__ = ["SpdxBuilder"]

import pathlib
from datetime import datetime
from typing import List, Tuple
from .package_info import PackageInfo
from .file_info import FileInfo

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


# Let's add two files. Have a look at the file class for all possible properties a file can have.
file1 = File(
    name="./package/file1.py",
    spdx_id="SPDXRef-File1",
    file_types=[FileType.SOURCE],
    checksums=[
        Checksum(ChecksumAlgorithm.SHA1, "d6a770ba38583ed4bb4525bd96e50461655d2758"),
        Checksum(ChecksumAlgorithm.MD5, "624c1abb3664f4b35547e7c73864ad24"),
    ],
    license_concluded=spdx_licensing.parse("MIT"),
    license_info_in_file=[spdx_licensing.parse("MIT")],
    copyright_text="Copyright 2022 Jane Doe",
)
file2 = File(
    name="./package/file2.py",
    spdx_id="SPDXRef-File2",
    checksums=[
        Checksum(ChecksumAlgorithm.SHA1, "d6a770ba38583ed4bb4525bd96e50461655d2759"),
    ],
    license_concluded=spdx_licensing.parse("GPL-2.0-only"),
)

# Assuming the package contains those two files, we create two CONTAINS relationships.
contains_relationship1 = Relationship("SPDXRef-Package", RelationshipType.CONTAINS, "SPDXRef-File1")
contains_relationship2 = Relationship("SPDXRef-Package", RelationshipType.CONTAINS, "SPDXRef-File2")

# This library uses run-time type checks when assigning properties.
# Because in-place alterations like .append() circumvent these checks, we don't use them here.
document.relationships += [contains_relationship1, contains_relationship2]
document.files += [file1, file2]

# We now have created a document with basic creation information, describing a package that contains two files.
# You can also add Annotations, Snippets and ExtractedLicensingInfo to the document in an analogous manner to the above.
# Have a look at their respective classes if you are unsure about their properties.


# This library provides comprehensive validation against the SPDX specification.
# Note that details of the validation depend on the SPDX version of the document.
validation_messages: List[ValidationMessage] = validate_full_spdx_document(document)

# You can have a look at each entry's message and context (like spdx_id, parent_id, full_element)
# which will help you pinpoint the location of the invalidity.
for message in validation_messages:
    logging.warning(message.validation_message)
    logging.warning(message.context)

# If the document is valid, validation_messages will be empty.
assert validation_messages == []

# Finally, we can serialize the document to any of the five supported formats.
# Using the write_file() method from the write_anything module,
# the format will be determined by the file ending: .spdx (tag-value), .json, .xml, .yaml. or .rdf (or .rdf.xml)
class SpdxBuilder:
    def __init__(self, document_name: str, data_license: str,
                 document_namespace: str, creators: List[Tuple[str, str]]):
        """
        :param document_name: The name of the SPDX document.
        :param data_license: The SPDX license identifier of the SPDX document.
        :param document_namespace: The namespace of the SPDX document (URL-formatted).
        :param creators: A list of tuples containing the name and email of the creators of the SPDX document.
        """
        creation_info = CreationInfo(
            spdx_version="SPDX-2.3",
            spdx_id="SPDXRef-DOCUMENT",
            name=document_name,
            data_license="CC0-1.0",
            document_namespace="https://some.namespace",
            creators=[Actor(ActorType.PERSON, name, email) for name, email in creators],
            created=datetime.now(),
        )
        self.__document = Document(creation_info)
        self.__package_name_to_ref_map = {}

    def add_package(self, package_info: PackageInfo):
        print(f"Adding package {package_info.name}")
        package_ref = f"SPDXRef-package-{package_info.name}"
        self.__package_name_to_ref_map[package_info.name] = package_ref
        package = Package(
            name=package_info.name,
            spdx_id=package_ref,
            download_location=package_info.url or "",
            version=package_info.version or "",
            file_name=package_info.source_dir,
            supplier=Actor(ActorType.PERSON, "Jane Doe", "jane.doe@example.com"),
            originator=Actor(ActorType.ORGANIZATION, "some organization", "contact@example.com"),
            files_analyzed=True,
            verification_code=PackageVerificationCode(
                value="d6a770ba38583ed4bb4525bd96e50461655d2758", excluded_files=["./some.file"]
            ),
            checksums=[
                Checksum(ChecksumAlgorithm.SHA1, "d6a770ba38583ed4bb4525bd96e50461655d2758"),
                Checksum(ChecksumAlgorithm.MD5, "624c1abb3664f4b35547e7c73864ad24"),
            ],
            license_concluded=spdx_licensing.parse("GPL-2.0-only OR MIT"),
            license_info_from_files=[spdx_licensing.parse("GPL-2.0-only"), spdx_licensing.parse("MIT")],
            license_declared=spdx_licensing.parse("GPL-2.0-only AND MIT"),
            license_comment="license comment",
            copyright_text="Copyright 2022 Jane Doe",
            description="package description",
            attribution_texts=["package attribution"],
            primary_package_purpose=PackagePurpose.LIBRARY,
            release_date=datetime(2015, 1, 1),
            external_references=[
                ExternalPackageRef(
                    category=ExternalPackageRefCategory.OTHER,
                    reference_type="http://reference.type",
                    locator="reference/locator",
                    comment="external reference comment",
                )
            ],
        )
        self.__document.packages.append(package)
        relationship = Relationship("SPDXRef-DOCUMENT", RelationshipType.DESCRIBES, "SPDXRef-Package")
        self.__document.relationships.append(relationship)


    def add_file(self, file_info):
        print(f"Adding file {file_info.path}")

    def write(self, file_path: pathlib.Path):
        print(f"Writing SPDX file for {self.package_name}")
        write_file(self.__document, file_path.as_posix())
