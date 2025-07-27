# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'OpenBIOS'
copyright = '2025, The OpenBIOS authors'
author = 'The OpenBIOS authors'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ['myst_parser', 'sphinx_external_toc', 'sphinx_reredirects']

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', 'README.md']

redirects = {
        "License": "Licensing.html",
        "OpenBoot": "OpenBOOT.html",
        "Project_Statement": "Code_of_Conduct.html",
        "Releases": "OpenBIOS.html",
        "Building_OFW_to_Run_Under_BIOS": "Building_OFW_to_Load_from_BIOS.html",
}


# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'alabaster'
html_static_path = ['_static']
