#include <QCoreApplication>
#include "tcpclient.h"
#include "tcpserver.h"
#include <cstring>
#include <cstdint>
#include <fstream>
#include <reverse.hpp>
#include <opencv2/opencv.hpp>
#include <cstdlib>
#include "CropImage.h"
#include <iostream>
#include "groundtruthfile.h"
#include <chrono>
#include<Windows.h>//use it only for Sleep()
#define random(a,b) (rand()%(b-a+1)+a)
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;
high_resolution_clock::time_point beginTime, endTime;
milliseconds timeInterval;


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    std::string path_of_answer="gt_detection.csv";
    GroundTruthFile gtfile(path_of_answer,true);
    GroundTruthData gtdata;

    loadData();
    char isrand;
    do{
    std::cout<<"If generate random data? [y/n] :"<<std::endl;
    std::cin>>isrand;}while((isrand!='y')&&(isrand!='n'));

    int Maxdelay;
    std::cout<<"Input max delay time (ms). Input 0 if don't want delay. MaxDelay = "<<std::endl;
    std::cin>>Maxdelay;


    TCPServer server1(7001,3);
    server1.start();

    TCPServer server2(7002,12);
    server2.start();

    char *data;
    QByteArray signal;
    std::cout<<"Waiting for start signal......"<<std::endl;
    while(!server1.readFixedData(signal));
    if((signal.at(0)=='\x02')&&(signal.at(1)=='\x30')&&(signal.at(2)=='\x03'))
        std::cout<<"Receive start signal."<<std::endl;
    else{
        std::cout<<"start signal is wrong, exit program. "<<std::endl;
        return 0;
    }


    data = new char[3];
    data[0] = '\x02'; data[1] = '\x32'; data[2] = '\x03';

    QHostAddress addre("192.168.1.7");
    addre = QHostAddress::LocalHost;
    TCPClient client1(addre,7000);
    client1.connectServer();
    client1.writeData(data,3);
    //client1.closeConnect();


    TCPClient client2(addre,7003);
    client2.connectServer();
    //client2.closeConnect();

    std::cout<<"Send start single to server. "<<std::endl;

    char box_type_char;
    char attribute_char;
    int8_t box_type;
    int8_t attribute;
    double box[4][2];
    double longitude;
    double latitude;
    long long timeStamp_gt,last_timeStamp;
    int64_t timeStamp_tk;
    char typeOfToken;

    gtfile.ReadOneGTData(gtdata);
    timeStamp_gt = gtdata.timeStamp_gt;

    last_timeStamp = 0;
    QByteArray oneDatagram;

    while(!server1.readFixedData(oneDatagram));

    int answercnt = 0;
    while(1)
    {

        if (last_timeStamp!=timeStamp_gt)
            while(!server2.readFixedData(oneDatagram));//等待数据到来


        char ddd[] ={oneDatagram.at(8),oneDatagram.at(7),oneDatagram.at(6),oneDatagram.at(5),oneDatagram.at(4),oneDatagram.at(3),oneDatagram.at(2),oneDatagram.at(1)};
        memcpy(&timeStamp_tk,ddd,8);
        typeOfToken = oneDatagram.at(9);


        if(timeStamp_tk<timeStamp_gt)continue;
        last_timeStamp = timeStamp_gt;


        // 读入包围框类型
        box_type_char = gtdata.box_type_char;
        box_type = (box_type_char == 'C') ? 0 : 1;


        // 读入目标属性
        attribute_char = gtdata.attribute_char;
        attribute = (attribute_char == 'H') ? 0 :(( attribute == 'V')?1:2);


        // 读入包围框顶点坐标
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 2; j++) {
                box[i][j] = gtdata.box[i][j];
            }
        }

        int x0 = 0, y0 = 0 ,image_length = 0;
        uint16_t width = 0, height = 0;
        char *image_data = new char;

        if(box_type_char == 'C')
        {
            x0 = box[0][0];
            y0 = box[0][1];
            width = box[1][0] - box[0][0] + 1 ;
            height = box[2][1] - box[1][1] + 1;
            if(isrand == 'y')
            {
                x0 = abs(x0 - random(0, 2*width));
                y0 = abs(y0 - random(0, 2*height));
                if(x0+width>1000)
                    x0 = 1000-width;
                if(y0+height >700)
                    y0 = 700-height;
                box[0][0] = x0;
                box[0][1] = y0;
                box[1][0] = x0+width-1;
                box[1][1] = y0;
                box[2][0] = x0+width-1;
                box[2][1] = y0+height-1;
                box[3][0] = x0;
                box[3][1] = y0+height-1;
            }

            cv::Mat roi(width, height, CV_8UC3, cv::Scalar(0, 255, 255));
            cv::Rect rect(x0,y0,width,height);
            std::cout<<"x0 = "<<x0<<",y0 = "<<y0<<",width = "<<width<<",height = "<<height;
            bool flag = cropImage(timeStamp_tk, rect, roi);
            delete image_data;
            image_data = new char[rect.width*rect.height*3 + 1];
            enCodingImage(roi, image_data, image_length);
            std::cout<<",image length = "<<image_length;
        }




        // 读取目标位置精度
        longitude = gtdata.longitude;

        // 读取目标位置纬度
        latitude = gtdata.latitude;

        /* 编码答案数据 */
        uint32_t length_of_ODanswer = 95 + image_length;
        char *ODanswer = new char[101 + image_length];
        char start_of_socket = '\x02';
        char end_of_socket = '\x03';
        char type_of_ODanswer = '\x4F';
        uint16_t image_cols = width;
        uint16_t image_rows = height;


        length_of_ODanswer = revert_32(length_of_ODanswer);
        timeStamp_tk = revert_l(timeStamp_tk);
        for(int i = 0;i<4;i++)
        {
            box[i][0] = revert_d(box[i][0]);
            box[i][1] = revert_d(box[i][1]);
        }
        longitude = revert_d(longitude);
        latitude = revert_d(latitude);
        image_cols = revert_w(image_cols);
        image_rows = revert_w(image_rows);




        memcpy(ODanswer, &start_of_socket, 1);
        memcpy(ODanswer + 1, &length_of_ODanswer, 4);
        memcpy(ODanswer + 5, &type_of_ODanswer, 1);
        memcpy(ODanswer + 6, &timeStamp_tk, 8);
        memcpy(ODanswer + 14, &box_type, 1);
        memcpy(ODanswer + 15, box, 64);
        memcpy(ODanswer + 79, &attribute,1);
        memcpy(ODanswer + 80, &latitude, 8);
        memcpy(ODanswer + 88, &longitude, 8);
        memcpy(ODanswer + 96, &image_cols, 2);
        memcpy(ODanswer + 98, &image_rows, 2);
        if(image_length!=0)
        memcpy(ODanswer + 100, image_data, image_length);
        memcpy(ODanswer + 100 +image_length  , &end_of_socket, 1);

        std::cout<<"Answer "<<answercnt<<" : total length = "<< 95 +image_length<<"end = ";
        printf("%d",ODanswer[100+image_length]);
        cout<<"\n";
        answercnt ++;


        if(Maxdelay>0)
            Sleep(random(0,Maxdelay));

        client2.writeData(ODanswer,101+image_length);

        if(!gtfile.ReadOneGTData(gtdata))
            break;
        // 读入时间戳
        timeStamp_gt = gtdata.timeStamp_gt;
        delete image_data;
        delete ODanswer;
    }

    while(!server1.readFixedData(oneDatagram));
    std::cout<<"Received end signal. Send end signal to server......"<<std::endl;
    data[0] = '\x02'; data[1] = '\x35'; data[2] = '\x03';
    //client1.connectServer();
    client1.writeData(data,3);
    //client1.closeConnect();
    delete data;


    return a.exec();
}
