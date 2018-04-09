TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle
CONFIG -= qt

DEFINES -= UNICODE

SOURCES += \
        main.cpp

LIBS += -lkernel32 -lgdi32 -luser32 -lshell32
LIBS += -L$$(VULKAN_SDK)/Lib/ -lsdl2 -lsdl2main -lvulkan-1

INCLUDEPATH += $$(VULKAN_SDK)/Include
DEPENDPATH += $$(VULKAN_SDK)/Include

HEADERS +=
