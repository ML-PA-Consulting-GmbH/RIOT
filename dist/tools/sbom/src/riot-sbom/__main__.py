"""
Copyright (C) 2025 ML!PA Consulting GmbH

SPDX-License-Identifier: MIT

Authors:
    Daniel Lockau <daniel.lockau@ml-pa.com>
"""

from .api import run_build
from .api import load_app_info
from .api import save_app_info
from .api import run_plugin

from pathlib import Path
import logging
from typing import List

def main() -> int:
    from .processing import plugin_registry
    import argparse
    parser = argparse.ArgumentParser(description='RIOT SBOM generator')
    parser.add_argument('app-dir', type=Path,
                        help='Path to the directory of the application for which the SBOM should be built. `make -j` should build the application in that directory. Ignored if loading application information from a file, required otherwise.',
                        required=False)
    parser.add_argument('--save-app-info', type=Path,
                        help='Path to the file where application information is saved in python pickle format. Will be ignored if loading application information from a file. This step will run after the application build and after each successfully executed input plugin.',
                        required=False)
    parser.add_argument('--load-app-info', type=Path,
                        help='Path to the file from which application information is loaded in python pickle format. This will not execute an application build. All selected input plugins will be re-executed.',
                        required=False)
    parser.add_argument('--output-file-prefix', type=Path,
                        help='Prefix of output files, without extension. Required if output plugins are selected.',
                        required=False)
    parser.add_argument('--external-plugin-dirs', nargs='+', type=List[str],
                        help='List of directories to search for loadable plugins.',
                        required=False)
    parser.add_argument('--list-plugins', action='store_true',
                        help='List all available plugins and exit.',
                        required=False)
    parser.add_argument('--plugin-pipeline', nargs='*', type=str,
                        help='List of input processing plugins to use for extraction of information or output generation. Plugins are expected to overwrite content created by other plugins, so highest priority should go last for anything altering the application information.',
                        required=False,
                        default=['scan-spdx-identfiers', 'scan-authors', 'scan-copyrights', 'generate-spdx', 'generate-mlpa-json'])
    args = parser.parse_args()

    # Load external plugins
    for ext_dir in args.extension_dirs:
        if ext_dir.is_dir():
            plugin_registry.load_plugins_from_directory(ext_dir)
        else:
            logging.error(f"Extension directory {ext_dir} does not exist. Aborting.")
            return 1
    if args.list_plugins:
        plugin_registry.print_plugin_list()
        return 0

    # Verify command line arguments
    non_existing_plugins = [x for x in args.plugin_pipeline
                            if x not in plugin_registry.get_plugin_names()]
    if non_existing_plugins:
        logging.error(f"The following selected plugins are not available: {non_existing_plugins}. Please run with `--list-plugins` to list available plugins.")
        return 1
    if args.load_app_info and args.app_dir:
        logging.error("Cannot specify both `--load-app-info` and `app-dir`. Please use one of them.")
        return 1
    if args.output_plugins and not args.output_file_prefix:
        logging.error("Cannot specify output plugins without an output file prefix. Please use `--output-file-prefix`.")
        return 1
    if args.app_dir and not args.app_dir.is_dir():
        logging.error(f"Application directory {args.app_dir} does not exist. Aborting.")
        return 1
    if args.app_dir and not args.app_dir.joinpath('Makefile').is_file():
        logging.error(f"Application directory {args.app_dir} does not contain a Makefile. Aborting.")
        return 1
    if args.save_app_info and not args.save_app_info.parent.is_dir():
        logging.error(f"Cannot create output file {args.save_app_info}. Parent directory does not exist. Aborting.")
        return 1
    if args.load_app_info and not args.load_app_info.is_file():
        logging.error(f"Cannot load application information from {args.load_app_info}. File does not exist. Aborting.")
        return 1

    # Get application information
    if args.load_app_info:
        logging.info(f"Loading application information from {args.load_app_info}")
        app_info = load_app_info(args.load_app_info)
    else:
        logging.info(f"Running build in {args.app_dir}")
        app_info = run_build(args.app_dir)
        if args.save_app_info:
            logging.info(f"Saving application information to {args.save_app_info}")
            save_app_info(app_info, args.save_app_info)

    if not app_info:
        logging.error("No application information available. Aborting.")
        return 1

    # Run input plugins
    logging.info(f"Running plugins: {args.plugin_pipeline}")
    for plugin in args.plugin_pipeline:
        logging.info(f"Running input plugin {plugin}")
        new_app_info = run_plugin(app_info, plugin)
        if not new_app_info:
            logging.warning(f"Plugin {plugin} does not conform to output spec.")
        elif args.save_app_info and new_app_info != app_info:
            logging.info(f"Saving new application information to {args.save_app_info}")
            save_app_info(app_info, args.save_app_info)

    return 0

if __name__ == '__main__':
    main()
