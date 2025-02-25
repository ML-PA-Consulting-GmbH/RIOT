# TODO all this and related code should be replaced by scancode-toolkit
# SPDX-License-Identifier: MIT

spdx_license_matcher = {
    "license: mit": "MIT",
    "mit license": "MIT",
    "gnu general public license v2.0": "GPL-2.0",
    "gpl-2.0": "GPL-2.0",
    "gnu general public license v3.0": "GPL-3.0",
    "gpl-3.0": "GPL-3.0",
    "gnu lesser general public license v2.1": "LGPL-2.1",
    "lgpl-2.1": "LGPL-2.1",
    "lgpl": "LGPL-2.1", # get this out
    "gnu lesser general public license v3.0": "LGPL-3.0",
    "lgpl-3.0": "LGPL-3.0",
    "apache license 2.0": "Apache-2.0",
    "apache-2.0": "Apache-2.0",
    "bsd 2-clause": "BSD-2-Clause",
    "bsd-3-clause": "BSD-3-Clause",
    "mozilla public license 2.0": "MPL-2.0",
    "mpl-2.0": "MPL-2.0",
    "common development and distribution license 1.0": "CDDL-1.0",
    "cddl-1.0": "CDDL-1.0",
    "eclipse public license 2.0": "EPL-2.0",
    "epl-2.0": "EPL-2.0",
    "gnu affero general public license v3.0": "AGPL-3.0",
    "agpl-3.0": "AGPL-3.0",
    "unlicense": "Unlicense",
    "pd": "CC0-1.0",
    "cc0-1.0": "CC0-1.0",
    "public domain": None, # get this out as on different legal terms
    "cc0": "CC0-1.0",
    "isc": "ISC",
    "zlib": "Zlib",
    "boost software license 1.0": "BSL-1.0",
    "bsl-1.0": "BSL-1.0",
    "gnu general public license version 2": "GPL-2.0",
    "gnu general public license version 3": "GPL-3.0",
    "gnu lesser general public license version 2.1": "LGPL-2.1",
    "gnu lesser general public license version 3": "LGPL-3.0",
    "apache license version 2.0": "Apache-2.0",
    "bsd 2-clause license": "BSD-2-Clause",
    "bsd 3-clause license": "BSD-3-Clause",
    "mozilla public license version 2.0": "MPL-2.0",
    "common development and distribution license version 1.0": "CDDL-1.0",
    "eclipse public license version 2.0": "EPL-2.0",
    "gnu affero general public license version 3": "AGPL-3.0",
    "the unlicense": "Unlicense",
    "creative commons zero v1.0 universal": "CC0-1.0",
    "boost software license version 1.0": "BSL-1.0"
}

if __name__ == "__main__":
    print("Registered license matchers:", spdx_license_matcher)
    print("Number of matchers:", len(spdx_license_matcher))
    print("Number of licenses covered:", len(set(spdx_license_matcher.values())))
