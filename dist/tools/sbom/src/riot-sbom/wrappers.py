# SPDX-License-Identifier: MIT

from .build_scanner import BuildScanner
from .file_info import FileInfo
from .package_info import PackageInfo
from .spdx_builder import SpdxBuilder
import pathlib

def create_spdx_for_build(app_dir: pathlib.Path, output_file: pathlib.Path):
    """
    Create a Software Bill of Materials (SBOM) for a RIOT application.

    :param app_dir: The directory of the RIOT application.
    :param output_file: The file to write the SBOM to.
    """
    if not app_dir.is_dir():
        raise ValueError(f'{app_dir} is not a directory')
    if not app_dir.joinpath('Makefile').is_file():
        raise ValueError(f'{app_dir} does not contain a Makefile')
    if not output_file.parent.is_dir():
        raise ValueError(f'{output_file.parent} is not a directory. Cannot create output file.')
    scanner = BuildScanner(app_dir)
    scanner.run()
    spdx = SpdxBuilder(
            f'Automatically generated SBOM for RIOT APPLICATION "{scanner.app_data['name']}"',
            scanner.app_data['name'],
            'https://riot-os.org',
            [('RIOT SBOM Tool', '')]
    )
    pkg_map = {}
    pkg_map[scanner.app_data['name']] = (scanner.app_data,
                                         PackageInfo.from_package_data(scanner.app_data))
    scanner.riot_data['name'] = 'RIOT OS'
    pkg_map['RIOT OS'] = (scanner.riot_data, PackageInfo.from_package_data(scanner.riot_data))
    pkg_map[scanner.app_data['name']] = (scanner.app_data,
                                         PackageInfo.from_package_data(scanner.app_data))
    pkg_map.update({module['name']: (module, PackageInfo.from_package_data(module))
                    for module in scanner.external_module_data})
    pkg_map.update({pkg['name']: (pkg, PackageInfo.from_package_data(pkg))
                    for pkg in scanner.package_data})
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
