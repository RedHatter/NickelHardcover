include ./NickelHook/NickelHook.mk

override LIBRARY  := libhardcover.so
override SOURCES  += $(wildcard src/*.cc src/**/*.cc qrcodegen/c/qrcodegen.c res/*.svg)
override CFLAGS   += -Wall -Wextra -Werror -Wno-missing-field-initializers -fvisibility=hidden -Iqrcodegen/c
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers -isystemminiz -fvisibility=hidden -fvisibility-inlines-hidden -Iqrcodegen/c
override PKGCONF  += Qt5Widgets Qt5Network
override MOCS 	  += $(wildcard src/*.h src/**/*.h)
override QRCS     += res/nickelhardcover.qrc

PNG := $(filter %.png,$(SOURCES:%.svg=%.png))
override GENERATED += $(PNG)

res/nickelhardcover.rcc: $(PNG)

%.png: %.svg
	resvg "$<" "$@"

include ./NickelHook/NickelHook.mk
