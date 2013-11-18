#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QMessageBox>
#include <QPushButton>
#include <QFileInfo>
#include <QInputDialog>
#include <QFileDialog>
#include <QSettings>
#include "sliceview.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"


void SliceView::initialize(void)
{

    mean_intensity = std::accumulate(data.begin(),data.end(),0.0)/data.size();
    maximum_intensity = *std::max_element(data.begin(),data.end());
    QString ROIName = FileName+".roi.mat";
    if(QFileInfo(ROIName).exists())
    {
        image::io::mat_read mat_header;
        mat_header.load_from_file(ROIName.toLocal8Bit().begin());
        mat_header >> roi;
    }
    else
    {
        roi.resize(data.geometry());
        std::fill(roi.begin(),roi.end(),0);
    }

    edit_info.resize(data.depth());

    modified = false;
    slice_pos = data.depth() >> 1;

    main_window->ui->SlicePos->setMaximum(data.depth()-1);
    main_window->ui->SlicePos->setValue(data.depth() >> 1);

}
void SliceView::loadFromData(image::basic_image<float,3>& data_,const float* vs,QString FileName_)
{
    data.swap(data_);
    std::copy(vs,vs+3,voxel_size);
    FileName = FileName_;
    initialize();
}

void SliceView::loadFromFile(QString FileName_)
{
    image::io::bruker_2dseq header;
    if(header.load_from_file(FileName_.toLocal8Bit().begin()))
    {
        header >> data;
        header.get_voxel_size(voxel_size);
    }
    else
    {
        image::io::mat_read mat_header;
        if(mat_header.load_from_file(FileName_.toLocal8Bit().begin()))
        {
            mat_header >> data;
            const float* vs = 0;
            unsigned int row,col;
            mat_header.read("voxel_size",row,col,vs);
            if(vs)
                std::copy(vs,vs+3,voxel_size);
            else
                std::fill(voxel_size,voxel_size+3,1.0);
        }
        else
        {
            image::io::bitmap bm_header;
            if(bm_header.load_from_file(FileName_.toLocal8Bit().begin()))
            {
                bm_header >> data;
                std::fill(voxel_size,voxel_size+3,1.0);
            }
            else
            {
                image::io::nifti header;
                if(header.load_from_file(FileName_.toLocal8Bit().begin()))
                {
                    header >> data;
                    header.get_voxel_size(voxel_size);
                    if(header.nif_header.srow_x[0] < 0)
                        image::flip_y(data);
                    else
                        image::flip_xy(data);
                }
                else
                {
                    QMessageBox::information(0,"Error","Invalid file",0);
                    return;
                }
            }
        }
    }

    FileName = FileName_;
    initialize();
}
bool SliceView::checkSave(void)
{
    if(!modified)
        return true;
    switch(QMessageBox::question(0,"T2 Studio","Save ROI?",
        QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel))
    {
        case QMessageBox::Save:
            saveToFile();
            break;
        case QMessageBox::Cancel:
            return false;
    }
    return true;
}

void SliceView::saveToFile(void)
{
    QString ROIName;
    if(FileName.isEmpty())
    {
        QSettings settings;
        ROIName = QFileDialog::getSaveFileName(
                0,
                "Save ROI",
                settings.contains("WORK_PATH") ?
                settings.value("WORK_PATH").toString() :FileName,
                "MAT4 File (*.mat)");
        if(ROIName.isEmpty())
            return;
    }
    else
        ROIName = FileName+".roi.mat";

    image::io::mat_write mat_header(ROIName.toLocal8Bit().begin());
    mat_header << roi;
    modified = false;
}

void SliceView::saveMat4(void)
{
    QSettings settings;
    QString filename = QFileDialog::getSaveFileName(
            0,
            "Export Images",
            settings.contains("WORK_PATH") ?
            settings.value("WORK_PATH").toString():FileName,
            "MAT4 File (*.mat)");
    if( filename.isEmpty() )
        return;
    image::io::mat_write mat_header(filename.toLocal8Bit().begin());
    mat_header << data;
    mat_header.write("voxel_size",voxel_size,1,3);
}

void SliceView::saveBitmap(void)
{
    QSettings settings;
    QString filename = QFileDialog::getSaveFileName(
            0,"Export Images",
            settings.contains("WORK_PATH") ?
            settings.value("WORK_PATH").toString():FileName,
            "Bitmap File (*.bmp)");
    if( filename.isEmpty() )
        return;
    image::basic_image<float,2> buf(image::geometry<2>(data.width(),data.height()));
    std::copy(data.slice_at(slice_pos).begin(),
              data.slice_at(slice_pos).begin()+buf.size(),
              buf.begin());
    image::normalize(buf);
    image::io::bitmap bitmap_header;
    bitmap_header << buf;
    bitmap_header.save_to_file(filename.toLocal8Bit().begin());
}

void SliceView::saveBitmapAll(void)
{
    QSettings settings;
    QString filename = QFileDialog::getSaveFileName(
            0,"Export Images",
            settings.contains("WORK_PATH") ?
            settings.value("WORK_PATH").toString():FileName,
            "Bitmap File (*.bmp)");
    if( filename.isEmpty() )
        return;
    for(int index = 0;index < data.depth();++index)
    {
        image::basic_image<float,3> buf(data);
        image::normalize(buf);
        image::io::bitmap bitmap_header;
        bitmap_header << buf.slice_at(index);
        bitmap_header.save_to_file((filename+QString::number(index)+".bmp").toLocal8Bit().begin());
    }
}

void SliceView::showSlide(void)
{
    if(!data.size())
        return;
    image::basic_image<float,2> buffer;
    image::reslicing(data,buffer,2,slice_pos);

    float brightness = main_window->ui->brightness->value();
    float dynamic_range = maximum_intensity > mean_intensity ?
                          main_window->ui->dynamic_range->value()/
                          (maximum_intensity-mean_intensity) : 1.0;
    slice_image.resize(buffer.geometry());
    for(unsigned int index = 0;index < buffer.size();++index)
    {
        float value = buffer[index];
        value -= mean_intensity;
        value *= dynamic_range;
        value += brightness;
        value = std::floor(value);
        if(value < 0.0)
            value = 0.0;
        if(value > 255.0)
            value = 255.0;
        slice_image[index] = (unsigned char)value;
    }

    unsigned char mask = 1 << main_window->active_region();
    unsigned int base_index = slice_pos*roi.plane_size();

    if(main_window->ui->showBlend->isChecked() ||
       main_window->ui->showMask->isChecked())
    {
        bool show_mask = main_window->ui->showMask->isChecked();
        for(unsigned int index = 0;index < slice_image.size();++index)
        {
            unsigned char roi_value = roi[base_index+index];
            if(!roi_value)
                continue;
            if(show_mask)
                slice_image[index] = 0;
            if(roi_value & mask)
                slice_image[index].r = 255;
            else
                slice_image[index].b = 255;
        }
    }


    float display_ratio = main_window->ui->display_ratio->value();
    QImage qimage((unsigned char*)&*slice_image.begin(),slice_image.width(),slice_image.height(),QImage::Format_RGB32);
    view_image = qimage.scaled(slice_image.width()*display_ratio,slice_image.height()*display_ratio);

    if(main_window->ui->showBoundary->isChecked())
    {
        QPainter paint(&view_image);
        paint.setBrush(Qt::NoBrush);
        for(int y = 1,index = slice_image.width();
                             y < slice_image.height()-1;++y)
        for(int x = 0;x < slice_image.width();++x,++index)
        {
            if(x == 0 || x+1 >= slice_image.width() || !roi[base_index+index])
                continue;
            unsigned int cur_index = base_index+index;
            float xd = x*display_ratio;
            float xd_1 = xd+display_ratio;
            float yd = y*display_ratio;
            float yd_1 = yd+display_ratio;
            for(unsigned char region = 0;region <= 8;++region)
            {
                paint.setPen((region == 8)? Qt::red : Qt::blue);
                unsigned char cur_mask = 1 << ((region == 8) ? main_window->active_region():region);
                if(!(roi[cur_index] & cur_mask))
                    continue;
                bool upper_edge = !(roi[cur_index-slice_image.width()] & cur_mask);
                bool lower_edge = !(roi[cur_index+slice_image.width()] & cur_mask);
                bool left_edge = !(roi[cur_index-1] & cur_mask);
                bool right_edge = !(roi[cur_index+1] & cur_mask);
                // upper edge
                if(upper_edge)
                    paint.drawLine(xd+(left_edge ? 1 : 0),yd+1,
                                   xd_1+(right_edge ? -1 : 0),yd+1);

                if(lower_edge)
                    paint.drawLine(xd+(left_edge ? 1 : 0),yd_1-1,
                                   xd_1+(right_edge ? -1 : 0),yd_1-1);

                // left edge
                if(left_edge)
                    paint.drawLine(xd+1,yd+(upper_edge? 1:0),
                                   xd+1,yd_1+(lower_edge? -1:0));

                if(right_edge)
                    paint.drawLine(xd_1-1,yd+(upper_edge? 1:0),
                                   xd_1-1,yd_1+(lower_edge? -1:0));
            }
        }
    }
    setSceneRect(0, 0, view_image.width(),view_image.height());
    clear();
    setItemIndexMethod(QGraphicsScene::NoIndex);
    addRect(0, 0, view_image.width(),view_image.height(),
            QPen(),view_image);

}

void SliceView::mousePressEvent ( QGraphicsSceneMouseEvent * mouseEvent )
{
    if (mouseEvent->button() == Qt::LeftButton)
    {
        float Y = mouseEvent->scenePos().y();
        float X = mouseEvent->scenePos().x();
        if(edit_option == 1)
            sel_point.clear();
        sel_point.push_back(image::vector<2,short>(X, Y));
        mouse_down = true;
    }
}

void SliceView::mouseMoveEvent ( QGraphicsSceneMouseEvent * mouseEvent )
{
    if (!mouse_down || edit_option == 2)
        return;
    float Y = mouseEvent->scenePos().y();
    float X = mouseEvent->scenePos().x();
    cX = X;
    cY = Y;
    QImage annotated_image = view_image;
    QPainter paint(&annotated_image);

    paint.setPen(Qt::red);
    paint.setBrush(Qt::NoBrush);

    sel_point.push_back(image::vector<2,short>(X, Y));
    for (size_t index = 1; index < sel_point.size(); ++index)
        paint.drawLine(sel_point[index-1][0],
                       sel_point[index-1][1],
                       sel_point[index][0],
                       sel_point[index][1]);
    clear();
    setItemIndexMethod(QGraphicsScene::NoIndex);
    addRect(0, 0, annotated_image.width(),annotated_image.height(),
            QPen(),annotated_image);
}

void SliceView::mouseReleaseEvent ( QGraphicsSceneMouseEvent * mouseEvent )
{
    float display_ratio = main_window->ui->display_ratio->value();

    if (mouseEvent->button() == Qt::RightButton)
    {
        if(edit_option == 2 && !sel_point.empty())
        {
            sel_point.pop_back();
            if(!sel_point.empty());
                sel_point.pop_back();
            edit_option = 1;
            mouseMoveEvent(mouseEvent);
            edit_option = 2;
            return;
        }

        float Y = mouseEvent->scenePos().y()/display_ratio;
        float X = mouseEvent->scenePos().x()/display_ratio;
        unsigned int index =
                image::pixel_index<3>(X,Y,(float)slice_pos,roi.geometry()).index();
        if(index >= roi.size())
            return;
        unsigned int value = roi[index];
        unsigned int mask = 1 << main_window->active_region();
        if(value & mask)
            value -= mask;
        for(unsigned int shift = 0;shift < main_window->region_count;++shift)
            if(value & (1 << shift))
                main_window->Rs[shift]->setChecked(true);
        return;
    }

    if(edit_option == 2)
    {
        edit_option = 1;
        mouseMoveEvent(mouseEvent);
        edit_option = 2;
    }
    else
    if (mouse_down)
    {
        mouse_down = false;
        unsigned char label = (mouseEvent->modifiers() & Qt::ShiftModifier) ? 0:1;
        std::vector<unsigned int> pos;
        // click mode
        if(sel_point.size() <= 4)
        {
            unsigned int index = image::pixel_index<3>(
                sel_point[0][0]/display_ratio,
                sel_point[0][1]/display_ratio,(float)slice_pos,roi.geometry()).index();
            if(index >= roi.size())
            return;
            pos.push_back(index);
            perform(label,main_window->active_region(),pos);
        }
        else
        {
            QImage bitmap(slice_image.width()*display_ratio,
                  slice_image.height()*display_ratio,QImage::Format_Mono),roi_map;
            QPainter paint(&bitmap);
            paint.setBrush(Qt::black);
            paint.drawRect(0,0,bitmap.width(),bitmap.height());
            paint.setBrush(Qt::white);
            std::vector<QPoint> qpoints(sel_point.size());
            for(size_t index = 0;index < sel_point.size();++index)
                qpoints[index] = QPoint(sel_point[index][0],sel_point[index][1]);
            paint.drawPolygon(&*qpoints.begin(),qpoints.size() - 1);
            roi_map = bitmap.scaled(slice_image.width(),slice_image.height());

            if(main_window->ui->rb2Dseries->isChecked())//2D series
            {
                for(unsigned int base_index = 0;base_index < data.size();base_index += roi.plane_size())
                for (image::pixel_index<2>index;
                     index.is_valid(slice_image.geometry());
                     index.next(slice_image.geometry()))
                    if (QColor(roi_map.pixel(index.x(),index.y())).red() > 128)
                        pos.push_back(index.index()+base_index);
            }
            else
            {
                unsigned int base_index = slice_pos*roi.plane_size();
                for (image::pixel_index<2>index;
                     index.is_valid(slice_image.geometry());
                     index.next(slice_image.geometry()))
                    if (QColor(roi_map.pixel(index.x(),index.y())).red() > 128)
                        pos.push_back(index.index()+base_index);
            }

            perform(label,main_window->active_region(),pos);
        }
        showSlide();
    }
}

void SliceView::mouseDoubleClickEvent ( QGraphicsSceneMouseEvent * mouseEvent )
{
    edit_option = 1;
    mouse_down = true;
    mouseMoveEvent(mouseEvent);
    mouseReleaseEvent(mouseEvent);
    edit_option = 2;
    sel_point.clear();
}


void SliceView::perform(unsigned char action,unsigned char shift,
             const std::vector<unsigned int>& pos)
{
    unsigned char mask = 1 << shift;
    std::vector<unsigned int> active_pos;
    for(unsigned int index = 0;index < pos.size();++index)
        if(action)// set label
        {
            if(!(roi[pos[index]] & mask))
            {
                active_pos.push_back(pos[index]);
                roi[pos[index]] |= mask;
            }
        }
        else
        if(roi[pos[index]] & mask)
            {
                roi[pos[index]] -= mask;
                active_pos.push_back(pos[index]);
            }

    edit_info[slice_pos].edit_pos.push_back(active_pos);
    edit_info[slice_pos].edit_action.push_back(action);
    edit_info[slice_pos].edit_shift.push_back(shift);
    modified = true;
}

void SliceView::undo(void)
{
    if(edit_info[slice_pos].edit_action.empty())
        return;
    unsigned int mask = 1 << edit_info[slice_pos].edit_shift.back();
    unsigned char action = edit_info[slice_pos].edit_action.back();
    std::vector<unsigned int>& edit_pos = edit_info[slice_pos].edit_pos.back();
    for(unsigned int index = 0;index < edit_pos.size();++index)
        if(!action)
            roi[edit_pos[index]] |= mask;
        else
        if(roi[edit_pos[index]] & mask)
            roi[edit_pos[index]] -= mask;
    edit_info[slice_pos].edit_pos.pop_back();
    edit_info[slice_pos].edit_action.pop_back();
    edit_info[slice_pos].edit_shift.pop_back();
    modified = true;
}



SliceView::~SliceView(void)
{
    if(modified)
    {
        if(QMessageBox::question(0,"T2 Studio","Save ROI?",
            QMessageBox::Save|QMessageBox::Discard) == QMessageBox::Save)
            saveToFile();
    }
}

template<typename ImageType,typename ROIType>
bool process_image_volume(ImageType& data,
                          ROIType& roi,QString id,const std::vector<int>& value,QStringList& result)
{
    image::basic_image<unsigned char,ImageType::dimension> label;
    if(id == "watershed")
    {
        image::filter::mean(data);
        image::filter::sobel(data);
        image::filter::mean(data);
        image::segmentation::watershed(data,label);
        image::segmentation::resample(data,label,data);
        return true;
    }
    if(id == "blur")
    {
        image::filter::mean(data);
        return true;
    }
    if(id == "sobel")
    {
        image::filter::sobel(data);
        return true;
    }
    if(id == "kmeans")
    {
        label.resize(roi.geometry());
        if(value[0] < 2)
        {
            result << "Invalid cluster size";
            return true;
        }
        int cluster_num = value[0];
        image::ml::k_means<float,unsigned char> k(cluster_num);
        k(data.begin(),data.end(),label.begin());
    }
    if(id == "graph_cut")
    {
        label.resize(roi.geometry());
        image::segmentation::graph_cut(data,label,(float)value[0]/10.0,value[1]);
    }

    if(label.empty())
        return false;

    std::vector<unsigned int> count(*std::max_element(label.begin(),label.end())+1);
    for (size_t index = 0;index < data.width();++index)
    {
        ++count[label[index]];
        ++count[label[label.size()-1-index]];
    }
    unsigned int background_index = std::max_element(count.begin(),count.end())-count.begin();
    std::fill(roi.begin(),roi.end(),0);
    for (unsigned int index = 0;index < data.size();++index)
    {
        if(label[index] == background_index)
        {
            roi[index] = 0;
            continue;
        }
        if(label[index] < background_index)
            roi[index] = (1 << label[index]);
        else
            roi[index] = (1 << label[index]) >> 1;
    }
    return true;
}

template<typename ImageType,typename ROIType>
bool process_image_pair(ImageType& data,
                        ROIType& roi,QString id,const std::vector<int>& value,QStringList& result)
{
    if(value.size() < 2)
         return false;
    unsigned char mask1 = 1 << value[0];
    unsigned char mask2 = 1 << value[1];
    if(id == "and")
    {
        for(unsigned int index = 0;index < roi.size();++index)
            if((roi[index] & mask1) && !(roi[index] & mask2))
                roi[index] -= mask1;
        return true;
    }
    if(id == "nand")
    {
        for(unsigned int index = 0;index < roi.size();++index)
            if((roi[index] & mask1) && (roi[index] & mask2))
                roi[index] -= mask1;
        return true;
    }
    if(id == "or")
    {
        for(unsigned int index = 0;index < roi.size();++index)
            if(!(roi[index] & mask1) && (roi[index] & mask2))
                roi[index] |= mask1;
        return true;
    }
    if(id == "nor")
    {
        for(unsigned int index = 0;index < roi.size();++index)
            if(!(roi[index] & mask1) && !(roi[index] & mask2))
                roi[index] |= mask1;
        return true;
    }
    if(id == "copy")
    {
        for(unsigned int index = 0;index < roi.size();++index)
        {
            if((roi[index] & mask1) && !(roi[index] & mask2))
                roi[index] -= mask1;
            else
            if(!(roi[index] & mask1) && (roi[index] & mask2))
                roi[index] |= mask1;
        }
        return true;
    }
    return false;
}

template<typename ImageType,typename ROIType>
bool process_image_single(ImageType& data,
                          ROIType& roi,QString id,const std::vector<int>& value,QStringList& result)
{
    if(value.empty())
        return false;
    unsigned char mask = 1 << value[0];
    image::basic_image<unsigned char,ROIType::dimension> cur_roi(roi.geometry());
    for(unsigned int index = 0;index < cur_roi.size();++index)
        cur_roi[index] = (roi[index] & mask) ? 1:0;
    bool processed = false;
    if(id == "smoothing")
    {
        image::morphology::smoothing(cur_roi);
        processed = true;
    }
    if(id == "defragment")
    {
        image::morphology::defragment(cur_roi);
        processed = true;
    }
    if(id == "erosion")
    {
        image::morphology::erosion(cur_roi);
        processed = true;
    }
    if(id == "erosion2")
    {
        image::morphology::erosion2(cur_roi,value[1] == 0 ? 3:value[1]);
        processed = true;
    }
    if(id == "dilation")
    {
        image::morphology::dilation(cur_roi);
        processed = true;
    }
    if(id == "dilation2")
    {
        image::morphology::dilation2(cur_roi,value[1] == 0 ? 3:value[1]);
        processed = true;
    }
    if(id == "opening")
    {
        image::morphology::opening(cur_roi);
        processed = true;
    }
    if(id == "closing")
    {
        image::morphology::closing(cur_roi);
        processed = true;
    }
    if(id == "edge")
    {
        image::morphology::edge(cur_roi);
        processed = true;
    }

    if(id == "closing1")
    {
        image::morphology::closing(cur_roi,-1);
        processed = true;
    }
    if(id == "convex")
    {
        image::morphology::convex_xy(cur_roi);
        processed = true;
    }
    if(id == "convex_x")
    {
        image::morphology::convex_x(cur_roi);
        processed = true;
    }
    if(id == "convex_y")
    {
        image::morphology::convex_y(cur_roi);
        processed = true;
    }
    if(id == "negate")
    {
        image::negate(cur_roi,1);
        processed = true;
    }

    if(id == "otsu")
    {
        float threshold = image::segmentation::otsu_threshold(data);
        for(unsigned int index = 0;index < data.size();++index)
            cur_roi[index] = data[index] > threshold ? 1:0;
        processed = true;
    }
    if(id == "threshold")
    {
        float threshold = value[1];
        threshold *= *std::max_element(data.begin(),data.end());
        threshold /= 100.0;
        for(unsigned int index = 0;index < data.size();++index)
            cur_roi[index] = data[index] > threshold ? 1:0;
        processed = true;
    }
    if(id == "fast_marching")
    {
        for(image::pixel_index<ROIType::dimension> index;
            index.is_valid(cur_roi.geometry());
            index.next(cur_roi.geometry()))
            if(cur_roi[index.index()])
            {
                image::filter::mean(data);
                image::filter::gradient_magnitude(data);
                image::basic_image<float,ROIType::dimension> time_data;
                image::segmentation::fast_marching(data,time_data,index);
                std::copy(time_data.begin(),time_data.end(),data.begin());
                processed = true;
                break;
            }
        if(!processed)
        {
            result << "no seed point assigned";
            return true;
        }

    }
    if(id == "refine")
    {
        if(*std::max_element(cur_roi.begin(),cur_roi.end()) == 0)
        {
            result << "no initial region assigned";
            return true;
        }
        image::segmentation::stochastic_competition(data,cur_roi,
                value[1] == 0 ? 30:value[1],  //Zc
                value[2] == 0 ? 5:value[2]);  //Zr
        processed = true;
    }

    if(id == "cca")
    {
            std::vector<size_t> hist(value[1]);
            std::vector<std::vector<unsigned int> > regions;
            image::basic_image<unsigned int,ImageType::dimension> label;
            image::morphology::connected_component_labeling(cur_roi,label,regions);
            for(size_t index = 0;index < regions.size();++index)
                if(!regions[index].empty() && regions[index].size() < hist.size())
                    hist[regions[index].size()]++;

            for(unsigned int index = 1;index < hist.size();++index)
                result << QString::number(hist[index]);
            processed = true;

    }
    if(id == "coordinate")
    {
        for(image::pixel_index<ROIType::dimension> index;
            index.is_valid(cur_roi.geometry());
            index.next(cur_roi.geometry()))
            if(cur_roi[index.index()])
            {
                QString coo = QString::number(index[0]);
                for(char dim = 1;dim < ROIType::dimension;++dim)
                    coo += QString(" %1").arg(index[1]);
                result << coo;
            }
        processed = true;
    }
    if(id == "mean")
    {
        double sum = 0.0;
        unsigned int total_pixels = 0;
        for(unsigned int index = 0;index < data.size();++index)
            if(cur_roi[index])
            {
                sum += data[index];
                ++total_pixels;
            }
        result <<  QString::number(sum / total_pixels);
        processed = true;

    }
    if(id == "size")
    {
        unsigned int count = 0;
        for(unsigned int index = 0;index < cur_roi.size();++index)
            if(cur_roi[index])
                ++count;
        result << QString::number(count);
        processed = true;

    }

    if(id == "clear")
    {
        std::fill(cur_roi.begin(),cur_roi.end(),0);
        processed = true;

    }

    if(!processed)
        return false;

    for(unsigned int index = 0;index < cur_roi.size();++index)
        if(cur_roi[index])
            roi[index] |= mask;
        else
        if(roi[index] & mask)
            roi[index] -= mask;
    return true;
}
template<typename ImageType,typename ROIType>
bool process_image(ImageType& data,
                   ROIType& roi,QString id,const std::vector<int>& value,QStringList& result)
{
    if(!process_image_volume(data,roi,id,value,result) &&
       !process_image_pair(data,roi,id,value,result) &&
       !process_image_single(data,roi,id,value,result))
    {
        result << "unknown command";
    }
    return false;
}

void SliceView::perform_action(QString id,const std::vector<int>& value)
{
    if(id == "2d")// switch to 2D
    {
        main_window->ui->rb2D->setChecked(true);
        return;
    }
    if(id == "2ds")// switch to 2D
    {
        main_window->ui->rb2Dseries->setChecked(true);
        return;
    }
    if(id == "3d")// switch to 3D
    {
        main_window->ui->rb3D->setChecked(true);
        return;
    }
    if(id == "push")
    {
        back_up_data.push_back(data);
        return;
    }
    if(id == "pop")
    {
        if(back_up_data.empty())
        {
            main_window->ui->textEdit->append("empty");
            return;
        }
        back_up_data.back().swap(data);
        back_up_data.pop_back();
        showSlide();
        return;
    }
    QStringList result;
    if(main_window->ui->rb2D->isChecked())//2D
    {
        unsigned int cur_slice = slice_pos;
        {
            image::basic_image<unsigned char,3>::slice_type roi_slice = roi.slice_at(cur_slice);
            image::basic_image<float,3>::slice_type data_slice = data.slice_at(cur_slice);
            process_image(data_slice,roi_slice,id,value,result);
        }
    }
    else
    if(main_window->ui->rb2Dseries->isChecked())//2D Series
    {
        for(unsigned int cur_slice = main_window->ui->rb2D->isChecked() ? slice_pos:0
                            ;cur_slice < data.depth();++cur_slice)
        {
            image::basic_image<unsigned char,3>::slice_type roi_slice = roi.slice_at(cur_slice);
            image::basic_image<float,3>::slice_type data_slice = data.slice_at(cur_slice);
            process_image(data_slice,roi_slice,id,value,result);

        }
    }
    else// 3D
        process_image(data,roi,id,value,result);


    edit_info.clear();
    edit_info.resize(data.depth());
    modified = true;
    for(unsigned int index = 0;index < result.count();++index)
        main_window->ui->textEdit->append(result[index]);
    showSlide();
}

void SliceView::action(void)
{
    QPushButton* source = (QPushButton*)sender();
    std::vector<int> value(6);
    value[0] = main_window->active_region();
    QString cmd = "-";
    cmd += source->text();
    cmd += " ";
    cmd += QString::number(value[0]);
    cmd += " ";
    cmd += main_window->ui->arg->text();
    std::istringstream arg_in(main_window->ui->arg->text().toLocal8Bit().begin());
    for(int i = 1;i < value.size() && arg_in;++i)
        arg_in >> value[i];
    main_window->ui->textEdit->append(cmd);
    perform_action(source->text(),value);
}
#include <sstream>
void SliceView::actions(QStringList act_list)
{
    for(int index = 0;index < act_list.count();++index)
    {
        main_window->ui->textEdit->append(act_list[index]);
        std::istringstream cmd_line(act_list[index].toLocal8Bit().begin());
        std::string cmd;
        int repeat = 1;
        std::vector<int> value;
        cmd_line >> cmd;
        if(!cmd.length() || cmd[0] != '-')
            continue;
        if(cmd == "-repeat")
        {
            cmd_line >> repeat >> cmd;
            cmd = std::string("-")+cmd;
        }
        std::copy(std::istream_iterator<int>(cmd_line),
                  std::istream_iterator<int>(),
                  std::back_inserter(value));
        value.resize(6);
        for(int index2 = 0;index2 < repeat;++index2)
            perform_action(cmd.c_str()+1,value);
    }
}

