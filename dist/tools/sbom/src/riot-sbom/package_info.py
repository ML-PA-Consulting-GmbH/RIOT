__all__ = ["PackageInfo"]

class PackageInfo:
    def __init__(self, name, version, source_dir, url, license, copyright):
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
        return cls(
            name=data.get("name"),
            version=data.get("version"),
            source_dir=data.get("source_dir"),
            url=data.get("url", None),
            license=data.get("license", None),
            copyright=data.get("copyright", None)
        )
