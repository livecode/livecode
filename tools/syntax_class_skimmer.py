import os
import csv
from os.path import expanduser

EXEC_FILES = [
    'ask',
    'cmds',
    'answer',
    'cmdsc',
    'cmdse',
    'cmdsf',
    'cmdsm',
    'cmdsp',
    'cmdss',
    'deploy',
    'visual',
    'statemnt',
    'keywords',
    'ide'
]

class SyntaxClassReader:
    def __init__(self, syntax_file, exec_files):
        self.syntax_file = self.create_dict_with_header(
            self.parse_file(
                self.load_cpp_file(
                    syntax_file
                )
            ),
            self.parse_multiple_exec_files(exec_files)
        )

    def parse_file(self, lines):
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

    def src_file_path(self, src_name):
        CWD = os.getcwd()
        return os.path.join(CWD, '..', 'engine', 'src', '{}.cpp'.format(src_name))

    def load_cpp_file(self, src_name):
        PATH = self.src_file_path(src_name)
        with open(PATH, 'r') as f:
            return list(f.readlines())

    def create_dict_with_header(self, ret_dict, count_dict):
        unified_keys = list(set(ret_dict.keys() + count_dict.keys()))
        for key in unified_keys:
            if key in ret_dict and key in count_dict:
                yield dict(className=key, classType=ret_dict[key], classMethodCount=count_dict[key])
            elif key in ret_dict:
                yield dict(className=key, classType=ret_dict[key], classMethodCount='derived class/function')
            elif key in count_dict:
                yield dict(className=key, classType='base class', classMethodCount=count_dict[key])


    def line_is_exec_signature(self, line):
        return 'MC' in line and 'Exec' in line and 'ctxt' in line

    def parse_exec_file(self, exec_file):
        lines = self.load_cpp_file(
            exec_file
        )

        ret_dict = {}
        current_methods = []
        current_context = []
        scanning = []
        exec_method = False

        for line in lines:
            split_line = line.split()
            if not split_line:
                continue
            if split_line[0] == 'void':
                if '::' not in line:
                    continue
                signature = split_line[1].split('::')
                method_name = signature[1].split('(')[0]
                if method_name == 'exec_ctxt':
                    current_context.append(signature[0])
                    exec_method = True
            elif split_line[0] == '{' and exec_method:
                scanning.append('{')
            elif split_line[0] == '}' and exec_method:
                scanning.pop()
                if not len(scanning):
                    ret_dict[current_context[-1]] = len(set(current_methods))
                    current_methods = []
                    exec_method = False
            elif self.line_is_exec_signature(line):
                current_methods.append(split_line[0].split('(')[0])

        return ret_dict

    def parse_multiple_exec_files(self, exec_files):
        ret = {}
        for exec_file in exec_files:
            ret.update(
                self.parse_exec_file(
                    exec_file
                )
            )
        return ret

    def get_file(self):
        return self.syntax_file

def write_csv(ret_dicts):
    homedir = expanduser('~')
    PATH = os.path.join(homedir, 'livecode_syntax_classes.csv')
    with open(PATH, 'w+') as f:
        fieldnames = ['className', 'classType', 'classMethodCount']
        writer = csv.DictWriter(f, fieldnames)
        writer.writeheader()

        for dct in ret_dicts:
            writer.writerow(dct)



    
if __name__ == "__main__":
    reader = SyntaxClassReader('newobj', EXEC_FILES)
    file = reader.syntax_file
    write_csv(file)
            
