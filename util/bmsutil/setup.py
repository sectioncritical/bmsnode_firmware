from setuptools import setup

setup(
    name = 'bmsutil',
    version = '0.1',
    py_modules = ['bmsutil', 'bmsloader'],
    install_requires = ['pyserial', 'blessed'],
    entry_points = {
        "console_scripts": [
        "bmsutil=bmsutil:cli",
        "bmsloader=bmsloader:cli"]
    }
)
