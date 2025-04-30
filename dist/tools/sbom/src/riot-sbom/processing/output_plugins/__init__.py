import logging

__all__ = []

# try to load all included generators to register them in the plugin registry

try:
    from . import spdx_generator
    __all__.append('spdx_generator')
except:
    logging.info("spdx_generator.py could not be imported")
