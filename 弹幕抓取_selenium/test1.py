import time
import random
import csv
from selenium import common

def get_danmu(web, sleepTime):
    sLoop = 1
    bLoop = 1
    notFoundNum = 0
    contentD = {}

    while sLoop < 50:
        a = web.find_elements_by_xpath('//li[contains(@class, "Barrage-listItem")]')
        data = time.strftime('%Y-%m-%d-%H:%M:%S', time.localtime())
        print('circle is ', sLoop, ' ', len(a))
        for item in a:
            itemId = item.id
            try:
                name = item.find_element_by_xpath('.//span[contains(@class, "Barrage-nickName")]').text
                content = item.find_element_by_xpath('.//span[contains(@class, "Barrage-content")]').text
                
                contentD[itemId] = (data, name[:-1], content)
            except common.exceptions.NoSuchElementException as e:
                notFoundNum = notFoundNum + 1
                #print('no such elemnt ', name)
                content = '富哥们儿来了或者送礼的'
                contentD[itemId] = (data, name[:-1], content)
            except common.exceptions.StaleElementReferenceException:
                print('miss one')
                continue

        sLoop = sLoop + 1
            
        if len(contentD) > 500:
            with open(''.join(('danmu', str(bLoop), '.csv')), 'w') as file1:
                writer = csv.writer(file1)
                for item in contentD.items():
                    writer.writerow((item[0], item[1][0], item[1][1], item[1][2]))
            contentD.clear()
            print('----->>>>>>write ', bLoop, '<<<<<<-----')
            bLoop = bLoop + 1

        print('Dict size is ', len(contentD))
        print('not found num is ', notFoundNum)
        print('----------------------------------')
        notFoundNum = 0
        time.sleep(sleepTime)

    with open(''.join(('danmu', str(bLoop), '.csv')), 'w') as file1:
        writer = csv.writer(file1)
        for item in contentD.items():
            writer.writerow((item[0], item[1][0], item[1][1], item[1][2]))
        contentD.clear()
        print('----->>>>>>write ', bLoop, '<<<<<<-----')

