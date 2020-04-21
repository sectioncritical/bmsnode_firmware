from setuptools import setup

setup(
    name = 'bmsutil',
    version = '0.1',
    py_modules = ['bmsutil'],
    install_requires = ['pyserial'],
    entry_points = '''
        [console_scripts]
        bmsutil=bmsutil:cli
    ''',
)
