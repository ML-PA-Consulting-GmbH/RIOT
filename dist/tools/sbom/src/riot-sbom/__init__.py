from . import file_info
from . import build_scanner
from . import spdx_writer

__all__ = ['file_info', 'build_scanner', 'spdx_writer']

def run():
    import argparse
    parser = argparse.ArgumentParser(description='RIOT SBOM generator')
    parser.add_argument('--app-dir', type=str, help='Path to the directory of the application', required=True)
    parser.add_argument('--output-file', type=str, help='Path to the output SPDX file', required=True)
    args = parser.parse_args()
    scanner = build_scanner.BuildScanner(args.app_dir)
    scanner.run()
    app_name = scanner.get_app_name()
    pkg_data = scanner.get_package_data()
    file_data = scanner.get_file_data()
    writer = spdx_writer.SpdxWriter(args.output_file, app_name)
    for pkg in pkg_data:
        writer.add_package(pkg)
    for file in file_data:
        file_pkg = file_info.get_file_package(file, pkg_data)
        file_license = file_info.get_file_license(file)
        writer.add_file(file, file_pkg.name, file_license if file_license else file_pkg.license)

if __name__ == '__main__':
    run()
