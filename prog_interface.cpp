#include <QProgressDialog>
#include <QtGui/QApplication>
#include <QObject>
#include <memory>
#include <ctime>
#include <iostream>

std::auto_ptr<QProgressDialog> progressDialog;
long cur_time = 0;
const long interval = 500;

        extern "C" void begin_prog(const char* title)
        {
            if(!progressDialog.get())
                return;
            progressDialog.reset(new QProgressDialog(title,"Abort",0,10,0));
            progressDialog->setMinimumDuration(0);
            qApp->processEvents();
            cur_time = 0;
        }

        extern "C" void set_title(const char* title)
        {
            if(!progressDialog.get())
                return;
            progressDialog->setLabelText(title);
            qApp->processEvents();
        }

        extern "C" void can_cancel(int cancel)
        {
            if(cancel && progressDialog.get())
                progressDialog->setCancelButtonText("&Cancel");
        }
        extern "C" int check_prog(int now,int total)
        {
            if(now == total && progressDialog.get())
                progressDialog.reset(new QProgressDialog("","Abort",0,10,0));
            if(progressDialog.get() && (std::clock() - cur_time > interval))
            {
                if(progressDialog->wasCanceled())
                    return false;
                progressDialog->setRange(0, total);
                progressDialog->setValue(now);
                progressDialog->setLabelText(QString("%1 of %2...").arg(now).arg(total));
                qApp->processEvents();
                cur_time = std::clock();
            }
            return now < total;
        }

        extern "C" int prog_aborted(void)
        {
            if(progressDialog.get())
                return progressDialog->wasCanceled();
            return false;
        }



