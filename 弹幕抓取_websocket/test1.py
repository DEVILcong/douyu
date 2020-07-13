import asyncio
from aiowebsocket.converses import AioWebSocket

def add_header(content_dict):
    '''
    应用层加包头
    '''
    tmp_str_b = ''
    tmp_str_s = ''
    for item in content_dict.items():
        tmp_str_s = '@='.join(item)
        tmp_str_b = '/'.join((tmp_str_b, tmp_str_s))

    tmp_str_b = tmp_str_b.encode() + b'\x2f\x00'
    length = len(tmp_str_b) + 7
    #print(length)
    length = length.to_bytes(4, 'little')  #int in little ending
    
    fix_data = b'\xb1\x02\x00\x00'
    
    return length + length + fix_data + tmp_str_b

async def login_proc(url_str, union_header, login_content):
    async with AioWebSocket(url_str, union_header = union_header) as aws:
        converse = aws.manipulator
        message = add_header(login_content)

        #await converse.send(message)
        #print('message sent', message)

        while True:
            message = await converse.receive()
            print('get message ', message)

union_header = {
    'Accept-Encoding':'gzip, deflate, br',
    'Accept-Language':'en-US,en;q=0.9,zh-CN;q=0.8,zh-TW;q=0.7,zh;q=0.6',
    'Host': 'danmuproxy.douyu.com:8501',
    'Origin':'https://www.douyu.com',
    'Connection':'Upgrade',
    'Upgrade':'websocket',
    'Sec-WebSocket-Key':' 5dY5OE2EUgvGWNvxRDj00Q==',
    'Sec-WebSocket-Extentions':'permessage-deflate; client_max_window_bits',
    'Sec-WebSocket-Version':'13',
    'User-Agent':'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/81.0.4044.129 Safari/537.36' 
    }
login_content = {
    'type':'loginreq',
    'roomid':'94228',
    'dfl':'sn@AA=105@ASss@AA=1',
    'username':'337596139',
    'uid':'337596139',
    'ver':'20190610',
    'aver':'218101901',
    'ct':'0'
    }
join_group_content = {
    'type':'joingroup',
    'rid':'290935',
    'gid':'1'
    }
logout_content = {'type':'logout'}

asyncio.get_event_loop().run_until_complete(login_proc('ws://danmuproxy.douyu.com:8501', union_header, login_content))
