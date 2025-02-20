from .wrappers import create_spdx_for_build
from pathlib import Path

def main():
    import argparse
    parser = argparse.ArgumentParser(description='RIOT SBOM generator')
    parser.add_argument('--app-dir', type=Path, help='Path to the directory of the application', required=True)
    parser.add_argument('--output-file', type=Path, help='Path to the output SPDX file', required=True)
    args = parser.parse_args()
    create_spdx_for_build(args.app_dir, args.output_file)

if __name__ == '__main__':
    main()
