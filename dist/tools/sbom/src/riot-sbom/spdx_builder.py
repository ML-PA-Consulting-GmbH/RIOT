__all__ = ["SpdxBuilder"]


class SpdxBuilder:
    def __init__(self, package_name):
        self.package_name = package_name

    def add_package(self, package_info):
        print(f"Adding package {package_info.name}")

    def add_file(self, file_info):
        print(f"Adding file {file_info.path}")

    def write(self, stream):
        print(f"Writing SPDX file for {self.package_name}")
