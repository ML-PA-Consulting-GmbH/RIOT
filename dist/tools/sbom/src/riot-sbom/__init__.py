from .file_info import FileInfo
from .build_scanner import BuildScanner
from .spdx_writer import SpdxWriter
from .package_info import PackageInfo

import logging
import pathlib

__all__ = ['FileInfo', 'BuildScanner', 'SpdxWriter',
           'PackageInfo', 'create_sbom']

def create_sbom(app_dir: pathlib.Path, output_file: pathlib.Path):
    """
    Create a Software Bill of Materials (SBOM) for a RIOT application.

    """
    if not app_dir.is_dir():
        raise ValueError(f'{app_dir} is not a directory')
    if not app_dir.joinpath('Makefile').is_file():
        raise ValueError(f'{app_dir} does not contain a Makefile')
    if not output_file.parent.is_dir():
        raise ValueError(f'{output_file.parent} is not a directory. Cannot create output file.')
    scanner = BuildScanner(app_dir)
    scanner.run()
    writer = SpdxWriter(output_file, scanner.app_data['name'])
    for pkg in scanner.package_data:
        pkg_info = PackageInfo.from_package_data(scanner.package_data[pkg])
        writer.add_package(pkg_info)
    for file in scanner.file_data:
        file_info = FileInfo.from_parsed_content_and_package(
            file['path'], scanner.package_data[file['package']])
        writer.add_file(file_info)

def main():
    import argparse
    parser = argparse.ArgumentParser(description='RIOT SBOM generator')
    parser.add_argument('--app-dir', type=str, help='Path to the directory of the application', required=True)
    parser.add_argument('--output-file', type=str, help='Path to the output SPDX file', required=True)
    args = parser.parse_args()
    create_sbom(args.app_dir, args.output_file)

if __name__ == '__main__':
    main()
