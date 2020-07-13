#include"douyu_danmu.hpp"
#include <thread>
#include <unordered_map>

bool isKeepAlive = true;

int main(int argc, char** argv){
    Douyu_Danmu dd(std::stoul(argv[1]));

    dd.initSqlite();
    bool status = dd.connectServ();
    if(status)
        std::cout << "hahahahaha" << std::endl;

    status = dd.login();
    dd.recv_message();

    dd.logout();
    
    return 0;
}
