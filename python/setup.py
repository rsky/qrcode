#!/usr/bin/env python

from distutils.core import setup, Extension

module1 = Extension('qr',
        include_dirs = ['./libqr'],
        libraries = ['z'],
        library_dirs = [],
        sources = ['qrmodule.c', 'libqr/qr.c', 'libqr/qrcnv.c',
                   'libqr/qrcnv_bmp.c', 'libqr/qrcnv_png.c',
                   'libqr/qrcnv_svg.c', 'libqr/qrcnv_tiff.c'])

setup(name = 'qr',
        version = '0.2.1',
        description = 'QR Code Module for Python',
        ext_modules = [module1])
