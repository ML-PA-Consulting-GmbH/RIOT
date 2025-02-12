from .file_info import FileInfo
from .build_scanner import BuildScanner
from .spdx_writer import SpdxWriter
import logging

__all__ = ['FileInfo', 'BuildScanner', 'SpdxWriter']

def run():
    import argparse
    parser = argparse.ArgumentParser(description='RIOT SBOM generator')
    parser.add_argument('--app-dir', type=str, help='Path to the directory of the application', required=True)
    parser.add_argument('--output-file', type=str, help='Path to the output SPDX file', required=True)
    args = parser.parse_args()
    scanner = BuildScanner(args.app_dir)
    scanner.run()
    writer = SpdxWriter(args.output_file, scanner.app_name)
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

if __name__ == '__main__':
    run()
