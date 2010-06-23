#!/usr/bin/env python

import qr

qr.qrcode('spam, egg', open('spam' + qr.extension(qr.JSON) , 'w'),
        format = qr.JSON, separator = 0)
