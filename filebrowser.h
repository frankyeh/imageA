#ifndef FILEBROWSER_H
#define FILEBROWSER_H

#include <QDialog>
#include <QGraphicsScene>
#include <memory>
#include <image/image.hpp>
#include <QTableWidget>
#include <boost/thread.hpp>

namespace Ui {
    class FileBrowser;
}
class FileBrowser : public QDialog
{
    Q_OBJECT
    QGraphicsScene scene;
public:
    unsigned int cur_z;
    float voxel_size[3];
    image::basic_image<float,3> data;
    image::basic_image<unsigned char,3> mask;
    image::color_image slice_image;
    QImage view_image;
public:
    explicit FileBrowser(QWidget *parent);
    ~FileBrowser();
    QString image_file_name,study_name;
    std::map<std::string,std::string> mon_map;
private:
    Ui::FileBrowser *ui;
    QTimer* timer;
    QStringList image_list;
    std::vector<float> te_list,tr_list,fa_list;
    std::vector<std::string> method_list;
    std::vector<std::string> method_file_list;
    void prepare_images(std::vector<image::basic_image<float,3> >& image_data,
                        image::basic_image<unsigned char,3>& trimmed_mask,
                        std::vector<float>& tes,std::vector<float>& trs,std::vector<float>& fas);
private:
    QString preview_file_name;
    float preview_voxel_size[3];
    bool preview_loaded;
    image::basic_image<float,3> preview_data;
    std::auto_ptr<boost::thread> preview_thread;
    void preview_image(QString file_name);
private slots:

    void on_Combine_clicked();
    void on_tableWidget_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
    void on_loadT1_clicked();
    void on_defragment_clicked();
    void on_dilation_clicked();
    void on_erosion_clicked();
    void on_smoothing_clicked();
    void on_threshold_clicked();
    void on_DefaultSegmentation_clicked();
    void on_loadT2_clicked();
    void on_changeWorkDir_clicked();
    void on_subject_list_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

    void on_refresh_list_clicked();

    void on_buttonBox_accepted();

    void on_loadR2_clicked();

public slots:
    void show_image();
    void populateDirs();
};

#endif // FILEBROWSER_H
