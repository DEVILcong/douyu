# 依赖
使用前需要安装sqlite3的库(sqlite3.h)，根据你使用的linux发行版的不同，软件包的名字可能为libsqlite3.xxx或者sqlite3-devel-xxx，不安装的话会无法编译通过。

# 大体使用方法
1. make(编译，生成可执行文件test_douyu);
2. ./test_douyu 290935 (生成的可执行文件加斗鱼房间号，运行后会在当前目录下生成sqlite数据库文件，文件名为"斗鱼房间号.db", 例如"290935.db");
3. 停止运行 在命令行中输入“e”回车后程序停止，注意：在弹幕量较多的时候可能要多试几次才会有效果。

# 文件结构
douyu_danmu.hpp douyu_danmu.cpp  主要函数

test_douyu.cpp 主（main）函数，使用主要功能函数

# 数据库描述
两张表chatmsg(弹幕信息)、dgb(礼物信息)，同一个房间号数据库可以保存程序多次运行的数据

## chatmsg
CREATE TABLE chatmsg(

    cid varchar(35) primary key,   //弹幕唯一标识，斗鱼提供

    uid varchar(12),               //斗鱼用户ID

    name varchar(36),              //用户昵称

    level smallint,                //用户等级

    device smallint,               //发送弹幕使用的设备

    badge varchar(15),             //用户带的牌子的名称

    blevel smallint,               //用户带的牌子的等级

    txt varchar(70),               //弹幕内容

    color smallint,                //弹幕颜色

    time timestamp);               //弹幕时间戳


## dgb
CREATE TABLE dgb(

    no integer primary key,        //序号，从1开始自增

    uid varchar(12),               //斗鱼用户ID

    name varchar(36),              //用户昵称

    level smallint,                //用户等级

    device smallint,               //赠送礼物所用的设备

    badge varchar(15),             //用户带的牌子的名称

    blevel smallint,               //用户带的牌子的等级

    gift varchar(20),              //礼物名称(弱鸡、稳等)，在douyu_danmu.cpp中开头有表

    hits smallint);                //数量（注意：仅作参考！！！！）

