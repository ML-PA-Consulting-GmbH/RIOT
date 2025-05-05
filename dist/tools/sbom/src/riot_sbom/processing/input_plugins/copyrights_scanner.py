"""
Copyright (C) 2025 ML!PA Consulting GmbH

SPDX-License-Identifier: MIT

Authors:
    Daniel Lockau <daniel.lockau@ml-pa.com>
"""

    @staticmethod
    def _get_copyright(line):
        copyright = None
        if 'copyright' in line.lower():
            copyright_matcher = re.compile(r'[Cc]opyright[ \t]*\([cC]\)[ \t]*([0-9]{4})(.*)')
            m = copyright_matcher.search(line)
            if m:
                copyright = m.group(1) + m.group(2)
                copyright.strip(' \t*/')
