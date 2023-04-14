.PHONY: build clean mostlyclean
build::
clean:: mostlyclean
mostlyclean:: ;	$(RM) *.o *~ *.bak
PKG_NAMES += glib-2.0
PKG_NAMES += libmce-glib
CPPFLAGS  += -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS    += -Wall -Wextra -Os -g -std=c99
LDFLAGS   += -g
LDLIBS    += -Wl,--as-needed
CFLAGS    += $(shell pkg-config --cflags $(PKG_NAMES))
LDLIBS    += $(shell pkg-config --libs $(PKG_NAMES))
example : example.o
build:: example
clean:: ; $(RM) example
