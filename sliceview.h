#ifndef SLICEVIEW_H
#define SLICEVIEW_H
#include <QGraphicsScene>
#include <vector>
#include "image/image.hpp"

struct EditInfo{
    std::vector<std::vector<unsigned int> > edit_pos;
    std::vector<unsigned char> edit_action;
    std::vector<unsigned char> edit_shift;
};

class MainWindow;
class SliceView : public QGraphicsScene
{
    Q_OBJECT
public:
    QString FileName;
    float voxel_size[3];
    std::vector<image::basic_image<float,3> > back_up_data;
    image::basic_image<float,3> data;
    image::basic_image<unsigned char,3> roi;
    float mean_intensity;
    float maximum_intensity;
    void loadFromFile(QString FileName);
    void loadFromData(image::basic_image<float,3>& data,const float* vs,QString FileName);
public:
    image::color_image slice_image;
    unsigned int slice_pos;
    QImage view_image;
    std::vector<image::vector<2,short> >sel_point;
    bool mouse_down;
    int cX, cY;
    unsigned char edit_option;
public:
    std::vector<EditInfo> edit_info;
    bool modified;

    void perform(unsigned char action,unsigned char shift,
                 const std::vector<unsigned int>& pos);
protected:
    void mouseDoubleClickEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    void mousePressEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    void mouseMoveEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    void mouseReleaseEvent ( QGraphicsSceneMouseEvent * mouseEvent );
private:
    void perform_action(QString id,const std::vector<int>& value);

    void initialize(void);
public slots:
    void showSlide(void);
    void saveToFile(void);
    void undo(void);
    void action(void);
    void saveMat4(void);
    void saveBitmap(void);
    void saveBitmapAll(void);
    bool checkSave(void);
public:
    MainWindow* main_window;
    SliceView(MainWindow* main_window_):
            slice_pos(0),main_window(main_window_),
            modified(false),edit_option(1)
    {

    }
    ~SliceView(void);
    void actions(QStringList act_list);
};

#endif // SLICEVIEW_H
