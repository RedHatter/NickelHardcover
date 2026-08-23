include ./NickelHook/NickelHook.mk

override LIBRARY  := libhardcover.so
override SOURCES  += $(wildcard src/*.cc src/**/*.cc res/*.svg)
override CFLAGS   += -Wall -Wextra -Werror -Wno-missing-field-initializers -fvisibility=hidden
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers -isystemminiz -fvisibility=hidden -fvisibility-inlines-hidden
override PKGCONF  += Qt5Widgets Qt5Network
override MOCS 	  += $(wildcard src/*.h src/**/*.h)
override QRCS     += res/nickelhardcover.qrc

PNG := $(filter %.png,$(SOURCES:%.svg=%.png))
override GENERATED += $(PNG)

res/nickelhardcover.rcc: $(PNG)

%.png: %.svg
	resvg "$<" "$@"

include ./NickelHook/NickelHook.mk
