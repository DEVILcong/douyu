#ifndef _DOUYU_DANMU_HPP_
#define _DOUYU_DANMU_HPP_

#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<unistd.h>
#include<arpa/inet.h>

#include<iostream>
#include<cstring>
#include<string>
#include<cerrno>
#include<thread>
#include<chrono>

#include<unordered_map>
#include<queue>
#include<regex>
#include<mutex>
#include<fstream>

#include<sqlite3.h>

#define RECV_BUF_SIZE 1500
#define DANMU_URL "danmuproxy.douyu.com"
#define DANMU_PORT "8601"
#define PACKAGE_T_FROM_S 690 
#define PACKAGE_T_FROM_C 689 //数据包类型，从服务器接受或客户端发送的

#define uchar unsigned char
#define usint unsigned short int
#define uint unsigned int


struct package{
    int length1;  
    int length2;  //与length1数值相同
    usint type;     //在上面宏定义中
    uchar encrypt = 0; //默认为0
    uchar reserve = 0; //默认为0
    char data[RECV_BUF_SIZE]; 
};

class Gifts{
    public:
        Gifts();
        ~Gifts();
        static std::unordered_map<std::string, std::string> gift_type;
};

class Socket{
public:
    Socket();
    Socket(int id);
    ~Socket();
    int getID();
    void operator= (int id);

private:
    int ID;

};

class Douyu_Danmu{
public:
    Douyu_Danmu(uint rid);
    ~Douyu_Danmu();
    int getSocketID(void);
    bool initSqlite(void);
    bool connectServ(void);
    bool login(void); 
    void recv_message(void);
    void logout(void);

private: 
    Socket socketID;
    uint roomID;
    sqlite3* db;
    sqlite3_stmt* insert_chatmsg;
    sqlite3_stmt* insert_dgb;

    bool isEnableSqlite;
    bool isKeepAlive;
    bool isKeepRecv;
    bool isKeepRunning;

    std::queue<std::unordered_map<std::string, std::string>> packages;
    std::mutex queue_lock;

    static void makePackage(package* pack, const char* data, usint data_len);
    bool joinGroup(void);
    static void control(bool* KeepRunning);
    static void keepAlive(int ID, bool* keepAlive);
    static std::unordered_map<std::string, std::string> parsePackage(package* pack);
    static void recvPackage(int ID, std::queue<std::unordered_map<std::string, std::string>>& q, bool* KeepRecv, std::mutex& queue_lk);

};

#endif
