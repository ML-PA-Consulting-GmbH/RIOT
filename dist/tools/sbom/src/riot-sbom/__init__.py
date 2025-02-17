from .file_info import FileInfo
from .build_scanner import BuildScanner
from .spdx_writer import SpdxWriter
import logging

__all__ = ['FileInfo', 'BuildScanner', 'SpdxWriter', 'create_sbom']

def create_sbom(app_dir, output_file):
    scanner = BuildScanner(app_dir)
    scanner.run()
    writer = SpdxWriter(output_file, scanner.app_data['name'])
    for pkg in scanner.package_data:
        writer.add_package(pkg)
    for file in scanner.file_data:
        try:
            file_info = FileInfo.from_spdx_identifier_and_package(
                file['path'], scanner.package_data[file['package']])
        except KeyError:
            logging.warning(f'SPDX identifier not found for file {file["path"]}. Trying to defer from parsed content.')
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
