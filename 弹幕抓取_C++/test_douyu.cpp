#include"douyu_danmu.hpp"

int main(int argc, char** argv){
    Douyu_Danmu dd(std::stoul(argv[1]));

    dd.initSqlite();
    dd.connectServ();
    dd.login();
    dd.recv_message();

    dd.logout();
    
    return 0;
}
