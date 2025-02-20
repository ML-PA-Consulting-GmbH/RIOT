# SBOM Creation for RIOT OS Based Applications

## About

The `sbom-riot` Python package is designed to generate and manage Software
Bill of Materials (SBOM) for software projects.
An SBOM is a comprehensive inventory of all components, libraries, and modules
that are included in a software application.
This package helps in creating valid SBOMs from application build information.
The generated SBOMs are SPDX formatted.

## How to Run

The prerequisite for using this package is a RIOT application which builds
successfully.
The simplest use case is to execute

```bash
$ python -m riot-sbom --app-dir my/application/directory/ --output-file my/output/directory/sbom.spdx
```
