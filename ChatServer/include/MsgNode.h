#include <cstring>
#include <arpa/inet.h>  //大小端转换
#include "const.h"

//totoal_len:head+body总长度
//max_len:body长度
//基类：解析头的时候使用
class MsgNode{
public:
    MsgNode(short total_len):_total_len(total_len),_cur_len(0){
        _data=new char[_total_len+1];
        _data[_total_len]='\0';
    }

    ~MsgNode(){delete[] _data;}
    short _cur_len;     //当前接收/写入长度
    short _total_len;   //总报文长度
    char* _data;
};

class RecvNode:public MsgNode{
public:
    RecvNode(short max_len,short msg_id):MsgNode(max_len),_msg_id(msg_id){}
    short _msg_id;
};

class SendNode:public MsgNode{
public:
    SendNode(const char* msg,short max_len,short msg_id)
        :MsgNode(max_len+HEAD_TOTAL_LEN),_msg_id(msg_id){
            //由于这俩是多字节整数，需要转换成网络字节序大端
            short id_net=htons(msg_id);
            short len_net=htons(max_len);
            memcpy(_data,&id_net,HEAD_ID_LEN);
            memcpy(_data+HEAD_ID_LEN,&len_net,HEAD_DATA_LEN);
            memcpy(_data+HEAD_TOTAL_LEN,msg,max_len);
        }
    
    short _msg_id;
};