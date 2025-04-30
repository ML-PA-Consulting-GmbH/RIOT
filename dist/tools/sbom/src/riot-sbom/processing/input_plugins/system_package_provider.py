"""
Copyright (C) 2025 ML!PA Consulting GmbH

SPDX-License-Identifier: MIT

Authors:
    Daniel Lockau <daniel.lockau@ml-pa.com>
"""

def _complete_package_data(sbom_input, file_paths: list):
    """
    Complete the package data by inferring package information from file paths.

    This function takes an SBOM (Software Bill of Materials) input and a list of file paths,
    and attempts to complete the package data by matching file paths to known package patterns.
    If a file path does not match any known package, it is added to a set of unresolved packages.

    :param sbom_input: The input SBOM data containing information about packages, application, and external modules.
    :type sbom_input: dict
    :param file_paths: A list of file paths to be checked against known package patterns.
    :type file_paths: list
    :return: A list of package data with inferred package information.
    :rtype: list
    """
    logging.info('Completing package data')
    package_data = [pkg for pkg in sbom_input['packages']]
    unresolved_packages = set()
    for file in file_paths:
        if (
            file.startswith(sbom_input['application']['source_dir'])
            or file.startswith(sbom_input['application']['build_dir'])
            or file.startswith(sbom_input['riot']['source_dir'])
            or any(file.startswith(package['source_dir'])
                    for package in package_data)
            or any(file.startswith(ext_mod['source_dir'])
                    for ext_mod in sbom_input['external_modules'])
            ):
            continue
        # package not known, try to infer from path
        # NOTE this is an ugly temporary solution
        packag_reg = [
            # TODO complete the information below
            {
                'name': 'picolibc',
                'matcher': re.compile(r'^(.*/picolibc)/.*$'),
                'url': "https://github.com/picolibc/picolibc",
                'version': None,
                'license': None # file dependent
            },
            {
                'name': 'newlib',
                'matcher': re.compile(r'^(.*/newlib)/.*$'),
                'url': "https://sourceware.org/newlib/",
                'version': None,
                'license': None # file dependent
            },
            {
                'name': 'libgcc',
                'matcher': re.compile(r'^(.*/lib/gcc)/.*$'),
                'url': "https://gcc.gnu.org/",
                'version': None,
                'license': "GPL-3.0-or-later WITH GCC-exception-3.1"
            },
            {
                'name': 'clang',
                'matcher': re.compile(r'^(.*/lib/clang).*$'),
                'url': "https://clang.llvm.org/",
                'version': None,
                'license': "Apache-2.0 WITH LLVM-exception"
            },
            {
                'name': 'llvm',
                'matcher': re.compile(r'^(.*/lib/llvm).*$'),
                'url': "https://llvm.org/",
                'version': None,
                'license': "Apache-2.0 WITH LLVM-exception"
            }
        ]
        file_has_pkg = False
        for reg in packag_reg:
            match = reg['matcher'].match(file)
            if match:
                package_data.append({
                    'name': reg['name'],
                    'source_dir': match.group(1),
                    'url': reg['url'],
                    'version': reg['version'],
                    'license': reg['license']
                })
                file_has_pkg = True
                break
        if not file_has_pkg:
            unresolved_packages.add(os.path.dirname(file))
    if unresolved_packages:
        logging.warning(f'Unresolved packages for files in: {unresolved_packages}')
    return package_data
