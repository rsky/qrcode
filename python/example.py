import qr;

q = qr.QRCode()
q.add_data('HELLO', qr.EM_ALNUM)
q.finalize()
print(q.get_symbol(format=qr.ASCII, magnify=1))
open('HELLO.json', 'w').write(q.get_symbol(format=qr.JSON, magnify=1))
open('HELLO.png', 'wb').write(q.get_symbol(format=qr.PNG, magnify=10))
