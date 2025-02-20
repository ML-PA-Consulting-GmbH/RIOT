from .build_scanner import BuildScanner
from .file_info import FileInfo
from .package_info import PackageInfo
from .spdx_builder import SpdxBuilder
import pathlib

def create_spdx(app_dir: pathlib.Path, output_file: pathlib.Path):
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
    spdx = SpdxBuilder(scanner.app_data['name'])
    for pkg in scanner.package_data:
        pkg_info = PackageInfo.from_package_data(scanner.package_data[pkg])
        spdx.add_package(pkg_info)
    for file in scanner.file_data:
        file_info = FileInfo.from_parsed_content_and_package(
            file['path'], scanner.package_data[file['package']])
        spdx.add_file(file_info)
    with open(output_file, 'wt') as f:
        spdx.write(f)
