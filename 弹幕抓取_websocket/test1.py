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

union_header = {
    'Host': 'danmuproxy.douyu.com:8503',
    'Origin':'https://www.douyu.com',
    'Sec-WebSocket-Extentions':'permessage-deflate; client_max_window_bits',
    'Sec-WebSocket-Version':'13',
    'User-Agent':'Chrome/81.0.4044.129' 
    }
login_content = {
    'type':'loginreq',
    'roomid':'290935',
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

a = add_header(login_content)
print(a.hex())
