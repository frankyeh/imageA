#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QRadioButton>
#include "sliceview.h"
namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    Ui::MainWindow *ui;
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool eventFilter(QObject *obj, QEvent *event);
    static const int region_count = 10;
    QRadioButton* Rs[region_count];
    int active_region(void)
    {
        for(int index = 0;index < region_count;++index)
            if(Rs[index]->isChecked())
                return index;
        return 0;
    }

private:
    SliceView scene;
    QStringList script_list;
private:
    enum { MaxRecentFiles = 10 };
    QAction *recentFileActs[MaxRecentFiles];
    void updateRecentList(QStringList files);
    void openFile(QString);
private slots:
    void on_applyBatch3_clicked();
    void on_applyBatch2_clicked();
    void on_actionBrowse_triggered();
    void on_applyScript_clicked();
    void on_applyBatch_clicked();
    void on_tool2_clicked();
    void on_tool1_clicked();
    void on_SlicePos_valueChanged(int value);
    void open_file(void);
    void openRecentFile(void);

    void on_clearlog_clicked();
};

#endif // MAINWINDOW_H
