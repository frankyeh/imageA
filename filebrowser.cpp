#include <QDir>
#include <QTimer>
#include <QMessageBox>
#include <QSettings>
#include <QFileDialog>
#include <QInputDialog>
#include "image/image.hpp"
#include "filebrowser.h"
#include "ui_filebrowser.h"
#include "prog_interface_static_link.h"
FileBrowser::FileBrowser(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FileBrowser)
{
    ui->setupUi(this);
    ui->graphicsView->setScene(&scene);
    ui->graphicsView->setMaximumHeight(10);
    ui->tableWidget->setColumnWidth(0,40);
    ui->tableWidget->setColumnWidth(1,40);
    ui->tableWidget->setColumnWidth(2,100);
    ui->tableWidget->setColumnWidth(3,90);
    ui->tableWidget->setColumnWidth(4,130);
    ui->tableWidget->setColumnWidth(5,60);
    ui->tableWidget->setColumnWidth(6,60);
    ui->tableWidget->setColumnWidth(7,60);
    ui->tableWidget->setColumnWidth(8,60);
    ui->tableWidget->setColumnWidth(9,100);

    ui->subject_list->setColumnWidth(0,110);
    ui->subject_list->setColumnWidth(1,80);
    ui->subject_list->setColumnWidth(2,110);
    ui->tableWidget->setStyleSheet("selection-background-color: blue");
    ui->subject_list->setStyleSheet("selection-background-color: blue");

    mon_map["Jan"] = "01";
    mon_map["Feb"] = "02";
    mon_map["Mar"] = "03";
    mon_map["Apr"] = "04";
    mon_map["May"] = "05";
    mon_map["Jun"] = "06";
    mon_map["Jul"] = "07";
    mon_map["Aug"] = "08";
    mon_map["Sep"] = "09";
    mon_map["Oct"] = "10";
    mon_map["Nov"] = "11";
    mon_map["Dec"] = "12";

    QSettings settings;
    ui->WorkDir->setText(settings.value("WORK_PATH",QDir::currentPath()).toString());
    populateDirs();
    //if(ui->listWidget->count())
    //    ui->listWidget->setCurrentRow(
    //            settings.value("WORK_PATH_INDEX",0).toInt());

    //if(ui->tableWidget->rowCount())
    //    ui->tableWidget->selectRow(
    //            settings.value("WORK_PATH_INDEX2",0).toInt());
    timer = new QTimer(this);
    timer->start(100);
    connect(timer, SIGNAL(timeout()), this, SLOT(show_image()));
}

FileBrowser::~FileBrowser()
{
    QSettings settings;
    settings.setValue("WORK_PATH", ui->WorkDir->text());
    //settings.setValue("WORK_PATH_INDEX", ui->subject_list->currentRow());
    //settings.setValue("WORK_PATH_INDEX2", ui->tableWidget->currentRow());
    delete ui;
}



void FileBrowser::on_subject_list_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    if(currentRow == -1 || currentRow == previousRow)
        return;
    QStringList params1,params2;
    params1 << "Method" << "PVM_EchoTime" << "PVM_RepetitionTime" << "PVM_SliceThick";
    params2 << "IMND_method_display_name" << "IMND_echo_time" << "IMND_rep_time" << "IMND_slice_thick";
    QDir directory = ui->WorkDir->text() + "/" +
                     ui->subject_list->item(currentRow,0)->text();
    study_name = ui->subject_list->item(currentRow,1)->text();
    // find all the acquisition
    data.clear();
    ui->tableWidget->clear();
    QStringList header;
    header << "#" << "reco" << "Sequence" << "Image size" << "Resolution(mm)" << "FOV(cm)" << "TE" << "TR" << "FA";
    ui->tableWidget->setHorizontalHeaderLabels(header);
    ui->tableWidget->setRowCount(0);
    image_list.clear();
    method_list.clear();
    method_file_list.clear();
    te_list.clear();
    tr_list.clear();
    fa_list.clear();

    std::vector<unsigned char> seq_list;
    {
        QStringList seq_txt_list = directory.entryList(QStringList("*"),QDir::Dirs | QDir::NoSymLinks);
        for(int index = 0;index < seq_txt_list.count();++index)
        {
            int seq = QString(seq_txt_list[index]).toInt();
            if(seq)
                seq_list.push_back(seq);
        }
    }

    std::sort(seq_list.begin(),seq_list.end());
    for(int index = 0;index < seq_list.size();++index)
    {
        image::io::bruker_info method,acq,reco_file,d3proc_file;
        bool method_file = true;
        QString method_dir = directory.absolutePath() + "/" + QString::number(seq_list[index]);
        QString method_file_name =  method_dir+ "/method";
        QString acq_file_name =  method_dir+ "/acqp";
        if(!method.load_from_file(method_file_name.toLocal8Bit().begin()))
        {
            method_file_name =  method_dir+ "/imnd";
            if(!method.load_from_file(method_file_name.toLocal8Bit().begin()))
                continue;
            method_file = false;
        }
        acq.load_from_file(acq_file_name.toLocal8Bit().begin());
        // search for all reco
        QStringList params = method_file ? params1:params2;
        for(int reco = 1;;++reco)
        {
            QString reco_dir = method_dir + "/pdata/" + QString::number(reco);
            if(!QDir(reco_dir).exists())
                break;
            QString cur_file_name = reco_dir + "/2dseq";
            QString cur_d3proc_name = reco_dir + "/d3proc";
            QString cur_reco_name = reco_dir + "/reco";
            if(!QFileInfo(cur_file_name).exists() ||
               !reco_file.load_from_file(cur_reco_name.toLocal8Bit().begin()))
                continue;
            int row = ui->tableWidget->rowCount();
            ui->tableWidget->setRowCount(row+1);

            QStringList item_list;
            item_list << QString::number(seq_list[index]);
            item_list << QString::number(reco);
            item_list << method[params[0].toLocal8Bit().begin()].c_str(); // sequence

            if(d3proc_file.load_from_file(cur_d3proc_name.toLocal8Bit().begin()))
                item_list << (d3proc_file["IM_SIX"] + " / " + d3proc_file["IM_SIY"] + " / "+ d3proc_file["IM_SIZ"]).c_str();
            else
                item_list << reco_file["RECO_size"].c_str();

            // output resolution
            {
                std::istringstream size_in(reco_file["RECO_size"].c_str());
                std::istringstream fov_in(reco_file["RECO_fov"].c_str());
                std::vector<float> data1((std::istream_iterator<float>(size_in)),std::istream_iterator<float>());
                std::vector<float> data2((std::istream_iterator<float>(fov_in)),std::istream_iterator<float>());
                std::ostringstream out;
                for(int index = 0;index < data1.size() && index < data2.size();++index)
                    out << data2[index]*10.0/data1[index] << " / ";
                out << method[params[3].toLocal8Bit().begin()].c_str(); // slice thickness
                item_list << out.str().c_str();
            }
            item_list << reco_file["RECO_fov"].c_str(); // FOV
            item_list << method[params[1].toLocal8Bit().begin()].c_str(); // TE
            item_list << method[params[2].toLocal8Bit().begin()].c_str(); // TR
            item_list << acq["ACQ_flip_angle"].c_str(); //FA


            for(int col = 0;col < item_list.count();++col)
                ui->tableWidget->setItem(row, col, new QTableWidgetItem(item_list[col]));


            image_list << cur_file_name;
            method_list.push_back(method["Method"]);
            method_file_list.push_back(method_file_name.toLocal8Bit().begin());
            te_list.push_back(item_list[3].toFloat());
            tr_list.push_back(item_list[4].toFloat());
            fa_list.push_back(QString(acq["ACQ_flip_angle"].c_str()).toFloat());
        }
        qApp->processEvents();
        if(ui->tableWidget->rowCount() == 0)
            return;
        if(index == 0)
            ui->tableWidget->selectRow(0);
    }

}

void FileBrowser::populateDirs(void)
{
    ui->subject_list->clear();
    QStringList header;
    header << "Directory" << "Name" << "Date";
    ui->subject_list->setHorizontalHeaderLabels(header);
    ui->subject_list->setRowCount(0);

    ui->tableWidget->clear();
    QDir directory = ui->WorkDir->text();
    QStringList script_list = directory.entryList(QStringList("*"),QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    ui->subject_list->setSortingEnabled(false);


    for(int index = 0;index < script_list.count();++index)
    {
        QString subject_file_name = ui->WorkDir->text()+"/" + script_list[index] + "/subject";
        image::io::bruker_info subject_file;
        if(subject_file.load_from_file(subject_file_name.toLocal8Bit().begin()))
        {
            int row = ui->subject_list->rowCount();
            ui->subject_list->setRowCount(row+1);
            ui->subject_list->setItem(row, 0, new QTableWidgetItem(script_list[index]));
            ui->subject_list->setItem(row, 1, new QTableWidgetItem(subject_file["SUBJECT_name_string"].c_str()));
            std::istringstream in(subject_file["SUBJECT_date"]);
            std::string year,mon,date,time;
            in >> time >> date >> mon >> year;
            std::ostringstream out;
            out << year << "/" << mon_map[mon] << (date.size() == 1 ? "/0":"/") << date << " " << time;
            ui->subject_list->setItem(row, 2, new QTableWidgetItem(out.str().c_str()));
        }
    }
    ui->subject_list->setSortingEnabled(true);
    if(ui->subject_list->rowCount())
    {
        ui->subject_list->sortByColumn(2);
        ui->subject_list->selectRow(0);
    }
}

void FileBrowser::on_changeWorkDir_clicked()
{
    QString filename = QFileDialog::getExistingDirectory(this,"Browse Directory",
                                              ui->WorkDir->text());
    if( filename.isEmpty() )
        return;
    ui->WorkDir->setText(filename);
    populateDirs();
}

void FileBrowser::show_image(void)
{
    if(preview_loaded && preview_thread.get())
    {
        preview_data.swap(data);
        std::copy(preview_voxel_size,preview_voxel_size+3,voxel_size);
        ui->graphicsView->setMaximumHeight(data.height()+10);
        cur_z = data.depth() >> 1;
        preview_loaded = false;
        preview_thread.reset(0);
    }

    if(preview_file_name.size() && preview_thread.get() == 0)
    {
        preview_thread.reset(new boost::thread(&FileBrowser::preview_image,this,preview_file_name));
        preview_file_name.clear();
        preview_loaded = false;
    }

    if(!data.size())
        return;
    ++cur_z;
    if(cur_z >= data.depth())
        cur_z = 0;

    {
        image::basic_image<float,2> data_buffer;
        image::reslicing(data,data_buffer,2,cur_z);
        image::normalize(data_buffer,slice_image,mask.empty() ? 255:125);
    }

    if(!mask.empty())
    {
        image::basic_image<unsigned char,2> mask_buffer;
        image::reslicing(mask,mask_buffer,2,cur_z);
        for(unsigned int index = 0;index < mask_buffer.size();++index)
            if (mask_buffer[index])
                slice_image[index][0] = 255;
    }

    view_image = QImage((unsigned char*)&*slice_image.begin(),slice_image.width(),slice_image.height(),QImage::Format_RGB32);
    scene.setSceneRect(0, 0, view_image.width(),view_image.height());
    scene.clear();
    scene.setItemIndexMethod(QGraphicsScene::NoIndex);
    scene.addRect(0, 0, view_image.width(),view_image.height(),QPen(),view_image);

}
void FileBrowser::preview_image(QString file_name)
{
    image::io::bruker_2dseq header;
    if(header.load_from_file(file_name.toLocal8Bit().begin()))
    {
        header >> preview_data;
        header.get_voxel_size(preview_voxel_size);

        QString ROIName = file_name+".roi.mat";
        if(QFileInfo(ROIName).exists())
        {
            image::io::mat_read mat_header;
            mat_header.load_from_file(ROIName.toLocal8Bit().begin());
            mat_header >> mask;
        }
        else
            mask.clear();
    }
    preview_loaded = true;
}

void FileBrowser::on_tableWidget_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    if(currentRow == previousRow || currentRow == -1)
        return;
    preview_file_name = image_list[currentRow];
    setWindowTitle(preview_file_name);
    image_file_name = preview_file_name;
}

#include <set>
void FileBrowser::prepare_images(std::vector<image::basic_image<float,3> >& image_data,
                    image::basic_image<unsigned char,3>& trimmed_mask,
                    std::vector<float>& tes,std::vector<float>& trs,std::vector<float>& fas)
{
    QList<QTableWidgetItem *> sel = ui->tableWidget->selectedItems();
    if(sel.size() < 2)
        return;
    std::set<int> selected_rows;
    image::geometry<3> geo;
    short rmin[3],rmax[3];
    trimmed_mask = mask;
    if(!mask.empty())
    {
        bounding_box(trimmed_mask,rmin,rmax);
        crop(trimmed_mask,rmin,rmax);
    }
    begin_prog("loading");
    for(unsigned int index = 0;check_prog(index,sel.size());++index)
    {
        int row = sel[index]->row();
        if(selected_rows.find(row) != selected_rows.end())
            continue;
        selected_rows.insert(row);
        image::io::bruker_2dseq header;
        if(!header.load_from_file(image_list[row].toLocal8Bit().begin()))
        {
            QMessageBox::information(this,"Error",
                QString("Cannot open file ")+image_list[row],0);
            return;
        }

        tes.push_back(te_list[row]);
        trs.push_back(tr_list[row]);
        fas.push_back(fa_list[row]);


        image_data.push_back(image::basic_image<float,3>());
        header >> image_data.back();
        if(!index)
            geo = image_data.back().geometry();
        else
        {
            if(image_data.back().geometry() != geo)
            {
                QMessageBox::information(this,"Error",
                    "image dimension is not consistent",0);
                image_data.clear();
                return;
            }
        }
        if(!mask.empty())
            image::crop(image_data.back(),rmin,rmax);
    }
}

void calculateR2(const std::vector<image::basic_image<float,3> >& image_data,
                 const image::basic_image<unsigned char,3>& trimmed_mask,
                 const std::vector<float>& tes,
                 image::basic_image<float,3>& data)
{
    data.clear();
    data.resize(image_data.front().geometry());
    begin_prog("calculating");
    for(unsigned int index = 0;check_prog(index,image_data.front().size());++index)
    {
        if(!trimmed_mask[index])
            continue;
        std::vector<float> log_s(tes.size());
        for(unsigned int i = 0;i < tes.size();++i)
            log_s[i] = std::log(std::max<float>(1.0,image_data[i][index]));
        std::pair<double,double> r = image::linear_regression(tes.begin(),tes.end(),log_s.begin());

        if(std::fabs(r.first) >= 0.001)
            data[index] = 1.0/-r.first;
    }
}

void calculateT2(const std::vector<image::basic_image<float,3> >& image_data,
                 const image::basic_image<unsigned char,3>& trimmed_mask,
                 const std::vector<float>& tes,
                 image::basic_image<float,3>& data)
{
    data.clear();
    data.resize(image_data.front().geometry());
    begin_prog("calculating");
    for(unsigned int index = 0;check_prog(index,image_data.front().size());++index)
    {
        if(!trimmed_mask[index])
            continue;
        /*
        std::vector<float> t2_list;
        for(unsigned int i = 0;i < tes.size();++i)
            for(unsigned int j = i+1;j < tes.size();++j)
        {
            float si = image_data[i][index];
            float sj = image_data[j][index];
            if(tes[i] == tes[j] ||  si == 0.0 || sj == 0.0 || si == sj)
                continue;
            if(tes[i] > tes[j] && si > sj)
                continue;
            if(tes[i] < tes[j] && si < sj)
                continue;
            t2_list.push_back((tes[i]-tes[j])/std::log(sj/si));
        }
        if(t2_list.empty())
            continue;
        //image::linear_regression();
        std::sort(t2_list.begin(),t2_list.end());
        data[index] = t2_list[(t2_list.size()-1) >> 1];
        if(data[index] > 5000)
            data[index] = 0;
            */

        std::vector<float> log_s(tes.size());
        for(unsigned int i = 0;i < tes.size();++i)
            log_s[i] = std::log(std::max<float>(1.0,image_data[i][index]));
        std::pair<double,double> r = image::linear_regression(tes.begin(),tes.end(),log_s.begin());

        data[index] = -r.first;
    }
}

void FileBrowser::on_loadT2_clicked()
{
    if(mask.empty())
        on_DefaultSegmentation_clicked();
    std::vector<image::basic_image<float,3> > image_data;
    image::basic_image<unsigned char,3> trimmed_mask;
    std::vector<float> tes,trs,fas;
    prepare_images(image_data,trimmed_mask,tes,trs,fas);
    if(image_data.empty())
        return;
    calculateT2(image_data,trimmed_mask,tes,data);
    accept();
}

void FileBrowser::on_loadR2_clicked()
{
    if(mask.empty())
        on_DefaultSegmentation_clicked();
    std::vector<image::basic_image<float,3> > image_data;
    image::basic_image<unsigned char,3> trimmed_mask;
    std::vector<float> tes,trs,fas;
    prepare_images(image_data,trimmed_mask,tes,trs,fas);
    if(image_data.empty())
        return;
    calculateR2(image_data,trimmed_mask,tes,data);
    accept();
}

void FileBrowser::on_loadT1_clicked()
{
    if(mask.empty())
        on_DefaultSegmentation_clicked();
    std::vector<image::basic_image<float,3> > image_data;
    image::basic_image<unsigned char,3> trimmed_mask;
    std::vector<float> tes,trs,fas;
    prepare_images(image_data,trimmed_mask,tes,trs,fas);
    if(image_data.empty())
        return;
    data.resize(image_data.front().geometry());
    image::multiply_constant(fas.begin(),fas.end(),3.1415926/180.0);
    std::vector<float> tan_fa(fas.size()),sin_fa(fas.size());
    for(unsigned int index = 0;index < fas.size();++index)
    {
        tan_fa[index] = std::tan(fas[index]);
        sin_fa[index] = std::sin(fas[index]);
    }

    begin_prog("calculating");
    for(unsigned int index = 0;check_prog(index,image_data.front().size());++index)
    {
        if(!trimmed_mask[index])
        {
            data[index] = 0;
            continue;
        }
        std::vector<float> t1_tan_list(fas.size()),t1_sin_list(fas.size());
        for(unsigned int i = 0;i < fas.size();++i)
        {
            t1_tan_list[i] = image_data[i][index]/tan_fa[i];
            t1_sin_list[i] = image_data[i][index]/sin_fa[i];
        }
        std::pair<double,double> result =
                image::linear_regression(t1_tan_list.begin(),t1_tan_list.end(),t1_sin_list.begin());
        if(result.first < 0)
        {
            data[index] = 0;
            continue;
        }
        data[index] = std::min(10000.0,std::max(0.0,-trs.front()/std::log(result.first)));
    }
    accept();
}
void FileBrowser::on_Combine_clicked()
{
    QList<QTableWidgetItem *> sel = ui->tableWidget->selectedItems();
    if(sel.size() < 2)
        return;
    std::set<int> selected_rows;
    data.clear();
    begin_prog("loading");
    for(unsigned int index = 0;check_prog(index,sel.size());++index)
    {
        int row = sel[index]->row();
        if(selected_rows.find(row) != selected_rows.end())
            continue;
        selected_rows.insert(row);
        image::io::bruker_2dseq header;
        if(!header.load_from_file(image_list[row].toLocal8Bit().begin()))
        {
            QMessageBox::information(this,"Error",
                QString("Cannot open file ")+image_list[row],0);
            return;
        }
        image::basic_image<float,3> buf;
        header >> buf;
        data.resize(image::geometry<3>(buf.width(),buf.height(),data.depth()+buf.depth()));
        std::copy(buf.begin(),buf.end(),data.end()-buf.size());
    }
    accept();
}

void FileBrowser::on_DefaultSegmentation_clicked()
{
    mask.resize(data.geometry());
    image::segmentation::otsu(data,mask);
    image::morphology::smoothing(mask);
    image::morphology::smoothing(mask);
}

void FileBrowser::on_threshold_clicked()
{
    bool ok;
    double threshold = QInputDialog::getDouble(this,"ImageA","Please assign the threshold",
                                         (float)image::segmentation::otsu_threshold(data),
                                         (float)*std::min_element(data.begin(),data.end()),
                                         (float)*std::max_element(data.begin(),data.end()),1,&ok);
    if (!ok)
        return;
    image::threshold(data,mask,threshold);
}

void FileBrowser::on_smoothing_clicked()
{
    if(!mask.empty())
        image::morphology::smoothing(mask);
}

void FileBrowser::on_erosion_clicked()
{
    if(!mask.empty())
        image::morphology::erosion(mask);

}

void FileBrowser::on_dilation_clicked()
{
    if(!mask.empty())
        image::morphology::dilation(mask);

}

void FileBrowser::on_defragment_clicked()
{
    if(!mask.empty())
        image::morphology::defragment(mask);
}

void FileBrowser::on_refresh_list_clicked()
{
    populateDirs();
}

void FileBrowser::on_buttonBox_accepted()
{
    if(preview_thread.get())
    {
        preview_thread->join();
        show_image();
    }
    if(method_list[ui->tableWidget->currentRow()] == "MSME" ||
       method_list[ui->tableWidget->currentRow()] == "MGE" ||
       method_list[ui->tableWidget->currentRow()] == "rARET2")
    {
        image::io::bruker_info method;
        if(method.load_from_file(method_file_list[ui->tableWidget->currentRow()].c_str()))
        {
            std::vector<float> TEs;
            std::istringstream in(method["EffectiveTE"]);
            std::copy(std::istream_iterator<float>(in),std::istream_iterator<float>(),std::back_inserter(TEs));

            std::vector<image::basic_image<float,3> > image_data(TEs.size());
            image::geometry<3> geo(data.width(),data.height(),data.depth()/TEs.size());
            image::basic_image<unsigned char,3> assign_mask(geo);
            std::fill(assign_mask.begin(),assign_mask.end(),1);
            for(unsigned int index = 0;index < TEs.size();++index)
            {
                image_data[index].resize(geo);
                std::copy(data.begin()+geo.size()*index,
                          data.begin()+geo.size()*(index+1),
                          image_data[index].begin());
            }
            image::basic_image<float,3> new_data;
            calculateT2(image_data,assign_mask,TEs,new_data);
            data = new_data;
        }
    }

}
