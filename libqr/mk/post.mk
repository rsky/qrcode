.c.o: 
	$(CC) $(CFLAGS) -I. $(CPPFLAGS) -fPIC -DPIC $(DEFS) -c -o $@ $<

qr: qrcmd.c $(MYLIBNAME)$(DLL_SUFFIX)
	$(CC) $(CFLAGS) -I. $(CPPFLAGS) $(DEFS) -L. $(LDFLAGS) -lqr $(LIBS) -o $@ qrcmd.c

qrs: qrcmd.c $(MYLIBNAME)$(DLL_SUFFIX)
	$(CC) $(CFLAGS) -I. $(CPPFLAGS) $(DEFS) -L. $(LDFLAGS) -lqr $(LIBS) \
		-DQRCMD_STRUCTURED_APPEND -o $@ qrcmd.c

qr_static: qrcmd.c $(MYOBJECTS)
	$(CC) $(CFLAGS) -I. $(CPPFLAGS) $(DEFS) $(LDFLAGS) $(LIBS) \
		 -DQRCMD_PROG_NAME=\"$@\" -o $@ $(MYOBJECTS) qrcmd.c

qrs_static: qrcmd.c $(MYOBJECTS)
	$(CC) $(CFLAGS) -I. $(CPPFLAGS) $(DEFS) $(LDFLAGS) $(LIBS) \
		-DQRCMD_STRUCTURED_APPEND -DQRCMD_PROG_NAME=\"$@\" -o $@ $(MYOBJECTS) qrcmd.c

$(MYLIBNAME).a: $(MYOBJECTS)
	$(AR) $(ARFLAGS) $(MYLIBNAME).a $(MYOBJECTS)

install: install-bin install-headers install-libs

install-bin: qr qrs
	mkdir -p $(DESTDIR)$(BINDIR)
	cp -Rf qr qrs $(DESTDIR)$(BINDIR)

install-bin_static: qr_static qrs_static
	mkdir -p $(DESTDIR)$(BINDIR)
	cp -Rf qr_static qrs_static $(DESTDIR)$(BINDIR)

install-headers:
	mkdir -p $(DESTDIR)$(INCDIR)
	cp -Rf $(MYHEADERS) $(DESTDIR)$(INCDIR)

install-libs: $(MYLIBNAME).a $(MYLIBNAME)$(DLL_SUFFIX)
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp -Rf $(MYLIBNAME).a $(MYSHAREDLIBS) $(DESTDIR)$(LIBDIR)

uninstall:
	cd $(DESTDIR)$(BINDIR) && rm -f qr qrs qr_static qrs_static
	cd $(DESTDIR)$(LIBDIR) && rm -f $(MYLIBNAME).a $(MYSHAREDLIBS)
	cd $(DESTDIR)$(INCDIR) && rm -f $(MYHEADERS)

clean:
	rm -f qr qrs qr_static qrs_static *.a *.dll *.dylib *.lib *.o *.so *.so.*

.PHONY: clean install*
