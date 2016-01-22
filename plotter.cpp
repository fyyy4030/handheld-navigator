#include "plotter.h"
#include "um220.h"
#include "qcursor.h"
#include "key.h"
#define BEISHU 0.1//一像素代表多少海里

extern class um220 *beidouData;
extern class readKey *readKeyThread;

/*****************
*
* 页面布局和文字刷新
* 以及画布scene初始化
*
*****************/
Plotter::Plotter(QWidget *parent) :
    QWidget(parent)
{
    QTimer *rTimer = new QTimer(this);
    rTimer->start(200);
    connect(rTimer,SIGNAL(timeout()),this,SLOT(showTime()));

    QObject::connect(beidouData,SIGNAL(dataUpdate()),this,SLOT(showTime()));

    scene = new QGraphicsScene;
    scene->setSceneRect(0, 0, viewWidth, viewHeight);
    plView =new PlView;
    plView->setScene(scene);
    plView->setRenderHint(QPainter::Antialiasing);//抗锯齿
    plView->centerOn(QPoint(0,0));
    plView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    //默认视野在中间,调至左上角否则无法实现鼠标全局坐标和view相对坐标的对应

    //窗口静态布局初始化
    {
        settingLabel =new QLabel(tr("SETTING"));
        settingLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        settingLabel->setFrameShape (QFrame::Box);

        CSELabel =new QLabel(tr("CSE"));
        CSELabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        CSELabel->setFrameShape (QFrame::Box);

        SPDLabel =new QLabel(tr("SPD"));
        SPDLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        SPDLabel->setFrameShape (QFrame::Panel);

        coordinateLabel =new QLabel(tr("longitude\t\tlatitude"));
        coordinateLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        coordinateLabel->setFrameShape (QFrame::Box);
        mainLayout = new QGridLayout(this);
        mainLayout->addWidget(settingLabel,0,0);
        mainLayout->addWidget(CSELabel,1,0);
        mainLayout->addWidget(SPDLabel,2,0);
        mainLayout->addWidget(plView,0,1,3,1);
        mainLayout->addWidget(coordinateLabel,3,0,1,2);
        mainLayout->setRowStretch(0,2);
        mainLayout->setRowStretch(1,2);
        mainLayout->setRowStretch(2,2);
        mainLayout->setRowStretch(3,1);
        mainLayout->setColumnStretch(0,1);
        mainLayout->setColumnStretch(1,5);
    }

    //创建一个直径8像素的圆形item代表当前位置
    QGraphicsItem *zero = new QGraphicsEllipseItem(
                QRectF(viewWidth/2.0,viewHeight/2.0,8,8));
//    plView->horizontalScrollBar()->setHidden(true);
//    plView->verticalScrollBar()->setHidden(true);

//    QPoint *temp = new QPoint(plView->mapToGlobal(QPoint(viewWidth/2,viewHeight/2)).x(),
//                              plView->mapToGlobal(QPoint(viewWidth/2,viewHeight/2)).y());
    plView->cursor.setPos(viewWidth/2+10*3+viewWidth/5+1,viewHeight/2+10*2+4);
    scene->addItem(zero);
    qDebug("%lf,%lf",zero->x(),zero->y());

}

void Plotter::showTime()
{//文字刷新
    CSEString = "CSE:";
    CSEString.append(beidouData->cog);
    CSEString.append("°");
    CSELabel->setText(CSEString);

    SPDString = "SPD:";
    SPDString.append(beidouData->spd);
    SPDString.append("knot");
    SPDLabel->setText(SPDString);

       const QPointF nowCoor =plView->coorCalc(cursor().pos(),viewWidth,viewHeight);

    coordinateString = "Longitude:";
    coordinateString.append(QString::number(nowCoor.x()));
    if(beidouData->E.toInt())//东南西北后缀判别
        coordinateString.append(QString("E\nLatitude:"));
    else
        coordinateString.append(QString("W\nLatitude:"));
    coordinateString.append(QString::number(nowCoor.y()));
    if(beidouData->N.toInt())
        coordinateString.append(QString("N"));
    else
        coordinateString.append(QString("S"));
    coordinateLabel->setText(coordinateString);
}

/*****************
* coorCalc
* 输入：光标位置
* 输出：经纬度
*
*****************/
QPointF PlView::coorCalc(QPoint nowPos,int viewWidth,int viewHeight)
{
    nowPos = mapFromGlobal(nowPos);
    return QPointF(beidouData->Lon.toInt()+(nowPos.x()-viewWidth/2.0)*BEISHU,
                   beidouData->Lat.toInt()+(nowPos.y()-viewHeight/2.0)*BEISHU
                   );
}


/*****************
*
* 自定义item，代表途径点
* 的小叉
*
*****************/
/*
XItem::XItem()
{
    setFlag(QGraphicsItem::ItemIsFocusable);
    setFlag(QGraphicsItem::ItemIsMovable);
    setAcceptHoverEvents(true);
}

QRectF XItem::boundingRect() const
{
    qreal adjust = 0;
    return QRectF(-10 - adjust, -10 - adjust,
                  20 + adjust, 20 + adjust);
}

void XItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
    painter->setBrush(Qt::black);
    painter->drawLine(-10,-10,10,10);
    painter->drawLine(-10,10,10,-10);

}
*/


/*****************
*
* View：整体嵌入布局
* 并负责管理光标操作
*
*****************/
PlView::PlView(QWidget *parent) :
    QGraphicsView(parent)
{
    QCursor cursor ;
    cursor = QCursor(Qt::CrossCursor);
    setCursor(cursor);
//    cursor.setPos(QPoint(500,400));
}

/*****************
*
* key.c中接收自定义按键数据,
* 发送keyEvent,在本函数中处理
* 不用自定义事件是为了便于调试
* 和增强可移植性
*
*****************/
void PlView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {

    case Qt::Key_Equal ://缩放视野,坐标体系不变
        scale(1.2, 1.2);
        break;
    case Qt::Key_Minus :
        scale(1 / 1.2, 1 / 1.2);
        break;

    case Qt::Key_Up ://上下左右移动光标
        if(cursor.pos().y()> PlView::pos().y() )//防止光标越界
            cursor.setPos((QPoint(cursor.pos().x(),cursor.pos().y()-8)));
        break;
    case Qt::Key_Down :
        if(cursor.pos().y() < (PlView::pos().y() + PlView::height()) )
            cursor.setPos((QPoint(cursor.pos().x(),cursor.pos().y()+8)));
        break;
    case Qt::Key_Left :
        if(cursor.pos().x() > PlView::pos().x() )
        {   cursor.setPos((QPoint(cursor.pos().x()-8,cursor.pos().y())));
            qDebug("Key_Left");
        }break;
    case Qt::Key_Right :
        if(cursor.pos().x() < (PlView::pos().x()+ PlView::width()))
        {   cursor.setPos((QPoint(cursor.pos().x()+8,cursor.pos().y())));
            qDebug("Key_Right");
        }break;


    case Qt::Key_Space ://管理途径点,以16x16的正方形表示
    {
        if(scene()->itemAt(QPoint(
                               mapFromGlobal(QCursor::pos()).x()-8,
                               mapFromGlobal(QCursor::pos()).y()-8)
                           )== 0)
        {scene()->addRect(//若无则添加
                        mapFromGlobal(QCursor::pos()).x()-8,
                        mapFromGlobal(QCursor::pos()).y()-8,
                        16,16);
            qDebug("point:%d,%d",mapFromGlobal(QCursor::pos()).x(),mapFromGlobal(QCursor::pos()).y());
        }
       /*
        //            XItem *item = new XItem;
        //            item->setPos(mapFromGlobal(QCursor::pos()).x()-10,
        //                         mapFromGlobal(QCursor::pos()).y()-10);
        //            scene()->addItem(item);
        //自定义item不能被itemAt识别？无法正常删除
        */
        else {scene()->removeItem(scene()->itemAt(QPoint(//若当前光标处有item，则删除
                                                     mapFromGlobal(QCursor::pos()).x()-8,
                                                     mapFromGlobal(QCursor::pos()).y()-8)));
        }
    }break;
    }
}
