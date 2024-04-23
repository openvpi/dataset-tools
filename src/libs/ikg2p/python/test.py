import time

from Zh_G2p import ZhG2p

g2p = ZhG2p("mandarin")

error = 0
count = 0
start_time = time.time()
with open('op_lab.txt', 'r', encoding='utf-8') as f:
    lines = f.readlines()
    for line in lines:
        raw, pinyin = line.strip("\n").split("|")
        count += len(raw)
        res = g2p.convert(raw)
        if res != pinyin:
            for i, j in zip(pinyin.split(" "), res.split(" ")):
                if i != j:
                    error += 1

end_time = time.time()

elapsed_time = end_time - start_time
print(f"错字数: {error}")
print(f"错误率: {round(error / count * 100, 2)}%")
print(f"执行时间为: {elapsed_time}秒")
