#ifndef _DOUYU_DANMU_HPP_
#define _DOUYU_DANMU_HPP_

#include <iostream>
#include <QWebSocket>
#include <QString>
#include <QMap>
#include <QFile>
#include <QObject>
#include <QTimer>
#include <QtSql>
#include <QQueue>
#include <QMutex>
#include <QThread>

#define KEEPALIVE_INTERVEL_MS 20000
#define MAX_SQL_RECORD_HANDLE_PER_TIME 20
#define RECV_BUF_SIZE 1500
#define MAX_RECV_BUF_SIZE 15000  //斗鱼有时候会发11KB左右的超大号包
#define MAX_RECONNECT_TIME 4  //失败之后的最大重连次数
#define CONFIG_FILE_NAME "config.json"
#define GIFT_ID_FILE_NAME "gift_id.json"

#define INSERT_CHAT "INSERT INTO chatmsg VALUES(\"%1\", \"%2\", \"%3\", %4, %5, \"%6\", %7, \"%8\", %9, %10);"
#define INSERT_DGB "INSERT INTO dgb VALUES(null, \"%1\", \"%2\", %3, %4, \"%5\", %6, \"%7\", %8);"
#define INSERT_NOBLE_NUM "INSERT INTO noble_num VALUES(%1, %2);"


#define uchar unsigned char
#define usint unsigned short int
#define uint unsigned int


struct package{
    int length1;  
    int length2;  //与length1数值相同
    usint type;
    uchar encrypt = 0; //默认为0
    uchar reserve = 0; //默认为0
    char data[RECV_BUF_SIZE]; 
};

class Douyu_Danmu : QObject{
    Q_OBJECT

public:
    Douyu_Danmu(QObject *parent = nullptr, QCoreApplication *application = nullptr, QString rid = NULL);
    ~Douyu_Danmu();
    bool connectServer(void);
    static void show_info(QString tag, QString info);

public slots:
    void onWebSocketConnected(void);
    void onWebSocketDisconnected(void);
    void onWebSocketError(QAbstractSocket::SocketError error);
    void onWebSocketBinaryMessageArrived(const QByteArray &message);
    void onWebSocketLogin(void);
    void keepAlive(void);

signals:
    void have_login(void);

private: 
    QCoreApplication *application_ptr;
    static QString roomID;
    QWebSocket *websocket;
    QString websocket_origin;
    QString websocket_server_addr;
    QString log_file_name;
    usint package_type_from_server;
    usint package_type_from_client;

    QFile *log_file;
    QTimer* timer;
    static QThread *sql_thread;
    static bool isThreadContinue;
    static QMutex isThreadContinue_mtx;
    static QMutex queue_mtx;
    static QQueue<QMap<QString, QString>> queue_with_sql;

//    static QSqlDatabase database;
//    QString insert_chat = "INSERT INTO chatmsg VALUES(:cid, :uid, :name, :level, :device, :badge, :blevel, :txt, :color, :time);";
//    QString insert_d = "INSERT INTO dgb VALUES(null, :uid, :name, :level, :device, :badge, :blevel, :gift, :hits);";
//    QString insert_n = "INSERT INTO noble_num VALUES(:time, :num);";
//    static QString insert_chat;
//    static QString insert_d;
//    static QString insert_n;
//    static QSqlQuery query_chatmsg;
//    static QSqlQuery query_dgb;
//    static QSqlQuery query_noble_num;

    bool isConnected;
    bool isKeepAlive;
    bool isOk;

    unsigned char reconnect_time;

    package tmp_package_to_keep_alive;
    static QMap<QString, QString> gift_id_map;

    bool login(void);
    void logout(void);
    static bool initSql(QSqlDatabase &database);
    void makePackage(package* pack, const char* data, usint data_len);
    bool joinGroup(void);
    bool read_config(void);
    static void sql_actions();
};

#endif
