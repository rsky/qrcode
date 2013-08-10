#!/usr/bin/env python
# -*- coding:utf-8 -*-

import qr, cStringIO
from PIL import Image

buf = cStringIO.StringIO(qr.qrcode('spam, egg', format=qr.BMP, scale=2))
Image.open(buf).save('spam.gif', 'GIF')
