include ./NickelHook/NickelHook.mk

override LIBRARY  := libhardcover.so
override SOURCES  += $(wildcard src/*.cc src/**/*.cc)
override CFLAGS   += -Wall -Wextra -Werror -Wno-missing-field-initializers -fvisibility=hidden
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers -isystemminiz -fvisibility=hidden -fvisibility-inlines-hidden -DVERSION='"$(VERSION)"'
override PKGCONF  += Qt5Widgets Qt5Network
override MOCS 	  += $(wildcard src/*.h src/**/*.h)
override QRCS     += res/nickelhardcover.qrc

include ./NickelHook/NickelHook.mk
