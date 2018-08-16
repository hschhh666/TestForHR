#include <QCoreApplication>
#include "tcpclient.h"
#include "tcpserver.h"
#include <cstring>
#include <cstdint>
#include <fstream>
#include <reverse.hpp>
#include <cstdlib>
#define random(a,b) (rand()%(b-a+1)+a)
//#define path_of_answer "C:/Users/admin/Desktop/Testdata-001-Groundtruth/gt_detection.csv"
using namespace std;
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
//    ifstream file1;
//    file1.open("./files/38490890.txt" ,std::ios_base::in);
//    int q,w;
//    int8_t e;
//    file1>>q>>q>>w>>e;
//    printf("%c",e);
//    getchar();
//    return 0;

    char judge;
    do{
    cout<<"If generate random data? [y/n] :"<<endl;
    cin>>judge;}while((judge!='y')&&(judge!='n'));


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

    TCPClient client1(QHostAddress::LocalHost,7000);
    client1.connectServer();
    client1.writeData(data,3);
    //client1.closeConnect();


    TCPClient client2(QHostAddress::LocalHost,7003);
    client2.connectServer();
    //client2.writeData(data,3);
    //client2.closeConnect();



    std::cout<<"Send start single to server. "<<std::endl;


    QByteArray oneDatagram;
    while(!server1.readFixedData(oneDatagram));

    int num;
    int currIndex=0;
    int x,y;
    int8_t value;
    long long timeStamp_tk;
    char typeOfToken;
    ifstream file;
    int8_t EnvirMap[81*81];
    char answer[1+2+1+8+81*81+1];
    char start_of_socket = '\x02';
    char end_of_socket = '\x03';
    int16_t length = 81*81+8+1;
    char type_of_answer = 'M';
    std::string root_direct="./files/";
    long long timestamp_gt[4] = {38490890,38505940,38548019,38661210};
    for(int i=0;i<81*81;i++)
        EnvirMap[i] = 0;



    while(1)
    {

        while(!server2.readFixedData(oneDatagram));//等待数据到来

        char ddd[] ={oneDatagram.at(8),oneDatagram.at(7),oneDatagram.at(6),oneDatagram.at(5),oneDatagram.at(4),oneDatagram.at(3),oneDatagram.at(2),oneDatagram.at(1)};
        memcpy(&timeStamp_tk,ddd,8);
        typeOfToken = oneDatagram.at(9);

        if(timestamp_gt[currIndex]!=timeStamp_tk)continue;

        cout<<"Send file "<<timestamp_gt[currIndex]<<".txt"<<endl;
        file.open(root_direct+ to_string(timestamp_gt[currIndex])+".txt" ,std::ios_base::in);
        if(!file.good())
        {
            cout<<"Groundtruth file open failed, exit program...\n";
            getchar();
            return 0;
        }
        file>>num;
        for(int i=0;i<num;i++)
        {
            file>>x>>y>>value;
            if(judge == 'n')
                EnvirMap[x*81+y] = value - 48;
            else
            {
                EnvirMap[x*81+y] = random(0,3);
            }
        }
        file.close();

        length = revert_w(length);
        timeStamp_tk = revert_l(timeStamp_tk);

        memcpy(answer,&start_of_socket,1);
        memcpy(answer+1,&length,2);
        memcpy(answer+3,&type_of_answer,1);
        memcpy(answer+4,&timeStamp_tk,8);
        memcpy(answer+12,EnvirMap,81*81);
        memcpy(answer+12+81*81,&end_of_socket,1);
        client2.writeData(answer,1+2+1+8+81*81+1);
        currIndex +=1;
        if(currIndex == 4)
            break;
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
