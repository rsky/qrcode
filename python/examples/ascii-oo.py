#!/usr/bin/env python
# -*- coding:utf-8 -*-

from __future__ import print_function
import qr;

q = qr.QRCode()
q.add_data('spam, egg')
q.format = qr.ASCII
q.scale = 1

print(q.get_symbol())
