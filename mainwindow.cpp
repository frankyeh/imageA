#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include "filebrowser.h"
#include <sstream>
#include <fstream>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "image/image.hpp"
#include <QProgressDialog>
extern std::auto_ptr<QProgressDialog> progressDialog;
extern QString app_dir;
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),scene(this)
{
    progressDialog.reset(new QProgressDialog);
    ui->setupUi(this);
    ui->graphicsView->setScene(&scene);

    Rs[0] = ui->R0;
    Rs[1] = ui->R1;
    Rs[2] = ui->R2;
    Rs[3] = ui->R3;
    Rs[4] = ui->R4;
    Rs[5] = ui->R5;
    Rs[6] = ui->R6;
    Rs[7] = ui->R7;
    Rs[8] = ui->R8;
    Rs[9] = ui->R9;
    for (int i = 0; i < 10; ++i)
        connect(Rs[i],SIGNAL(clicked(bool)),&scene,SLOT(showSlide()));

    connect(ui->action_Open,SIGNAL(triggered()),this,SLOT(open_file()));
    connect(ui->actionSave,SIGNAL(triggered()),&scene,SLOT(saveToFile()));
    connect(ui->actionMAT4,SIGNAL(triggered()),&scene,SLOT(saveMat4()));
    connect(ui->actionBitmap,SIGNAL(triggered()),&scene,SLOT(saveBitmap()));
    connect(ui->actionBitmap_series,SIGNAL(triggered()),&scene,SLOT(saveBitmapAll()));

    connect(ui->actionUndo,SIGNAL(triggered()),&scene,SLOT(undo()));
    connect(ui->actionUndo,SIGNAL(triggered()),&scene,SLOT(showSlide()));

    // Examination
    connect(ui->brightness,SIGNAL(valueChanged(int)),&scene,SLOT(showSlide()));
    connect(ui->dynamic_range,SIGNAL(valueChanged(int)),&scene,SLOT(showSlide()));
    connect(ui->display_ratio,SIGNAL(valueChanged(int)),&scene,SLOT(showSlide()));
    connect(ui->showNone,SIGNAL(clicked()),&scene,SLOT(showSlide()));
    connect(ui->showBoundary,SIGNAL(clicked()),&scene,SLOT(showSlide()));
    connect(ui->showMask,SIGNAL(clicked()),&scene,SLOT(showSlide()));
    connect(ui->showBlend,SIGNAL(clicked()),&scene,SLOT(showSlide()));


    // actions
    QStringList cmd_list;
    cmd_list << "smoothing"
             << "defragment"
             << "erosion"
             << "dilation"
             << "edge"
             << "negate"
             << "kmeans"
             << "otsu"
             << "threshold"
             << "refine"
             << "cca"
             << "size"
             << "and"
             << "or"
             << "and_negate"
             << "copy"
             << "push"
             << "pop"
             << "clear";
    for (int i = 0; i < cmd_list.size(); ++i)
    {
        QPushButton* new_pb = new QPushButton(this);
        new_pb->setText(cmd_list[i]);
        connect(new_pb,SIGNAL(clicked()),&scene,SLOT(action()));
        ui->actionLayout->addWidget(new_pb,i/4,i%4);
    }

    for (int i = 0; i < MaxRecentFiles; ++i)
    {
        recentFileActs[i] = new QAction(this);
        recentFileActs[i]->setVisible(false);
        connect(recentFileActs[i], SIGNAL(triggered()),
            this, SLOT(openRecentFile()));
        ui->menuRecent_Files->addAction(recentFileActs[i]);
    }


    QSettings settings;
    updateRecentList(settings.value("recentFileList").toStringList());
    ui->brightness->setValue(settings.value("brightness",0).toInt());
    ui->dynamic_range->setValue(settings.value("dynamic_range",128).toInt());
    ui->display_ratio->setValue(settings.value("display_ratio",5).toInt());

    // find all the scripts
    {
        QDir directory = QDir(QFileInfo(app_dir).absoluteDir().absolutePath() + "/scripts");
        script_list = directory.entryList(QStringList("*.txt"),QDir::Files | QDir::NoSymLinks);
        for(int index = 0;index < script_list.count();++index)
        {
            ui->scriptBox->addItem(script_list[index]);
            script_list[index] = directory.absolutePath() + "/" + script_list[index];
        }
    }

    qApp->installEventFilter(this);
}

MainWindow::~MainWindow()
{
    QSettings settings;
    settings.setValue("brightness",ui->brightness->value());
    settings.setValue("dynamic_range",ui->dynamic_range->value());
    settings.setValue("display_ratio",ui->display_ratio->value());
    delete ui;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{

  if (event->type() == QEvent::MouseMove &&
      obj->parent() && obj->parent()->objectName() == QString("graphicsView"))
  {

      QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
      QPointF point = ui->graphicsView->mapToScene(mouseEvent->pos().x(),mouseEvent->pos().y());


      QString status;
      int pos[3];
      pos[0] = std::floor(((float)point.x()+0.5)/ui->display_ratio->value());
      pos[1] = std::floor(((float)point.y()+0.5)/ui->display_ratio->value());
      pos[2] = ui->SlicePos->value();
      status = QString("(%1,%2,%3) ").arg(pos[0]).arg(pos[1]).arg(pos[2]);
      if(!scene.data.empty()&& scene.data.geometry().is_valid(pos))
          status += QString(" %1").arg(scene.data.at(pos[0],pos[1],pos[2]));
      ui->statusBar->showMessage(status);
  }
  return false;
}
void MainWindow::updateRecentList(QStringList files)
{
    // remove those files no readable
    for(size_t index = 0;index < files.count();)
        if(!QFileInfo(files[index]).isReadable())
            files.removeAt(index);
        else
            ++index;

    int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);
    for (int i = 0; i < numRecentFiles; ++i)
    {
        recentFileActs[i]->setText(QString("&%1 %2").arg(i + 1).arg(files[i]));
        recentFileActs[i]->setData(files[i]);
        recentFileActs[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
        recentFileActs[j]->setVisible(false);
}

void MainWindow::openFile(QString filename)
{
    if(!scene.checkSave())
        return;
    scene.loadFromFile(filename);
    setWindowTitle(QString("ImageA ") + __DATE__ + " build - " + filename);

    // update recent file list
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll(filename);
    files.prepend(filename);
    while (files.size() > MaxRecentFiles)
         files.removeLast();
    settings.setValue("recentFileList", files);
    updateRecentList(files);
}

void MainWindow::open_file()
{
    QString filename = QFileDialog::getOpenFileName(
            this,
            "Open Images files",
            QDir::currentPath(),
            "Bruker files(2dseq subject *.mat);;All files (*.*)" );
    if( filename.isEmpty() )
        return;

    if(QFileInfo(filename).baseName() == "subject")
    {
        if(!browse_files.get())
            browse_files.reset(new FileBrowser(this));
        if(browse_files->exec() == QDialog::Rejected)
            return;
        filename = browse_files->image_file_name;
    }
    openFile(filename);
}

void MainWindow::openRecentFile(void)
{
    if(!scene.checkSave())
        return;
    QAction *action = qobject_cast<QAction *>(sender());
    setWindowTitle(QString("ImageA ") + __DATE__ + " build - " + action->data().toString());
    scene.loadFromFile(action->data().toString());
}

void MainWindow::on_SlicePos_valueChanged(int value)
{
    if(!scene.data.size())
        return;
    scene.sel_point.clear();
    scene.slice_pos = value;
    scene.showSlide();
}

void MainWindow::on_tool1_clicked()
{
    scene.sel_point.clear();
    scene.edit_option = 1;
}

void MainWindow::on_tool2_clicked()
{
    scene.sel_point.clear();
    scene.edit_option = 2;
}

void MainWindow::on_applyBatch_clicked()
{
    std::istringstream string_list(
            ui->batchActions->toPlainText().toLocal8Bit().begin());
    QStringList cmd_list;
    std::string line;
    while(std::getline(string_list,line))
        cmd_list.append(line.c_str());
    scene.actions(cmd_list);
}

void MainWindow::on_applyBatch2_clicked()
{
    std::istringstream string_list(
            ui->batchActions2->toPlainText().toLocal8Bit().begin());
    QStringList cmd_list;
    std::string line;
    while(std::getline(string_list,line))
        cmd_list.append(line.c_str());
    scene.actions(cmd_list);
}
void MainWindow::on_applyBatch3_clicked()
{
    std::istringstream string_list(
            ui->batchActions3->toPlainText().toLocal8Bit().begin());
    QStringList cmd_list;
    std::string line;
    while(std::getline(string_list,line))
        cmd_list.append(line.c_str());
    scene.actions(cmd_list);
}
void MainWindow::on_applyScript_clicked()
{
    std::ifstream in(script_list[ui->scriptBox->currentIndex()].toLocal8Bit().begin());
    QString cmd_list;
    std::string line;
    while(std::getline(in,line))
    {
        if(line[0] != '-')
            continue;
        cmd_list += line.c_str();
        cmd_list += "\n";
    }
    ui->batchActions->clear();
    ui->batchActions->setPlainText(cmd_list);
}

void MainWindow::on_actionBrowse_triggered()
{
    if(!browse_files.get())
        browse_files.reset(new FileBrowser(this));
    if(browse_files->exec() == QDialog::Rejected || !scene.checkSave())
        return;
    scene.loadFromData(browse_files->data,browse_files->voxel_size,browse_files->image_file_name);
    setWindowTitle(QString("ImageA ") + __DATE__ + " build - " + browse_files->study_name + " at " + browse_files->image_file_name);

}



void MainWindow::on_clearlog_clicked()
{
    ui->textEdit->clear();
}
