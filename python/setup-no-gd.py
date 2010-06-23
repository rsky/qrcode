#!/usr/bin/env python

from distutils.core import setup, Extension

module1 = Extension('qr',
        define_macros = [('QR_PYTHON_MODULE', 1),
                         ('QR_STATIC_BUILD', 1)],
        include_dirs = ['./libqr'],
        libraries = [],
        library_dirs = [],
        sources = ['qrmodule.c', 'libqr/qr.c', 'libqr/qrcnv.c',
                   'libqr/qrcnv_bmp.c', 'libqr/qrcnv_svg.c'])

setup(name = 'qr',
        version = '0.2.0',
        description = 'QR Code Module for Python',
        ext_modules = [module1])
