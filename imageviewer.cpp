/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>
#ifndef QT_NO_PRINTER
#include <QPrintDialog>
#endif
#include <QDir>
#include <QDebug>
#include <QStringList>
#include <QDateTime>
#include <QTimer>
#include <QProcess>


#include "imageviewer.h"

//! [0]
ImageViewer::ImageViewer()
   : imageLabel(new QLabel)
   , scrollArea(new QScrollArea)
   , scaleFactor(1)
{
    qDebug() << "In ImageViewer";

    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setScaledContents(true);

    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(imageLabel);
    scrollArea->setVisible(false);
    scrollArea->setWidgetResizable(true);
    setCentralWidget(scrollArea);
    createActions();

    resize(QGuiApplication::primaryScreen()->availableSize() * 1 / 5);
    showMenu = false;
    menuBar()->hide();
    statusBar()->hide();
    pauseDisplay = false;
    pauseDisplayPerm = false;
    idleCount = 0;
    prevCount = 0;
    QDateTime now = QDateTime::currentDateTime();
    int l_seed = (now.toMSecsSinceEpoch() % RAND_MAX);
    qsrand(l_seed);
    readSettings();
    if (! sourcepath.isEmpty()) {
        buildFileList(sourcepath);
        pickFile();
    }
    else {
        showMenu = true;
        menuBar()->show();
        statusBar()->show();
        pauseDisplay = true;
    }
    timer = NULL;
    startDisplayLoop();

}

bool ImageViewer::loadFile(const QString &fileName)
{

    int pLength = prevList.length();
    while (pLength > 20) {
        prevList.removeLast();
        pLength = prevList.length();
    }
  //  for (int x = 0; x<prevList.length();x++){
  //      qDebug() << "x=" << x << "File=" << prevList[x];
  //  }
    currFileName = fileName;
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QImage newImage = reader.read();
    if (newImage.isNull()) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot load %1: %2")
                                 .arg(QDir::toNativeSeparators(fileName), reader.errorString()));
        return false;
    }
//! [2]

    double m_size_factor = (double)((double)this->width() / (double)newImage.width());
    int new_height = (int) ((double)newImage.height() * m_size_factor);
    resize(width(),new_height);

    setImage(newImage);

    setWindowFilePath(fileName);

    const QString message = tr("Opened \"%1\", %2x%3, Depth: %4")
        .arg(QDir::toNativeSeparators(fileName)).arg(image.width()).arg(image.height()).arg(image.depth());
    statusBar()->showMessage(message);

    return true;
}

void ImageViewer::setImage(const QImage &newImage)
{
    image = newImage;
    imageLabel->setPixmap(QPixmap::fromImage(image));
//! [4]
    scaleFactor = 1.0;

    scrollArea->setVisible(true);
//    printAct->setEnabled(true);
//    fitToWindowAct->setEnabled(true);
//    updateActions();

//    if (!fitToWindowAct->isChecked())
//        imageLabel->adjustSize();
}

//! [4]

bool ImageViewer::saveFile(const QString &fileName)
{
    QImageWriter writer(fileName);

    if (!writer.write(image)) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot write %1: %2")
                                 .arg(QDir::toNativeSeparators(fileName)), writer.errorString());
        return false;
    }
    const QString message = tr("Wrote \"%1\"").arg(QDir::toNativeSeparators(fileName));
    statusBar()->showMessage(message);
    return true;
}

//! [1]

static void initializeImageFileDialog(QFileDialog &dialog, QFileDialog::AcceptMode acceptMode)
{
    static bool firstDialog = true;

    if (firstDialog) {
        firstDialog = false;
        const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
    }

    QStringList mimeTypeFilters;
    const QByteArrayList supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen
        ? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();
    foreach (const QByteArray &mimeTypeName, supportedMimeTypes)
        mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/jpeg");
    if (acceptMode == QFileDialog::AcceptSave)
        dialog.setDefaultSuffix("jpg");
}

void ImageViewer::openDir()
{
    qDebug() << "In openDir" << endl;
    sourcepath = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                 ".",
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
    qDebug() << "Got" << sourcepath << endl;
    buildFileList(sourcepath);
    //QFileDialog dialog(this, tr("Open File"));
    //initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);

    //while (dialog.exec() == QDialog::Accepted && !loadFile(dialog.selectedFiles().first())) {}
}


void ImageViewer::open()
{
    QFileDialog dialog(this, tr("Open File"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);

    while (dialog.exec() == QDialog::Accepted && !loadFile(dialog.selectedFiles().first())) {}
}
//! [1]

void ImageViewer::saveAs()
{
    QFileDialog dialog(this, tr("Save File As"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptSave);

    while (dialog.exec() == QDialog::Accepted && !saveFile(dialog.selectedFiles().first())) {}
}

//! [5]
void ImageViewer::print()
//! [5] //! [6]
{
    Q_ASSERT(imageLabel->pixmap());
#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
//! [6] //! [7]
    QPrintDialog dialog(&printer, this);
//! [7] //! [8]
    if (dialog.exec()) {
        QPainter painter(&printer);
        QRect rect = painter.viewport();
        QSize size = imageLabel->pixmap()->size();
        size.scale(rect.size(), Qt::KeepAspectRatio);
        painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
        painter.setWindow(imageLabel->pixmap()->rect());
        painter.drawPixmap(0, 0, *imageLabel->pixmap());
    }
#endif
}
//! [8]

void ImageViewer::copy()
{
#ifndef QT_NO_CLIPBOARD
    QGuiApplication::clipboard()->setImage(image);
#endif // !QT_NO_CLIPBOARD
}

#ifndef QT_NO_CLIPBOARD
static QImage clipboardImage()
{
    if (const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData()) {
        if (mimeData->hasImage()) {
            const QImage image = qvariant_cast<QImage>(mimeData->imageData());
            if (!image.isNull())
                return image;
        }
    }
    return QImage();
}
#endif // !QT_NO_CLIPBOARD

void ImageViewer::paste()
{
#ifndef QT_NO_CLIPBOARD
    const QImage newImage = clipboardImage();
    if (newImage.isNull()) {
        statusBar()->showMessage(tr("No image in clipboard"));
    } else {
        setImage(newImage);
        setWindowFilePath(QString());
        const QString message = tr("Obtained image from clipboard, %1x%2, Depth: %3")
            .arg(newImage.width()).arg(newImage.height()).arg(newImage.depth());
        statusBar()->showMessage(message);
    }
#endif // !QT_NO_CLIPBOARD
}

//! [9]
void ImageViewer::zoomIn()
//! [9] //! [10]
{
    scaleImage(1.25);
}

void ImageViewer::zoomOut()
{
    scaleImage(0.8);
}

//! [10] //! [11]
void ImageViewer::normalSize()
//! [11] //! [12]
{
    imageLabel->adjustSize();
    scaleFactor = 1.0;
}
//! [12]

//! [13]
void ImageViewer::fitToWindow()
//! [13] //! [14]
{
    qDebug() << "Fit to window clicked";
    bool fitToWindow = fitToWindowAct->isChecked();
    scrollArea->setWidgetResizable(fitToWindow);
    if (!fitToWindow)
        normalSize();
    updateActions();
}
//! [14]


//! [15]
void ImageViewer::about()
//! [15] //! [16]
{
    QMessageBox::about(this, tr("About Image Viewer"),
            tr("<p>The <b>Image Viewer</b> example shows how to combine QLabel "
               "and QScrollArea to display an image. QLabel is typically used "
               "for displaying a text, but it can also display an image. "
               "QScrollArea provides a scrolling view around another widget. "
               "If the child widget exceeds the size of the frame, QScrollArea "
               "automatically provides scroll bars. </p><p>The example "
               "demonstrates how QLabel's ability to scale its contents "
               "(QLabel::scaledContents), and QScrollArea's ability to "
               "automatically resize its contents "
               "(QScrollArea::widgetResizable), can be used to implement "
               "zooming and scaling features. </p><p>In addition the example "
               "shows how to use QPainter to print an image.</p>"));
}
//! [16]

//! [17]
void ImageViewer::createActions()
//! [17] //! [18]
{

/*
//    QAction *hideAct = actionMenu->addAction(tr("&Hide"), this, &ImageViewer::hide);
    hideAct = actionMenu->addAction(tr("&Hide"), this, &ImageViewer::hide);
    hideAct->setShortcut(tr("Ctrl+H"));

//    QAction *prevAct = actionMenu->addAction(tr("&Prev"), this, &ImageViewer::prev);
    prevAct = actionMenu->addAction(tr("&Prev"), this, &ImageViewer::prev);
    prevAct->setShortcut(tr("Ctrl+P"));
*/
//    const QIcon newIcon = QIcon::fromTheme("document-new", QIcon(":/images/open.png"));
//    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

//    QAction *openDirAct = fileMenu->addAction(tr("&Open sourcepath..."), this, &ImageViewer::openDir);
//    openDirAct->setShortcut(tr("Ctrl+D"));

//    QAction *openAct = fileMenu->addAction(tr("&Open..."), this, &ImageViewer::open);
//    openAct->setShortcut(QKeySequence::Open);

//    saveAsAct = fileMenu->addAction(tr("&Save As..."), this, &ImageViewer::saveAs);
//    saveAsAct->setEnabled(false);

//    printAct = fileMenu->addAction(tr("&Print..."), this, &ImageViewer::print);
//    printAct->setShortcut(QKeySequence::Print);
//    printAct->setEnabled(false);

//    fileMenu->addSeparator();

//    QAction *exitAct = fileMenu->addAction(tr("E&xit"), this, &QWidget::close);
//    exitAct->setShortcut(tr("Ctrl+Q"));
/*

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));

    copyAct = editMenu->addAction(tr("&Copy"), this, &ImageViewer::copy);
    copyAct->setShortcut(QKeySequence::Copy);
    copyAct->setEnabled(false);

    QAction *pasteAct = editMenu->addAction(tr("&Paste"), this, &ImageViewer::paste);
    pasteAct->setShortcut(QKeySequence::Paste);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    changeFileAct = viewMenu->addAction(tr("Change"), this, &ImageViewer::changeFile);

    zoomInAct = viewMenu->addAction(tr("Zoom &In (25%)"), this, &ImageViewer::zoomIn);
    zoomInAct->setShortcut(QKeySequence::ZoomIn);
    zoomInAct->setEnabled(false);

    zoomOutAct = viewMenu->addAction(tr("Zoom &Out (25%)"), this, &ImageViewer::zoomOut);
    zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    zoomOutAct->setEnabled(false);

    normalSizeAct = viewMenu->addAction(tr("&Normal Size"), this, &ImageViewer::normalSize);
    normalSizeAct->setShortcut(tr("Ctrl+S"));
    normalSizeAct->setEnabled(false);

    viewMenu->addSeparator();

    fitToWindowAct = viewMenu->addAction(tr("&Fit to Window"), this, &ImageViewer::fitToWindow);
    fitToWindowAct->setEnabled(false);
    fitToWindowAct->setCheckable(true);
    fitToWindowAct->setShortcut(tr("Ctrl+F"));
*/
/*    QMenu *actionMenu = menuBar()->addMenu(tr("&Actions"));

    actionMenu->addAction(tr("&Open File"), this, &ImageViewer::openFileInPhotos);
    actionMenu->addAction(tr("&Open Folder"), this, &ImageViewer::openFolderInExplorer);
    actionMenu->addAction(tr("&Prev"), this, &ImageViewer::prev);
    actionMenu->addAction(tr("&Next"), this, &ImageViewer::next);
    actionMenu->addAction(tr("&Info"), this, &ImageViewer::showFileInfo);
    actionMenu->addAction(tr("&Resume"), this, &ImageViewer::hide);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));

    helpMenu->addAction(tr("&About"), this, &ImageViewer::about);
    helpMenu->addAction(tr("About &Qt"), &QApplication::aboutQt);
*/
    openDirAct = menuBar()->addAction(tr("&Open"));
    openDirAct->setShortcut(tr("Ctrl+O"));
    openDirAct->setStatusTip(tr("Open source folder of images"));
    connect(openDirAct, &QAction::triggered, this, &ImageViewer::openDir);

    prevAct = menuBar()->addAction(tr("&Prev"));
    prevAct->setShortcut(tr("Ctrl+P"));
    prevAct->setStatusTip(tr("Show previous image"));
    connect(prevAct, &QAction::triggered, this, &ImageViewer::prev);

    nextAct = menuBar()->addAction(tr("&Next"));
    nextAct->setShortcut(tr("Ctrl+N"));
    nextAct->setStatusTip(tr("Show next image"));
    connect(nextAct, &QAction::triggered, this, &ImageViewer::next);

    pauseAct = menuBar()->addAction(tr("&Pause"));
    pauseAct->setShortcut(tr(" "));
    pauseAct->setStatusTip(tr("Pause display"));
    connect(pauseAct, &QAction::triggered, this, &ImageViewer::pause);

    resumeAct = menuBar()->addAction(tr("&Resume"));
    resumeAct->setShortcut(tr(" "));
    resumeAct->setStatusTip(tr("Resume display"));
    connect(resumeAct, &QAction::triggered, this, &ImageViewer::resume);

    infoAct = menuBar()->addAction(tr("&Info"));
    infoAct->setShortcut(tr("Ctrl+I"));
    infoAct->setStatusTip(tr("Show file info"));
    connect(infoAct, &QAction::triggered, this, &ImageViewer::showFileInfo);

    editAct = menuBar()->addAction(tr("&Edit"));
    editAct->setShortcut(tr("Ctrl+E"));
    editAct->setStatusTip(tr("Edit file in Windows Photo app"));
    connect(editAct, &QAction::triggered, this, &ImageViewer::openFileInPhotos);

    exploreAct = menuBar()->addAction(tr("&Explore"));
    exploreAct->setShortcut(tr("Ctrl+X"));
    exploreAct->setStatusTip(tr("View folder in Windows Explorer"));
    connect(exploreAct, &QAction::triggered, this, &ImageViewer::openFolderInExplorer);

    decreaseAct = menuBar()->addAction(tr("Dec"));
    decreaseAct->setStatusTip(tr("Decrease time delay by 1 second"));
    connect(decreaseAct, &QAction::triggered, this, &ImageViewer::decreaseDelay);

    increaseAct = menuBar()->addAction(tr("Inc"));
    increaseAct->setStatusTip(tr("Increase time delay by 1 second"));
    connect(increaseAct, &QAction::triggered, this, &ImageViewer::increaseDelay);

    setDelayAct = menuBar()->addAction(tr("Set Delay"));
    setDelayAct->setStatusTip(tr("Directly set the time delay"));
    connect(setDelayAct, &QAction::triggered, this, &ImageViewer::setDelay);

    quitAct = menuBar()->addAction(tr("&Quit"));
    quitAct->setShortcut(tr("Ctrl-Q"));
    quitAct->setStatusTip(tr("Quit"));
    connect(quitAct, &QAction::triggered, this, &QWidget::close);




}
//! [18]

//! [21]
void ImageViewer::updateActions()
//! [21] //! [22]
{
//    saveAsAct->setEnabled(!image.isNull());
//    copyAct->setEnabled(!image.isNull());
//    zoomInAct->setEnabled(!fitToWindowAct->isChecked());
//    zoomOutAct->setEnabled(!fitToWindowAct->isChecked());
//    normalSizeAct->setEnabled(!fitToWindowAct->isChecked());
}
//! [22]

//! [23]
void ImageViewer::scaleImage(double factor)
//! [23] //! [24]
{
    Q_ASSERT(imageLabel->pixmap());
    scaleFactor *= factor;
    imageLabel->resize(scaleFactor * imageLabel->pixmap()->size());

    adjustScrollBar(scrollArea->horizontalScrollBar(), factor);
    adjustScrollBar(scrollArea->verticalScrollBar(), factor);

    zoomInAct->setEnabled(scaleFactor < 3.0);
    zoomOutAct->setEnabled(scaleFactor > 0.333);
}
//! [24]

//! [25]
void ImageViewer::adjustScrollBar(QScrollBar *scrollBar, double factor)
//! [25] //! [26]
{
    scrollBar->setValue(int(factor * scrollBar->value()
                            + ((factor - 1) * scrollBar->pageStep()/2)));
}
//! [26]
void ImageViewer::buildFileList(const QString &sourcepath){
    qDebug() << "In buildFileList with" << sourcepath;
    qDebug() << "fileList len before =" << fileList.length();

    findRecursion(sourcepath, QStringLiteral("*.jpg"), &fileList);
    qDebug() << "fileList len after =" << fileList.length();
    //loadFile(fileList.at(3));
}

void ImageViewer::findRecursion(const QString &path, const QString &pattern, QStringList *result)
{
    QDir currentDir(path);
    const QString prefix = path + QLatin1Char('/');
    foreach (const QString &match, currentDir.entryList(QStringList(pattern), QDir::Files | QDir::NoSymLinks))
        result->append(prefix + match);
    foreach (const QString &sourcepath, currentDir.entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot))
        findRecursion(prefix + sourcepath, pattern, result);
}

void ImageViewer::closeEvent(QCloseEvent *event)
{
    qDebug() << "in closeEvent";

        writeSettings();
        event->accept();
}

void ImageViewer::writeSettings()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.setValue("sourcepath", sourcepath);
    settings.setValue("position", pos());
    settings.setValue("size", size());
    settings.setValue("delay", delay);
}

void ImageViewer::readSettings()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    sourcepath = settings.value("sourcepath").toString();
    delay = settings.value("delay", 4000).toInt();
    move(settings.value("position", QPoint(800, 10)).toPoint());
    resize(settings.value("size", QSize(200, 200)).toSize());
}

void ImageViewer::pickFile()
{
    int i = fileList.length() * (double)((double)qrand() / (double)RAND_MAX);
//  qDebug() << "l=" << fileList.length() << "i = " << i;
//    qDebug() << "file = " << fileList.at(i);
    if (currFileName != "") {
        prevList.push_front(currFileName);
    }
    loadFile(fileList.at(i));

}

void ImageViewer::changeFile()
{
    if (showMenu && (! pauseDisplayPerm) ) {
        idleCount++;
        if (idleCount >= 3) {
            showMenu = false;
            pauseDisplay = false;
            menuBar()->hide();
            statusBar()->hide();
            this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
            show();

        }
    }
    if ( (! pauseDisplay) && (! pauseDisplayPerm) ) {
        pickFile();
    }
}

void ImageViewer::startDisplayLoop()
{
    if (timer != NULL) {
        timer->stop();
    }
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(changeFile()));
    timer->start(delay);
}


void ImageViewer::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - m_dragPosition);
        idleCount = 0;
        event->accept();
    }

}
void ImageViewer::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        idleCount = 0;
        event->accept();
    }

    if (event->button() == Qt::RightButton) {
        menuBar()->show();
        pauseDisplay = true;
        idleCount = 0;
        event->accept();
    }
}
void ImageViewer::pause() {
    qDebug("in pause");
    menuBar()->show();
    statusBar()->show();
    show();
    showMenu = true;
    pauseDisplay = true;
    pauseDisplayPerm = true;
    idleCount = 0;

}
void ImageViewer::resume() {
    qDebug("in resume");
    menuBar()->hide();
    statusBar()->hide();
    //show();
    showMenu = false;
    pauseDisplay = false;
    pauseDisplayPerm = false;
    idleCount = 0;
    pickFile();
    prevCount = 0;
}

void ImageViewer::prev() {
    qDebug() << "in prev count" << prevCount << "of" << prevList.length();
    pauseDisplay = true;
    pauseDisplayPerm = true;
    qDebug() << "pl0" << prevList.at(0);
    qDebug() << "cFN" << currFileName;
    if ((prevCount == 0) && (prevList.at(0) != currFileName)){
        prevList.push_front(currFileName);
        prevCount++;
    }
    idleCount = 0;
    if (prevCount >= prevList.length() ) {
        prevCount = prevList.length() - 1;
    }
    loadFile(prevList.at(prevCount));
    prevCount++;

}

void ImageViewer::next() {
    qDebug() << "in next count" << prevCount;
    pauseDisplay = true;
    pauseDisplayPerm = true;
    idleCount = 0;
    prevCount--;
    if (prevCount <0) {
        prevCount = 0;
        pickFile();
    }
    else {
        loadFile(prevList.at(prevCount));
    }
}

void ImageViewer::openFolderInExplorer() {
    pauseDisplayPerm = true;
    QFileInfo fInfo(currFileName);
    QString path = fInfo.absolutePath();
    path.replace("/","\\");
    QString cmd = "explorer";
    QStringList parameters;
    parameters << path;
    QProcess *myProcess = new QProcess(this);
    myProcess->start(cmd, parameters);
}

void ImageViewer::openFileInPhotos() {
    pauseDisplayPerm = true;
    QString tmpFileName = currFileName;
    QString cmd = "explorer";
    QStringList parameters;
    tmpFileName.replace("/","\\");
    parameters << tmpFileName;
    QProcess *myProcess = new QProcess(this);
    myProcess->start(cmd, parameters);
}

void ImageViewer::enterEvent(QEvent *event){
    qDebug() << "In enter mb" << menuBar()->isVisible();
    if ( ! menuBar()->isVisible() ) {
        menuBar()->show();
        statusBar()->show();
        show();
        showMenu = true;
        pauseDisplay = true;
       // pauseDisplayPerm = true;
        idleCount = 0;
    }
    event->accept();
}

void ImageViewer::leaveEvent(QEvent *event){
    qDebug() << "In leave mb" << menuBar()->isVisible() << imageLabel->underMouse() << scrollArea->underMouse() << menuBar()->underMouse();
    event->accept();
}
void ImageViewer::showFileInfo() {

    qDebug() << "In showFileInfo";
    QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("%1")
                                 .arg(QDir::toNativeSeparators(currFileName)));
    QClipboard *clipboard = QGuiApplication::clipboard();
//    QString originalText = clipboard->text();
    clipboard->setText(currFileName);

    //        return false;
}

void ImageViewer::decreaseDelay() {

    qDebug() << "In decDelay";
    delay -= 1000;
    if (delay < 1000) {
        delay = 1000;
    }
    statusBar()->showMessage(tr("Delay is %1 seconds").arg((int) delay/1000));
    startDisplayLoop();
}

void ImageViewer::increaseDelay() {

    qDebug() << "In incDelay";
    delay += 1000;
    if (delay > 60000) {
        delay = 60000;
    }
    statusBar()->showMessage(tr("Delay is %1 seconds").arg((int) delay/1000));
    startDisplayLoop();
}

void ImageViewer::setDelay() {
    qDebug() << "in setDelay";
    bool ok;
    int i = QInputDialog::getInt(this, tr("Set delay in seconds"),
                                 tr("Seconds:"), (int) delay/1000, 1, 60, 1, &ok);
    if (ok) {
        if (i < 1) {
            i = 1;
        }
        if (i > 60) {
            i = 60;
        }
        delay = i * 1000;
        statusBar()->showMessage(tr("Delay is %1 seconds").arg((int) delay/1000));
        startDisplayLoop();
    }
}
