"""
Copyright (C) 2025 ML!PA Consulting GmbH

SPDX-License-Identifier: MIT

Authors:
    Daniel Lockau <daniel.lockau@ml-pa.com>
"""

__all__ = ["SpdxBuilder"]

import logging
import os
import pathlib
from datetime import datetime
from typing import List, Tuple
from riot_sbom.data.package_info import PackageInfo
from riot_sbom.data.file_info import FileInfo

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
from spdx_tools.spdx.model.spdx_none import SpdxNone
from spdx_tools.spdx.model.spdx_no_assertion import SpdxNoAssertion
from spdx_tools.spdx.validation.document_validator import validate_full_spdx_document
from spdx_tools.spdx.validation.validation_message import ValidationMessage
from spdx_tools.spdx.writer.write_anything import write_file

class SpdxBuilder:
    def __init__(self, document_name: str, application_name: str,
                 document_namespace: str, creators: List[Tuple[str, str]]):
        """
        :param document_name: The name of the SPDX document.
        :param data_license: The SPDX license identifier of the SPDX document.
        :param document_namespace: The namespace of the SPDX document (URL-formatted).
        :param creators: A list of tuples containing the name and email of the creators of the SPDX document.
        """
        self.__application_name = application_name
        self.__document_ref = "SPDXRef-DOCUMENT"
        self.__application_ref = "SPDXRef-Application"
        creation_info = CreationInfo(
            spdx_version="SPDX-2.3",
            spdx_id=self.__document_ref,
            name=document_name,
            data_license="CC0-1.0",
            document_namespace=document_namespace,
            creators=[Actor(ActorType.PERSON, name, email) for name, email in creators],
            created=datetime.now(),
        )
        self.__document = Document(creation_info)
        self.__package_name_to_ref_map = {}

    def add_package(self, package_info: PackageInfo):
        logging.debug(f"Adding package {package_info.name}")
        package_ref = (self.__application_ref
                       if self.__application_name == package_info.name
                       else f"SPDXRef-Package-{len(self.__document.packages) + 1}")
        self.__package_name_to_ref_map[package_info.name] = package_ref
        package = Package(
            name=package_info.name,
            spdx_id=package_ref,
            download_location=package_info.url if package_info.url else SpdxNoAssertion(),
            version=package_info.version if package_info.version else None,
            file_name=package_info.source_dir,
            #supplier=Actor(ActorType.PERSON, "Jane Doe", "jane.doe@example.com"),
            #originator=Actor(ActorType.ORGANIZATION, "some organization", "contact@example.com"),
            files_analyzed=True,
            # verification_code=PackageVerificationCode(
            #     value="d6a770ba38583ed4bb4525bd96e50461655d2758", excluded_files=["./some.file"]
            # ),
            # checksums=[
            #     Checksum(ChecksumAlgorithm.SHA1, "d6a770ba38583ed4bb4525bd96e50461655d2758"),
            #     Checksum(ChecksumAlgorithm.MD5, "624c1abb3664f4b35547e7c73864ad24"),
            # ],
            #license_concluded=spdx_licensing.parse("GPL-2.0-only OR MIT"),
            #license_info_from_files=[spdx_licensing.parse("GPL-2.0-only"), spdx_licensing.parse("MIT")],
            license_declared=(spdx_licensing.parse(package_info.license)
                              if package_info.license else SpdxNone()),
            #license_comment=None,
            copyright_text=package_info.copyright,
            #description="package description",
            #attribution_texts=["package attribution"],
            primary_package_purpose=PackagePurpose.LIBRARY,
            #release_date=datetime(2015, 1, 1),
            # external_references=[
            #     ExternalPackageRef(
            #         category=ExternalPackageRefCategory.OTHER,
            #         reference_type="http://reference.type",
            #         locator="reference/locator",
            #         comment="external reference comment",
            #     )
            # ],
        )
        self.__document.packages.append(package)
        if self.__application_name == package_info.name:
            relationship = Relationship(self.__document_ref, RelationshipType.DESCRIBES, package_ref)
            self.__document.relationships.append(relationship)
        else:
            relationship = Relationship(self.__application_ref, RelationshipType.DEPENDS_ON, package_ref)
            self.__document.relationships.append(relationship)


    def add_file(self, file_info: FileInfo, file_package_info: PackageInfo | None=None):
        logging.debug(f"Adding file {file_info.path}")
        file_ref = f"SPDXRef-File-{len(self.__document.files) + 1}"
        file_package_ref = self.__package_name_to_ref_map.get(file_info.package, None)
        file_name = (os.path.basename(file_info.path)
                     if not file_package_info
                     else os.path.relpath(file_info.path, file_package_info.source_dir))
        file = File(
            name=file_name,
            spdx_id=file_ref,
            file_types=[FileType.SOURCE],
            checksums=[
                Checksum(ChecksumAlgorithm.SHA1, file_info.digests["sha1"]),
                Checksum(ChecksumAlgorithm.MD5, file_info.digests["md5"]),
            ],
            license_concluded=spdx_licensing.parse(file_package_info.license) if file_package_info else None,
            license_info_in_file=[spdx_licensing.parse(file_info.license)] if file_info.license else None,
            copyright_text=file_info.copyright,
        )
        self.__document.files.append(file)
        if file_package_ref:
            relationship = Relationship(file_package_ref, RelationshipType.CONTAINS, file_ref)
            self.__document.relationships.append(relationship)
        else:
            relationship = Relationship(self.__application_ref, RelationshipType.DEPENDS_ON, file_ref)
            self.__document.relationships.append(relationship)


    def write(self, file_path: pathlib.Path):
        logging.info(f"Writing SPDX file for {self.__application_name} to {file_path}")
        write_file(self.__document, file_path.as_posix(), False)


    def validate(self) -> List[ValidationMessage]:
        return validate_full_spdx_document(self.__document)


def create_spdx_document_from_app_info(app_info: dict, output_file: pathlib.Path):
    """
    Create an SPDX document from the application information.

    :param app_info: The application information to process.
    :param output_file: The file to write the SPDX document to.
    """
    if not app_info['app_dir'].is_dir():
        raise ValueError(f'{app_info["app_dir"]} is not a directory')
    if not app_info['app_dir'].joinpath('Makefile').is_file():
        raise ValueError(f'{app_info["app_dir"]} does not contain a Makefile')
    if not output_file.parent.is_dir():
        raise ValueError(f'{output_file.parent} is not a directory. Cannot create output file.')

    spdx = SpdxBuilder(
            f'Automatically generated SBOM for RIOT APPLICATION "{scanner.app_data['name']}"',
            scanner.app_data['name'],
            'https://riot-os.org',
            [('RIOT SBOM Tool', '')]
    )
    for pkg, pkg_info in pkg_map.values():
        spdx.add_package(pkg_info)
    for file in scanner.file_data:
        file_pkg_info = pkg_map.get(file['package'], (None, None))
        file_info = FileInfo.from_parsed_content_and_package(
                file['path'], file_pkg_info[0])
        spdx.add_file(file_info, file_pkg_info[1])
    print('Validating SPDX document...')
    validation_messages = spdx.validate()
    for message in validation_messages:
        print("======= VALIDATION MESSAGE =======")
        print(message.validation_message)
        print(message.context)
    spdx.write(output_file)
    print(f'Wrote SPDX file to {output_file}')
