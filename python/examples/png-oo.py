#!/usr/bin/env python
# -*- coding:utf-8 -*-

import qr;

q = qr.QRCode()
q.add_data('spam, egg')
q.format = qr.PNG
q.scale = 8

open('spamegg.png', 'wb').write(q.get_symbol())
