from distutils.core import setup, Extension

module = Extension('pyhook', sources = ['main.c'])

setup (
    name        = 'pyhook',
    version     = '1.0',
    description = 'Python hooks for pppd',
    ext_modules = [module],
)
