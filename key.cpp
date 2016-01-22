//#include <stdio.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <fcntl.h>
#include <key.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include<QDebug>
#include <QWSServer>

#define KEYCOUNTS 6

readKey::readKey(QObject *parent) :
    QThread(parent)
{
    initFlag = false;
    indexNum = 1;
}


void readKey::run()
{
    int keyValue[KEYCOUNTS];
    int i;
    if( !initFlag )
    {    fd = open("/dev/key",0);
         initFlag = ~initFlag;
    }

    read(fd,keyValue, sizeof(keyValue));

    for(i=0;i<KEYCOUNTS;i++)
    {
        if(keyValue[i] == 1)//1还是0代表按下来着？
        {
        switch(i)
        {
        case 1:
        {
           if(i==0)//切换键
            {
                emit disSwitch(indexNum);
                indexNum = (indexNum+1)%4;
//                qDebug()<<QString("%1").arg(indexNum);
            }         
        }break;
        case 2:
            QWSServer::sendKeyEvent(-1,Qt::Key_Up);break;
        case 3:
            QWSServer->sendKeyEvent(-1,Qt::Key_Down);break;
        case 4:
            QWSServer->sendKeyEvent(-1,Qt::Key_Left);break;
        case 5:
            QWSServer->sendKeyEvent(-1,Qt::Key_Right);break;
        case 6://确认键
            QWSServer->sendKeyEvent(-1,Qt::Key_Space);break;
        }
    }}
    exec();
}