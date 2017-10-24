import os
import csv
from os.path import expanduser

def src_file_path(src_name):
    CWD = os.getcwd()
    return os.path.join(CWD, '..','engine','src', '{}.cpp'.format(src_name))

def load_cpp_file(src_name):
    PATH = src_file_path(src_name)
    with open(PATH, 'r') as f:
        return list(f.readlines())

def parse_file(lines):
    current_mode = []
    ret_dict = {}
    for line in lines:
        split_line = line.split()
        if not split_line:
            continue
        elif split_line[0] in ['case', 'switch', '{', '}', '//']:
            continue
        elif split_line[0][0] == 'M':
            name = split_line[1].split('(')[0][1:].split('_')[-1]
            current_mode.append(name)
        elif split_line[0] == 'return':
            ret_dict[split_line[-1][:-1].split('(')[0]] = current_mode[-1]
    return ret_dict


def create_dict_with_header(ret_dict):
    for key in ret_dict:
        yield dict(className=key, classType=ret_dict[key])


def write_csv(ret_dicts):
    homedir = expanduser('~')
    PATH = os.path.join(homedir, 'livecode_syntax_classes.csv')
    with open(PATH, 'w+') as f:
        fieldnames = ['className', 'classType']
        writer = csv.DictWriter(f, fieldnames)
        writer.writeheader()

        for dct in ret_dicts:
            writer.writerow(dct)
        
    
if __name__ == "__main__":
    cpp_file = load_cpp_file('newobj')
    dicts = create_dict_with_header(parse_file(cpp_file))
    write_csv(dicts)
            
