# -*- Mode: makefile-gmake -*-

.PHONY: clean all debug release pkgconfig install install-dev

#
# Required packages
#

PKGS = glib-2.0 gio-2.0 gio-unix-2.0 libglibutil

#
# Default target
#

all: debug release pkgconfig

#
# Library version
#

VERSION_MAJOR = 1
VERSION_MINOR = 0
VERSION_RELEASE = 12

# Version for pkg-config
PCVERSION = $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_RELEASE)

NAME = mce-glib
LIB_NAME = lib$(NAME)
LIB_DEV_SYMLINK = $(LIB_NAME).so
LIB_SYMLINK1 = $(LIB_DEV_SYMLINK).$(VERSION_MAJOR)
LIB_SYMLINK2 = $(LIB_SYMLINK1).$(VERSION_MINOR)
LIB_SONAME = $(LIB_SYMLINK1)
LIB = $(LIB_SONAME).$(VERSION_MINOR).$(VERSION_RELEASE)

#
# Sources
#

SRC = \
  mce_battery.c \
  mce_charger.c \
  mce_display.c \
  mce_proxy.c \
  mce_tklock.c
GEN_SRC = \
  com.nokia.mce.request.c \
  com.nokia.mce.signal.c

#
# Directories
#

SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
GEN_DIR = $(BUILD_DIR)
SPEC_DIR = spec
DEBUG_BUILD_DIR = $(BUILD_DIR)/debug
RELEASE_BUILD_DIR = $(BUILD_DIR)/release

#
# Tools and flags
#

CC ?= $(CROSS_COMPILE)gcc
LD = $(CC)
DEFINES += -DGLIB_DISABLE_DEPRECATION_WARNINGS
WARNINGS = -Wall -Wno-unused-parameter -Wno-multichar
INCLUDES = -I$(INCLUDE_DIR) -I$(GEN_DIR)
BASE_FLAGS = -fPIC $(CFLAGS)
FULL_CFLAGS = $(BASE_FLAGS) $(DEFINES) $(WARNINGS) $(INCLUDES) -MMD -MP \
  $(shell pkg-config --cflags $(PKGS))
FULL_LDFLAGS = $(BASE_FLAGS) $(LDFLAGS) -shared -Wl,-soname -Wl,$(LIB_SONAME) \
  -Wl,--version-script=$(LIB_NAME).map
DEBUG_FLAGS = -g
RELEASE_FLAGS =

KEEP_SYMBOLS ?= 0
ifneq ($(KEEP_SYMBOLS),0)
RELEASE_FLAGS += -g
endif

DEBUG_CFLAGS = $(FULL_CFLAGS) $(DEBUG_FLAGS) -DDEBUG
RELEASE_CFLAGS = $(FULL_CFLAGS) $(RELEASE_FLAGS) -O2
DEBUG_LDFLAGS = $(FULL_LDFLAGS) $(DEBUG_FLAGS)
RELEASE_LDFLAGS = $(FULL_LDFLAGS) $(RELEASE_FLAGS)

#
# Files
#

PKGCONFIG = \
  $(BUILD_DIR)/$(LIB_NAME).pc
DEBUG_OBJS = \
  $(GEN_SRC:%.c=$(DEBUG_BUILD_DIR)/%.o) \
  $(SRC:%.c=$(DEBUG_BUILD_DIR)/%.o)
RELEASE_OBJS = \
  $(GEN_SRC:%.c=$(RELEASE_BUILD_DIR)/%.o) \
  $(SRC:%.c=$(RELEASE_BUILD_DIR)/%.o)
GEN_FILES = $(GEN_SRC:%=$(GEN_DIR)/%)
LIBS = $(shell pkg-config --libs $(PKGS))
.PRECIOUS: $(GEN_FILES)

#
# Dependencies
#

DEPS = $(DEBUG_OBJS:%.o=%.d) $(RELEASE_OBJS:%.o=%.d)
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(DEPS)),)
-include $(DEPS)
endif
endif

DEBUG_LIB = $(DEBUG_BUILD_DIR)/$(LIB)
RELEASE_LIB = $(RELEASE_BUILD_DIR)/$(LIB)
DEBUG_LINK = $(DEBUG_BUILD_DIR)/$(LIB_SONAME)
RELEASE_LINK = $(RELEASE_BUILD_DIR)/$(LIB_SONAME)

$(GEN_FILES): | $(GEN_DIR)
$(DEBUG_OBJS): | $(DEBUG_BUILD_DIR) $(GEN_FILES)
$(RELEASE_OBJS): | $(RELEASE_BUILD_DIR) $(GEN_FILES)
$(PKGCONFIG): | $(BUILD_DIR)
$(DEBUG_LINK): | $(DEBUG_LIB)
$(RELEASE_LINK): | $(RELEASE_LIB)

#
# Rules
#

debug: $(DEBUG_LIB) $(DEBUG_LINK)

release: $(RELEASE_LIB) $(RELEASE_LINK)

clean:
	rm -f *~ $(SRC_DIR)/*~ $(INCLUDE_DIR)/*~ rpm/*~
	rm -fr $(BUILD_DIR) RPMS installroot
	rm -fr debian/tmp debian/lib$(NAME) debian/lib$(NAME)-dev
	rm -f documentation.list debian/files debian/*.substvars
	rm -f debian/*.debhelper.log debian/*.debhelper debian/*~
	rm -f debian/*.install

$(GEN_DIR):
	mkdir -p $@

$(DEBUG_BUILD_DIR):
	mkdir -p $@

$(RELEASE_BUILD_DIR):
	mkdir -p $@

$(GEN_DIR)/%.c: $(SPEC_DIR)/%.xml
	gdbus-codegen --generate-c-code $(@:%.c=%) $<

$(DEBUG_BUILD_DIR)/%.o : $(GEN_DIR)/%.c
	$(CC) -c -I. $(DEBUG_CFLAGS) -MT"$@" -MF"$(@:%.o=%.d)" $< -o $@

$(RELEASE_BUILD_DIR)/%.o : $(GEN_DIR)/%.c
	$(CC) -c -I. $(RELEASE_CFLAGS) -MT"$@" -MF"$(@:%.o=%.d)" $< -o $@

$(DEBUG_BUILD_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) -c $(DEBUG_CFLAGS) -MT"$@" -MF"$(@:%.o=%.d)" $< -o $@

$(RELEASE_BUILD_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) -c $(RELEASE_CFLAGS) -MT"$@" -MF"$(@:%.o=%.d)" $< -o $@

$(DEBUG_LIB): $(DEBUG_OBJS)
	$(LD) $(DEBUG_LDFLAGS) -o $@ $^ $(LIBS)

$(RELEASE_LIB): $(RELEASE_OBJS)
	$(LD) $(RELEASE_LDFLAGS) -o $@ $^ $(LIBS)
ifeq ($(KEEP_SYMBOLS),0)
	strip $@
endif

$(DEBUG_LINK):
	ln -sf $(LIB) $@

$(RELEASE_LINK):
	ln -sf $(LIB) $@

# This one could be substituted with arch specific dir
LIBDIR ?= /usr/lib
ABS_LIBDIR := $(shell echo /$(LIBDIR) | sed -r 's|/+|/|g')

pkgconfig: $(PKGCONFIG)

$(PKGCONFIG): $(LIB_NAME).pc.in Makefile
	sed -e 's|@version@|$(PCVERSION)|g' -e 's|@libdir@|$(ABS_LIBDIR)|g' $< > $@

debian/%.install: debian/%.install.in
	sed 's|@LIBDIR@|$(LIBDIR)|g' $< > $@

#
# Install
#

INSTALL = install
INSTALL_DIRS = $(INSTALL) -d
INSTALL_FILES = $(INSTALL) -m 644

INSTALL_LIB_DIR = $(DESTDIR)$(ABS_LIBDIR)
INSTALL_INCLUDE_DIR = $(DESTDIR)/usr/include/$(LIB_NAME)
INSTALL_PKGCONFIG_DIR = $(DESTDIR)$(ABS_LIBDIR)/pkgconfig

install: $(INSTALL_LIB_DIR)
	$(INSTALL) -m 755 $(RELEASE_LIB) $(INSTALL_LIB_DIR)
	ln -sf $(LIB) $(INSTALL_LIB_DIR)/$(LIB_SYMLINK2)
	ln -sf $(LIB_SYMLINK2) $(INSTALL_LIB_DIR)/$(LIB_SYMLINK1)

install-dev: install $(INSTALL_INCLUDE_DIR) $(INSTALL_PKGCONFIG_DIR)
	$(INSTALL_FILES) $(INCLUDE_DIR)/*.h $(INSTALL_INCLUDE_DIR)
	$(INSTALL_FILES) $(PKGCONFIG) $(INSTALL_PKGCONFIG_DIR)
	ln -sf $(LIB_SYMLINK1) $(INSTALL_LIB_DIR)/$(LIB_DEV_SYMLINK)

$(INSTALL_LIB_DIR):
	$(INSTALL_DIRS) $@

$(INSTALL_INCLUDE_DIR):
	$(INSTALL_DIRS) $@

$(INSTALL_PKGCONFIG_DIR):
	$(INSTALL_DIRS) $@
