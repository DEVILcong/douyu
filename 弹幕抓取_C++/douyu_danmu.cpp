#include"douyu_danmu.hpp"

Gifts::Gifts(){}
Gifts::~Gifts(){}

std::unordered_map<std::string, std::string> Gifts::gift_type = {
    {"20005", "超级火箭"},
    {"20004", "火箭"},
    {"20003", "飞机"},
    {"20002", "办卡"},
    {"20010", "MVP"},
    {"20541", "大气"},
    {"20000", "100鱼丸"},
    {"20709", "壁咚"},
    {"20751", "Mojito之夏"},
    {"20755", "夏日火箭"},
    {"20417", "福袋"},
    {"200124", "对A"},
    {"193", "弱鸡"},
    {"192", "赞"},
    {"520", "稳"},
    {"712", "棒棒哒"},
    {"824", "荧光棒"},
    {"1823", "粉丝荧光棒"}};


Socket::Socket(){}

Socket::Socket(int id) : ID(id){}

Socket::~Socket(){
    close(this->ID);
}

int Socket::getID(){
    return this->ID;
}

void Socket::operator= (int id){
    this->ID = id;
}

Douyu_Danmu::Douyu_Danmu(uint rid):roomID(rid){
    this->db = nullptr;
    this->insert_chatmsg = nullptr;
    this->insert_dgb = nullptr;

    this->isEnableSqlite = false;
    this->isKeepAlive = true;
    this->isKeepRecv = true;
    this->isKeepRunning = true;
}

Douyu_Danmu::~Douyu_Danmu(){
    int status = sqlite3_finalize(this->insert_chatmsg);
    if(status != SQLITE_OK){
        std::cerr << "ERROR: failed to delete sql statement" << std::endl;
        std::cerr << sqlite3_errstr(status) << std::endl;
    }

    status = sqlite3_finalize(this->insert_dgb);
    if(status != SQLITE_OK){
        std::cerr << "ERROR: failed to delete sql statement" << std::endl;
        std::cerr << sqlite3_errstr(status) << std::endl;
    }

    status = sqlite3_close_v2(this->db);
    if(status != SQLITE_OK){
        std::cerr << "ERROR: failed to close sql connection" << std::endl;
        std::cerr << sqlite3_errstr(status) << std::endl;
    }

}

int Douyu_Danmu::getSocketID(void){
    return this->socketID.getID();
}

bool Douyu_Danmu::initSqlite(void){
    char* errmsg = nullptr;

    std::string chatmsg = 
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

    std::string dgb = 
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

    std::string insert_chat = "INSERT INTO chatmsg VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    std::string insert_d = "INSERT INTO dgb VALUES(null, ?, ?, ?, ?, ?, ?, ?, ?);";

    int status = sqlite3_open((std::to_string(this->roomID) + ".db").c_str(), &(this->db));
    if(status != SQLITE_OK){
        std::cerr << "ERROR: failed to open sqlite databases" << std::endl;
        std::cerr << sqlite3_errstr(status) << std::endl;
        return false;
    }

    sqlite3_exec(this->db, chatmsg.c_str(), NULL, NULL, &errmsg);
    if(errmsg != nullptr){
        std::cerr << "ERROR: failed to create table in database" << std::endl;
        std::cerr << errmsg << std::endl;
        sqlite3_free(errmsg);
        return false;
    }

    sqlite3_exec(this->db, dgb.c_str(), NULL, NULL, &errmsg);
    if(errmsg != nullptr){
        std::cerr << "ERROR: failed to create table in database" << std::endl;
        std::cerr << errmsg << std::endl;
        sqlite3_free(errmsg);
        return false;
    }
    
    status = sqlite3_prepare_v2(this->db, insert_chat.c_str(), insert_chat.size(), &(this->insert_chatmsg), NULL);
    if(status != SQLITE_OK){
        std::cerr << "ERROR: failed to create sql statement" << std::endl;
        std::cerr << sqlite3_errstr(status) << std::endl;
        return false;
    }

    status = sqlite3_prepare_v2(this->db, insert_d.c_str(), insert_d.size(), &(this->insert_dgb), NULL);
    if(status != SQLITE_OK){
        std::cerr << "ERROR: failed to create sql statement" << std::endl;
        std::cerr << sqlite3_errstr(status) << std::endl;
        return false;
    }

    this->isEnableSqlite = true;
    return true;
}

bool Douyu_Danmu::connectServ(void){
    int s, sfd;
    struct addrinfo hints;
    struct addrinfo *result = nullptr, *rp = nullptr;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 6;    //tcp described in /etc/protocol

    s = getaddrinfo(DANMU_URL, DANMU_PORT, &hints, &result);
    if(s != 0){
        std::cerr << "ERROR: function getaddrinfo in connect:" << gai_strerror(s) << std::endl;
        return false;
    }

    for(rp = result; rp != nullptr; rp = rp -> ai_next){
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(-1 == sfd)
            continue;

        if(connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;
    }

    if(rp == nullptr){
        std::cerr << "ERROR: connect to address failed" << std::endl;
        freeaddrinfo(result);
        return false;
    }
    else{
        std::clog << "INFO: successfully connect to the server" << std::endl;
        this->socketID = sfd;
        //std::cout << sfd << std::endl;
        freeaddrinfo(result);

        struct timeval tv = {14, 0};  //set socket timeout
        if(setsockopt(this->socketID.getID(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0){
            std::cerr << "WARNING: failed to set socket timeout" << std::endl;
            std::cerr << strerror(errno) << std::endl;
        }
        return true;
    }
}

void Douyu_Danmu::makePackage(package* pack, const char* data, usint data_len){
    usint length = data_len + 8;
    memset(pack, 0, sizeof(package));

    pack->length1 = pack->length2 = length;

    pack->type = PACKAGE_T_FROM_C;

    memcpy(pack->data, data, data_len);

    return;
}

std::unordered_map<std::string, std::string> Douyu_Danmu::parsePackage(package* pack){
    std::unordered_map<std::string, std::string> tmp_map;
    std::string content(pack->data);
    std::regex item1("(.*?)@=(.*?)/");
    std::smatch match1;

    while(std::regex_search(content, match1, item1)){
        if(match1.size() >= 3){
            tmp_map.emplace(match1[1], match1[2]);
            content = match1.suffix().str();
        }
        else{
            std::cerr << "WARNING: something wrong in parsePackage" << std::endl;
        }
    }

    return tmp_map; 
}

bool Douyu_Danmu::login(void){
    char buffer[RECV_BUF_SIZE];
    package pack;
    package* tmp_pack = nullptr;
    std::string data("type@=loginreq/roomid@="); 
    short int status = 0;

    memset(buffer, 0, RECV_BUF_SIZE);

    data += std::to_string(this->roomID);
    data += "/\0";
    this->makePackage(&pack, data.c_str(), data.size() + 1);
    
    status = send(this->getSocketID(), &pack, pack.length1 + 4, 0);   // +4(extra package length2 wassn't included)
    if(status <= 0){
        std::cerr << "ERROR: failed to send in function login" << std::endl;
        std::cerr << strerror(errno) << std::endl;
        return false;
    }

    status = recv(this->socketID.getID(), buffer, RECV_BUF_SIZE, 0);
    if(status <= 0){
        std::cerr << "ERROR: failed to login" << std::endl;
        std::cerr << (int)status << " " << strerror(errno) << std::endl;
        return false;
    }else{
        //tmp_pack = (package*)buffer;
        //std::cout << tmp_pack->data << std::endl;
        return true;
    }
}

void Douyu_Danmu::keepAlive(int ID, bool* KeepAlive){
    package tmp_pack;
    short int status = 0;

    std::chrono::seconds span(5);

    char info[] = "type@=mrkl/\0";
    makePackage(&tmp_pack, info, 12);
    
    while(*KeepAlive){
        status = send(ID, &tmp_pack, tmp_pack.length1 + 4, 0);
        if(status <= 0){
            std::cerr << "WARNING: send keep alive packet error" << std::endl;
            std::cerr << strerror(errno) << std::endl;
        }
        for(char i = 0; i < 8 && *KeepAlive; ++i)
            std::this_thread::sleep_for(span);
    }
    return;
}

bool Douyu_Danmu::joinGroup(void){
    package tmp_pack;
    package* tmp_pack_ptr = nullptr;
    char buffer[RECV_BUF_SIZE];
    short int status = 0;

    std::string content = "type@=joingroup/rid@=";
    content += std::to_string(this->roomID);
    content += "/gid=-9999/\0";
    
    memset(buffer, 0, RECV_BUF_SIZE);
    this->makePackage(&tmp_pack, content.c_str(), content.size() + 1 );
    status = send(this->socketID.getID(), &tmp_pack, tmp_pack.length1 + 4, 0);
    if(status <= 0){
        std::cerr << "ERROR: failed to join group(send)" << std::endl;
        std::cerr << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

void Douyu_Danmu::recvPackage(int ID, std::queue<std::unordered_map<std::string, std::string>>& p, bool* KeepRecv, std::mutex& queue_lk){
    char buffer[RECV_BUF_SIZE];
    memset(buffer, 0, RECV_BUF_SIZE);

    short int status = 0;

    while(*KeepRecv){
        status = recv(ID, buffer, RECV_BUF_SIZE, 0);
        if(status <= 0){
            std::cerr << "WARNING: failed to recv" << std::endl;
            std::cerr << strerror(errno) << std::endl;
            continue;
        }else{
            queue_lk.lock();
            p.emplace(parsePackage((package*)buffer));
            memset(buffer, 0, RECV_BUF_SIZE);
            queue_lk.unlock();
        }
    }
}

void Douyu_Danmu::control(bool* KeepRunning){
    std::string command;

    for(int i = 0; i < 2400; ++i){
        std::cin >> command;
        if(command == "e"){
            *KeepRunning = false;
            break;
        }
    }
    return;
}

void Douyu_Danmu::recv_message(void){
    if(!this->joinGroup()){
        std::cout << "ERROR: failed to join group" << std::endl;
        std::cout << strerror(errno) << std::endl;
        return;
    }

    std::thread keep(keepAlive, this->getSocketID(), &(this->isKeepAlive));
    std::thread main_recv(recvPackage, this->getSocketID(), std::ref(this->packages), &(this->isKeepRecv), std::ref(this->queue_lock));
    std::thread control_t(control, &(this->isKeepRunning));

    std::this_thread::sleep_for(std::chrono::seconds(1));

    int status = 0;
    while(1){
        this->queue_lock.lock();
        if(!this->packages.empty()){
            std::unordered_map<std::string, std::string> p(this->packages.front());
            this->queue_lock.unlock();

            std::cout << p["type"] << "\t" << p["rid"]  << "\t" << p["txt"] << "  " << packages.size()<< std::endl;
            if( p["type"] == "chatmsg"){
                sqlite3_bind_text(this->insert_chatmsg, 1, p["cid"].c_str(), p["cid"].size(), SQLITE_STATIC);
                sqlite3_bind_text(this->insert_chatmsg, 2, p["uid"].c_str(), p["uid"].size(), SQLITE_STATIC);
                sqlite3_bind_text(this->insert_chatmsg, 3, p["nn"].c_str(), p["nn"].size(), SQLITE_STATIC);
                sqlite3_bind_text(this->insert_chatmsg, 8, p["txt"].c_str(), p["txt"].size(), SQLITE_STATIC);
                sqlite3_bind_text(this->insert_chatmsg, 6, p["bnn"].c_str(), p["bnn"].size(), SQLITE_STATIC);

                try{
                    sqlite3_bind_int(this->insert_chatmsg, 4, std::stoi(p["level"]));
                }catch(std::invalid_argument& e){
                    sqlite3_bind_null(this->insert_chatmsg, 4);
                }

                try{
                    sqlite3_bind_int(this->insert_chatmsg, 5, std::stoi(p["ct"]));
                }catch(std::invalid_argument& e){
                    sqlite3_bind_null(this->insert_chatmsg, 5);
                }

                try{
                    sqlite3_bind_int(this->insert_chatmsg, 7, std::stoi(p["bl"]));
                }catch(std::invalid_argument& e){
                    sqlite3_bind_null(this->insert_chatmsg, 7);
                }

                try{
                    sqlite3_bind_int(this->insert_chatmsg, 9, std::stoi(p["col"]));
                }catch(std::invalid_argument& e){
                    sqlite3_bind_null(this->insert_chatmsg, 9);
                }

                try{
                    sqlite3_bind_int(this->insert_chatmsg, 10, std::stoi(p["cst"].substr(0, 10)));
                }catch(std::invalid_argument& e){
                    sqlite3_bind_null(this->insert_chatmsg, 10);
                }
                    
                status = sqlite3_step(this->insert_chatmsg);
                if(status != SQLITE_DONE){
                    std::cerr << "WARNING: failed to insert database" << std::endl;
                    std::cerr << sqlite3_errstr(status) << std::endl;
                }

                status = sqlite3_reset(this->insert_chatmsg);
                if(status != SQLITE_OK){
                    std::cerr << "WARNING: reset statement error" << std::endl;
                    std::cerr << sqlite3_errstr(status) << std::endl;
                }
            }else if(p["type"] == "dgb"){
                sqlite3_bind_text(this->insert_dgb, 1, p["uid"].c_str(), p["uid"].size(), SQLITE_STATIC);
                sqlite3_bind_text(this->insert_dgb, 2, p["nn"].c_str(), p["nn"].size(), SQLITE_STATIC);
                //sqlite3_bind_int(this->insert_dgb, 3, std::stoi(p["level"]));
                //sqlite3_bind_int(this->insert_dgb, 4, std::stoi(p["ct"]));
                sqlite3_bind_text(this->insert_dgb, 5, p["bnn"].c_str(), p["bnn"].size(), SQLITE_STATIC);
                //sqlite3_bind_int(this->insert_dgb, 6, std::stoi(p["bl"]));

                try{
                    sqlite3_bind_int(this->insert_dgb, 3, std::stoi(p["level"]));
                }catch(std::invalid_argument& e){
                    sqlite3_bind_null(this->insert_dgb, 3);
                }

                try{
                    sqlite3_bind_int(this->insert_dgb, 4, std::stoi(p["ct"]));
                }catch(std::invalid_argument& e){
                    sqlite3_bind_null(this->insert_dgb, 4);
                }

                try{
                    sqlite3_bind_int(this->insert_dgb, 6, std::stoi(p["bl"]));
                }catch(std::invalid_argument& e){
                    sqlite3_bind_null(this->insert_dgb, 6);
                }


                std::string gift_name;
                if(Gifts::gift_type[p["gfid"]] != ""){
                    gift_name = Gifts::gift_type[p["gfid"]];
                }else{
                    gift_name = p["gfid"];
                }
                sqlite3_bind_text(this->insert_dgb, 7, gift_name.c_str(), gift_name.size(), SQLITE_STATIC);

                try{
                    sqlite3_bind_int(this->insert_dgb, 8, std::stoi(p["hits"]));
                }catch(std::invalid_argument &e){
                    sqlite3_bind_null(this->insert_dgb, 8);

                }

                status = sqlite3_step(this->insert_dgb);
                if(status != SQLITE_DONE){
                    std::cerr << "WARNING: failed to insert database" << std::endl;
                    std::cerr << sqlite3_errstr(status) << std::endl;
                }

                status = sqlite3_reset(this->insert_dgb);
                if(status != SQLITE_OK){
                    std::cerr << "WARNING: reset statement error" << std::endl;
                    std::cerr << sqlite3_errstr(status) << std::endl;
                }
            }
            
            this->queue_lock.lock();
            this->packages.pop();
        }
        this->queue_lock.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if(!this->isKeepRunning)
            break;
    }

    this->isKeepAlive = false;
    this->isKeepRecv = false;

    control_t.join();
    keep.join();
    main_recv.join();

    return;
}

void Douyu_Danmu::logout(void){
    package tmp_pack;
    short int status = 0;
    std::string msg("type@=logout/");

    this->makePackage(&tmp_pack, msg.c_str(), 13);
    status = send(this->getSocketID(), &tmp_pack, tmp_pack.length1 + 4, 0);
    if(status <= 0){
        std::cout << "ERROR: logout failed" << std::endl;
        std::cout << strerror(errno) << std::endl;
    }
}
