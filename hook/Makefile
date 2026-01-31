include ./NickelHook/NickelHook.mk

override LIBRARY  := libhardcover.so
override SOURCES  += $(wildcard src/*.cc)
override CFLAGS   += -Wall -Wextra -Werror -Wno-missing-field-initializers -fvisibility=hidden
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers -isystemminiz -fvisibility=hidden -fvisibility-inlines-hidden
override PKGCONF  += Qt5Widgets
override MOCS 	  += $(wildcard src/*.h)

include ./NickelHook/NickelHook.mk
