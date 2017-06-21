QT += widgets core
qtHaveModule(printsupport): QT += printsupport

HEADERS       = imageviewer.h
SOURCES       = imageviewer.cpp \
                main.cpp

# install
target.path = C:/Users/Francis2.DESKTOP-ATBE3P4/Documents/Learning/WinApps/imageViewer
INSTALLS += target

DISTFILES += \
    images/open.png
