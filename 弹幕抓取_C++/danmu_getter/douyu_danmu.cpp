#include"douyu_danmu.hpp"
#include <QFile>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QRegExp>
#include <QWebSocket>
#include <QDateTime>

#include <QDebug>

int num_test = 0;

QString Douyu_Danmu::roomID;
QThread* Douyu_Danmu::sql_thread = nullptr;
QQueue<QMap<QString, QString>> Douyu_Danmu::queue_with_sql;
QMutex Douyu_Danmu::queue_mtx;
bool Douyu_Danmu::isThreadContinue = true;
QMutex Douyu_Danmu::isThreadContinue_mtx;

//QSqlDatabase Douyu_Danmu::database;
//QSqlQuery Douyu_Danmu::query_chatmsg;
//QSqlQuery Douyu_Danmu::query_dgb;
//QSqlQuery Douyu_Danmu::query_noble_num;

QMap<QString, QString> Douyu_Danmu::gift_id_map;

//QString Douyu_Danmu::insert_chat = "INSERT INTO chatmsg VALUES(%1, %2, %3, %4, %5, %6, %7, %8, %9, %10);";
//QString Douyu_Danmu::insert_d = "INSERT INTO dgb VALUES(null, %1, %2, %3, %4, %5, %6, %7, %8);";
//QString Douyu_Danmu::insert_n = "INSERT INTO noble_num VALUES(%1, %2);";

Douyu_Danmu::Douyu_Danmu(QObject *parent, QCoreApplication *application, QString rid)
    : QObject(parent){
    this->application_ptr = application;
    Douyu_Danmu::roomID = rid;
    this->websocket = new QWebSocket;

    this->isConnected = false;
    this->isKeepAlive = true;
    this->isOk = true;

    this->reconnect_time = 0;

    this->timer = new QTimer();
    this->timer->setInterval(KEEPALIVE_INTERVEL_MS);
    connect(this->timer, SIGNAL(timeout()), this, SLOT(keepAlive()));

    connect(this->websocket, SIGNAL(connected()), this, SLOT(onWebSocketConnected()));
    connect(this->websocket, SIGNAL(disconnected()), this, SLOT(onWebSocketDisconnected()));
    connect(this->websocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onWebSocketError(QAbstractSocket::SocketError)));
    connect(this->websocket, SIGNAL(binaryMessageReceived(const QByteArray)), this, SLOT(onWebSocketBinaryMessageArrived(const QByteArray)));

    connect(this, SIGNAL(have_login()), this, SLOT(onWebSocketLogin()));


    if(!(this->read_config())){
        this->isOk = false;
    }

    char info[] = "type@=mrkl/\0";
    makePackage(&(this->tmp_package_to_keep_alive), info, 12);

    sql_thread = QThread::create(Douyu_Danmu::sql_actions);
    if(sql_thread == nullptr){
        this->isOk = false;
    }
}

Douyu_Danmu::~Douyu_Danmu(){
    this->reconnect_time = MAX_RECONNECT_TIME;
    this->isOk = false;
    this->logout();

    isThreadContinue_mtx.lock();
    isThreadContinue = false;
    isThreadContinue_mtx.unlock();

    this->websocket->close();
    delete this->websocket;

    this->log_file->close();
    delete this->log_file;

    this->timer->stop();

    sql_thread->wait();
    if(sql_thread != nullptr)
        delete sql_thread;

    Douyu_Danmu::show_info("INFO", "service closed");
}

bool Douyu_Danmu::read_config(void){
    QFile tmp_config_file(CONFIG_FILE_NAME);
    QFile tmp_gift_id_file(GIFT_ID_FILE_NAME);

    if(!tmp_config_file.open(QIODevice::ReadOnly)){
        Douyu_Danmu::show_info("ERROR", "can't open config file");
        return false;
    }

    if(!tmp_gift_id_file.open(QIODevice::ReadOnly)){
        Douyu_Danmu::show_info("ERROR", "can't open gift id file");
        return false;
    }

    QJsonDocument tmp_jsonDoc = QJsonDocument::fromJson(tmp_config_file.readAll());
    if(tmp_jsonDoc.isNull()){
        Douyu_Danmu::show_info("ERROR", "can't read config file");
        tmp_config_file.close();
        tmp_gift_id_file.close();
        return false;
    }
    this->websocket_server_addr = tmp_jsonDoc["ws_protocol"].toString() +
            tmp_jsonDoc["ws_host"].toString() + ":" + tmp_jsonDoc["ws_port"].toString();
    this->websocket_origin = tmp_jsonDoc["ws_orogin"].toString();
    this->package_type_from_client = tmp_jsonDoc["package_type_from_client"].toInt();
    this->package_type_from_server = tmp_jsonDoc["package_type_from_server"].toInt();
    this->log_file_name = Douyu_Danmu::roomID + tmp_jsonDoc["logfile_suffix"].toString();

    tmp_jsonDoc = QJsonDocument::fromJson(tmp_gift_id_file.readAll());
    if(tmp_jsonDoc.isNull()){
        Douyu_Danmu::show_info("ERROR", "can't read gift id file");
        tmp_config_file.close();
        tmp_gift_id_file.close();
        return false;
    }
    QJsonArray tmp_jsonArray = tmp_jsonDoc.array();
    if(tmp_jsonArray.isEmpty()){
        Douyu_Danmu::show_info("ERROR", "can't read gift id file");
        tmp_config_file.close();
        tmp_gift_id_file.close();
        return false;
    }

    QJsonObject tmp_object;
    for(QJsonArray::iterator tmp_iterator = tmp_jsonArray.begin(); tmp_iterator != tmp_jsonArray.end(); ++tmp_iterator){
        tmp_object = (*tmp_iterator).toObject();
        gift_id_map[tmp_object["id"].toString()] = tmp_object["name"].toString();
    }

    this->log_file = new QFile(this->log_file_name);
    if(!this->log_file->open(QIODevice::ReadWrite | QIODevice::Append | QIODevice::Text)){
        Douyu_Danmu::show_info("ERROR", "can't open log file");
        tmp_config_file.close();
        tmp_gift_id_file.close();
        return false;
    }

    tmp_config_file.close();
    tmp_gift_id_file.close();
    return true;
}

void Douyu_Danmu::onWebSocketConnected(){
    Douyu_Danmu::show_info("INFO", "websocket connected");
    this->login();
}

void Douyu_Danmu::onWebSocketDisconnected(){
    Douyu_Danmu::show_info("ERROR", "websocket connection is closed");
    if(!this->isOk)
        return;
    this->isConnected = false;
    if(++(this->reconnect_time) < MAX_RECONNECT_TIME){
        this->connectServer();
        this->show_info("WARNING", "reconnect server");
    }else{
        isThreadContinue_mtx.lock();
        isThreadContinue = false;
        isThreadContinue_mtx.unlock();

        this->show_info("ERROR", "failed, reached maxium reconnect time");
        application_ptr->exit();
    }
}

void Douyu_Danmu::onWebSocketError(QAbstractSocket::SocketError error){
    Douyu_Danmu::show_info("ERROR", "websocket error, error code is " + QString::number(error));
}

void Douyu_Danmu::onWebSocketBinaryMessageArrived(const QByteArray &message){
    static QMap<QString, QString> tmp_map;
    static char buffer[MAX_RECV_BUF_SIZE];
    int packet_size = 0;
    int all_length = 0;
    QByteArray::Iterator tmp_iterator = (QByteArray::Iterator)message.begin();

    if(!this->isOk){
        return;
    }

    while((tmp_iterator != message.end()) && (all_length < message.length() - 1)){
        tmp_map.clear();
        //memset(buffer, 0, RECV_BUF_SIZE);
        for(int i = 0; i < 4; ++i){
            ((unsigned char*)&packet_size)[i] = *tmp_iterator;
            ++tmp_iterator;
            ++all_length;
        }

        if(packet_size - 2 > MAX_RECV_BUF_SIZE){
//            std::cerr << "max buffer, return\n";
            tmp_iterator += packet_size;
            all_length += packet_size;
            continue;
        }

        tmp_iterator = tmp_iterator + 8;
        all_length += 8;
        int i;
        for(i = 0; i < packet_size - 8 && all_length < message.length() - 1; ++i){
            buffer[i] = *tmp_iterator;
            ++tmp_iterator;
            ++all_length;
        }

        buffer[i] = 0;
        buffer[i + 1] = 0;
        QString tmp_string(buffer);
        QRegExp tmp_regexp("(.*)@=(.*)/");
        tmp_regexp.setPatternSyntax(QRegExp::RegExp2);
        tmp_regexp.setMinimal(true);
        int pos = 0;
        while((pos = tmp_regexp.indexIn(tmp_string, pos)) != -1){
            if(tmp_regexp.captureCount() == 2){
                tmp_map[tmp_regexp.cap(1)] = tmp_regexp.cap(2);
                    pos += tmp_regexp.matchedLength();
            }
        }

        if(tmp_map["type"] == "chatmsg" || tmp_map["type"] == "dgb" || tmp_map["type"] == "noble_num_info"){
            queue_mtx.lock();
            queue_with_sql.enqueue(tmp_map);
            queue_mtx.unlock();
        }else if(tmp_map["type"] == "loginres"){   //登录成功
            emit have_login();
            if(!sql_thread->isRunning())
                sql_thread->start();
            if(sql_thread->isRunning()){
                Douyu_Danmu::show_info("INFO", "login successfully");
                this->isConnected = true;
            }
            else{
                Douyu_Danmu::show_info("ERROR", "login successful but fail to start database thread");
                this->isOk = false;
            }
        }else if(tmp_map["type"] == "mrkl"){    //心跳
            this->isKeepAlive = true;
        }else if(tmp_map["type"] == "rss"){     //主播开关播
            if(tmp_map["rid"] == roomID){
                if(tmp_map["ss"] == "0"){
                    //主播下播
                }else if(tmp_map["ss"] == "1"){
                    //主播开播
                }
            }
        }else{
            this->log_file->write(tmp_string.toUtf8());
            this->log_file->write("\n");
        }
    }
}

void Douyu_Danmu::onWebSocketLogin(void){
    this->joinGroup();
    this->keepAlive();

    this->timer->start();
}


bool Douyu_Danmu::initSql(QSqlDatabase &database){
    QString chatmsg =
    "CREATE TABLE IF NOT EXISTS chatmsg("
    "cid varchar(35) primary key,"
    "uid varchar(12),"
    "name varchar(36),"
    "level smallint,"
    "device smallint,"
    "badge varchar(15),"
    "blevel smallint,"
    "txt varchar(70),"
    "color smallint,"
    "time timestamp);";

    QString dgb =
    "CREATE TABLE IF NOT EXISTS dgb("
    "no integer primary key,"
    "uid varchar(12),"
    "name varchar(36),"
    "level smallint,"
    "device smallint,"
    "badge varchar(15),"
    "blevel smallint,"
    "gift varchar(20),"
    "hits smallint);";

    QString noble_num =
    "CREATE TABLE IF NOT EXISTS noble_num("
    "time timestamp primary key,"
    "num int);";

    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName(roomID + ".db");
    if(!database.open()){
        Douyu_Danmu::show_info("ERROR", "failed to open sqlite databases");
        std::cerr << database.lastError().text().toStdString() << std::endl;
        return false;
    }

    QSqlQuery tmp_query;

    if(!tmp_query.exec(chatmsg)){
        Douyu_Danmu::show_info("ERROR", "failed to create table in database" + tmp_query.lastError().text());
        database.close();
        return false;
    }

    if(!tmp_query.exec(dgb)){
        Douyu_Danmu::show_info("ERROR", "failed to create table in database" + tmp_query.lastError().text());
        database.close();
        return false;
    }

    if(!tmp_query.exec(noble_num)){
        Douyu_Danmu::show_info("ERROR", "failed to create table in database" + tmp_query.lastError().text());
        database.close();
        return false;
    }

    return true;
}

bool Douyu_Danmu::connectServer(void){
    QUrl tmp_url(this->websocket_server_addr);
    if(!tmp_url.isValid()){
        Douyu_Danmu::show_info("ERROR", "connect to server failed");
        return false;
    }
    this->websocket->open(tmp_url);
    return true;
}

void Douyu_Danmu::makePackage(package* pack, const char* data, usint data_len){
//    std::cerr << "enter 2\n";
    usint length = data_len + 8;
//    memset(pack, 0, sizeof(package));

    pack->length1 = pack->length2 = length;

    pack->type = this->package_type_from_client;

    memcpy(pack->data, data, data_len);
//    std::cerr << "leave 2\n";

    return;
}

bool Douyu_Danmu::login(void){
    package pack;
    QString data("type@=loginreq/roomid@=");
    qint64 status = 0;

    data += roomID;
    data += "/ct@=0/\0";
    this->makePackage(&pack, data.toLocal8Bit().data(), data.size() + 1);
    
    if(!this->websocket->isValid()){
        return false;
    }
    status = this->websocket->sendBinaryMessage(QByteArray((const char*)&pack, pack.length1 + 4)); // +4(extra package length2 wasn't included)
    if(status <= 0){
        Douyu_Danmu::show_info("ERROR", "failed to send login msg");
        return false;
    }
    return true;
}

void Douyu_Danmu::keepAlive(void){
    if(!this->isOk){
        return;
    }

    int status = this->websocket->sendBinaryMessage(QByteArray((const char*)&(this->tmp_package_to_keep_alive), this->tmp_package_to_keep_alive.length1 + 4)); // +4(extra package length2 wasn't included)
    if(status <= 0){
        isKeepAlive = false;
        this->timer->stop();
        this->timer->setInterval(5000);
        this->timer->start();
    }else{
        if(!(this->isKeepAlive)){
            isKeepAlive = true;
            this->timer->stop();
            this->timer->setInterval(KEEPALIVE_INTERVEL_MS);
            this->timer->start();
        }
    }
}

bool Douyu_Danmu::joinGroup(void){
    package tmp_pack;
    qint64 status = 0;

    QString content = "type@=joingroup/rid@=";
    content += roomID;
    content += "/ver@=20190610/aver@=218101901/ct@=0\0";
    
    this->makePackage(&tmp_pack, content.toLocal8Bit().data(), content.size() + 1 );

    if(!this->websocket->isValid()){
        return false;
    }
    status = this->websocket->sendBinaryMessage(QByteArray((const char*)&tmp_pack, tmp_pack.length1 + 4)); // +4(extra package length2 wasn't included)
    if(status <= 0){
        Douyu_Danmu::show_info("ERROR", "failed to send join group msg");
        return false;
    }
    return true;
}

void Douyu_Danmu::logout(void){
    package tmp_pack;
    short int status = 0;
    std::string msg("type@=logout/");

    this->makePackage(&tmp_pack, msg.c_str(), 13);
    status = this->websocket->sendBinaryMessage(QByteArray((const char*)&tmp_pack, tmp_pack.length1 + 4)); // +4(extra package length2 wasn't included)
    if(status <= 0){
        std::cout << "ERROR: logout failed" << std::endl;
    }
}

void Douyu_Danmu::show_info(QString tag, QString info){
    std::cerr << "[" << roomID.toStdString() << "][" << QDateTime::currentDateTime().toString().toStdString() << "]" << tag.toStdString() << ": " <<
                      info.toStdString() << std::endl;
}

void Douyu_Danmu::sql_actions(){
    QSqlDatabase tmp_database;
    if(!initSql(tmp_database)){
        Douyu_Danmu::show_info("ERROR", "failed to init database");
        return;
    }

    QMap<QString, QString> tmp_map;
    QQueue<QMap<QString, QString>> tmp_queue;
    bool isContinue = true;

    while(1){
        isThreadContinue_mtx.lock();
        if(!isThreadContinue){
            isContinue = false;
        }
        isThreadContinue_mtx.unlock();

        if(!tmp_database.isOpen()){
            Douyu_Danmu::show_info("ERROR", "failed to open database");
            break;
        }

        queue_mtx.lock();
        for(char i = 0; (i < MAX_SQL_RECORD_HANDLE_PER_TIME) && (queue_with_sql.length() > 0); ++i){
            tmp_queue.enqueue(queue_with_sql.dequeue());
        }
        queue_mtx.unlock();

        tmp_database.transaction();
        while(tmp_queue.length() > 0){
            tmp_map = tmp_queue.dequeue();
            if(tmp_map["type"] == "chatmsg"){
                //"INSERT INTO chatmsg VALUES(:cid, :uid, :name, :level, :device, :badge, :blevel, :txt, :color, :time);";
                QString tmp_string = QString(INSERT_CHAT).arg(
                            tmp_map["cid"], tmp_map["uid"], tmp_map["nn"], tmp_map["level"],
                            (tmp_map["ct"] != ""?tmp_map["ct"]:"null"), (tmp_map["bnn"] != ""?tmp_map["bnn"]:"null"), (tmp_map["bl"] != ""?tmp_map["bl"]:"null"),
                            tmp_map["txt"], (tmp_map["col"] != ""?tmp_map["col"]:"null")).arg(tmp_map["cst"].mid(0, 10));

                tmp_database.exec(tmp_string);
            }else if(tmp_map["type"] == "dgb"){
                QString gift_name;
                if(gift_id_map.contains(tmp_map["gfid"])){
                    gift_name = gift_id_map[tmp_map["gfid"]];
                }else{
                    gift_name = tmp_map["gfid"];
                }
                //INSERT INTO dgb VALUES(null, :uid, :name, :level, :device, :badge, :blevel, :gift, :hits);;
                QString tmp_string = QString(INSERT_DGB).arg(tmp_map["uid"], tmp_map["nn"],
                        tmp_map["level"], tmp_map["ct"],(tmp_map["bnn"] != ""?tmp_map["bnn"]:"null"), (tmp_map["bl"] != ""?tmp_map["bl"]:"null"),
                        gift_name, tmp_map["hits"]);
                tmp_database.exec(tmp_string);
            }else if(tmp_map["type"] == "noble_num_info"){
                QString tmp_string = QString(INSERT_NOBLE_NUM).arg(QString::number(QDateTime::currentSecsSinceEpoch()),
                tmp_map["vn"]);
                tmp_database.exec(tmp_string);
            }
        }
        if(!tmp_database.commit()){
            Douyu_Danmu::show_info("Warning", "failed to insert data into database " + tmp_database.lastError().text());
        }

        queue_mtx.lock();
        if(!(isContinue || !queue_with_sql.isEmpty())){
            queue_mtx.unlock();
            break;
        }
        queue_mtx.unlock();
        QThread::msleep(200);
    }

    tmp_database.close();
    return;
}
