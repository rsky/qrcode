#!/usr/bin/env python
# -*- coding:utf-8 -*-

from __future__ import print_function
import qr;

q = qr.QRCode()
q.add_data('spam, egg')
q.format = qr.SVG
q.scale = 1
q.separator = 0

print(q.get_symbol())
