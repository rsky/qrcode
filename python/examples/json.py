#!/usr/bin/env python
# -*- coding:utf-8 -*-

import qr

qr.qrcode('spam, egg', open('spam' + qr.extension(qr.JSON) , 'w'),
        format = qr.JSON, separator = 0)
