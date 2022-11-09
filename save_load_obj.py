import pickle


def save_variable(v, filename):
    f = open(filename, 'wb')  # 打开或创建名叫filename的文档。
    pickle.dump(v, f)  # 在文件filename中写入v
    f.close()  # 关闭文件，释放内存。
    return filename


def load_variavle(filename):
    try:
        f = open(filename, 'rb+')
        r = pickle.load(f)
        f.close()
        return r

    except EOFError:
        return ""


if __name__ == '__main__':
    a = [1, 2, 3]
    import numpy as np
    import os
    b = np.ones((5, 5))
    # filename = 'ab.pkl'
    file_save = os.path.join('wyx_data', 'test1.pkl')
    # if not os.path.exists('wyx_data/'):  # 判断所在目录下是否有该文件名的文件夹
    #     os.mkdir('wyx_data')  # 创建多级目录用mkdirs，单击目录mkdir
    # save_variable([a, b], file_save)

    if not os.path.exists(file_save):
        print('No file exists')
    # data_a, data_b = load_variavle(file_save)
    # print(data_b)
