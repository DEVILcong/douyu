#include <QCoreApplication>
#include "douyu_danmu.hpp"
#include <signal.h>
#include <iostream>
#include <QString>

QCoreApplication *a;

void sig_handler(int signum){
    a->exit();
}

int main(int argc, char *argv[])
{
    a = new QCoreApplication(argc, argv);

    if(argc != 2){
        std::cout << "Usage: danmu_getter your_room_id" << std::endl;
        delete a;
        return 0;
    }

    struct sigaction new_sigaction, old_sigaction;
    sigemptyset(&new_sigaction.sa_mask);
    new_sigaction.sa_handler = sig_handler;
    new_sigaction.sa_flags = 0;
    if(sigaction(SIGINT, &new_sigaction, &old_sigaction) < 0){
        Douyu_Danmu::show_info("ERROR", "failed to set signal handler");
        delete a;
        return 0;
    }

    if(sigaction(SIGTERM, &new_sigaction, &old_sigaction) < 0){
        Douyu_Danmu::show_info("ERROR", "failed to set signal handler");
        delete a;
        return 0;
    }

    Douyu_Danmu tmp_danmu(a, a, QString(argv[1]));
    tmp_danmu.connectServer();

    a->exec();
    return 0;
}
