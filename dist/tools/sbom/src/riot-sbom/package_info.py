__all__ = ["PackageInfo"]

class PackageInfo:
    def __init__(self, name, version, source_dir, url, license, copyright):
        """
        :param name: The name of the package.
        :param version: The version of the package.
        :param source_dir: The directory of the package source.
        :param url: The URL of the package.
        :param license: The license of the package.
        :param copyright: The copyright of the package.
        """
        self.name = name
        self.supplier = None
        self.originator = None
        self.version = version
        self.source_dir = source_dir
        self.url = url
        self.license = license
        self.copyright = copyright
        self.external_ref = []

    @classmethod
    def from_package_data(cls, data):
        """
        Create a PackageInfo object from package data.

        :param data: The package data. Format as per the build scanner.
        :return: The PackageInfo object.
        """
        return cls(
            name=data.get("name"),
            version=data.get("version"),
            source_dir=data.get("source_dir"),
            url=data.get("url", None),
            license=data.get("license", None),
            copyright=data.get("copyright", None)
        )
