#!/usr/bin/env python3

import sys
import shutil
import os
import re

IGNORE_FILES = ('test', '.*.txt', 'kernel_dummies.h', '.*.py', '.git', 'cmake-build-debug', 'main.c', '.idea')

class SrcFile:
    def __init__(self, file_name, content):
        self.file_name = file_name
        self.content = content

def main(src, dest):
    target_files = get_files_in_folder(src)
    print('### removing std-headers ###')
    reg_ex = re.compile(".*<.*.h>.*")
    iterate_lines(target_files, lambda line: "" if reg_ex.match(line) else line)
    print('### remove kernel_dummies header ### ')
    reg_ex = re.compile(".*kernel_dummies.*")
    iterate_lines(target_files, lambda line: "" if reg_ex.match(line) else line)
    print('### printf -> printk ###')
    iterate_lines(target_files, lambda line: line.replace('printf', 'printk'), print_log=False)
    write_to_file(target_files, dest)

def iterate_lines(all_files, func, print_log=True):
    """
    Iterates through all lines in the target folder any applies function
    """
    assert(type(all_files) is list)
    assert(callable(func))
    new_content = []
    for f in all_files:
        for line in f.content:
            line_in = line
            line = func(line)
            new_content.append(line)
            if line != line_in and print_log:
                print(line_in[:-1] + " => " + line)
        f.content = new_content
        new_content = []


def get_files_in_folder(src) -> [SrcFile]:
    target_files = []
    reg_ex_list = list(map(lambda reg_str: re.compile(reg_str), IGNORE_FILES))
    files = os.listdir(src)
    for file in files:
        if ignore_file(file, reg_ex_list):
            continue
        else:
            with open(file, 'r') as f:
                print(f'found file {file}')
                target_files.append(SrcFile(file, f.readlines() ))
    return target_files



def write_to_file(files, dest):
    for file in files:
        with open(os.path.join(dest, file.file_name), 'w') as f:
            for line in file.content:
                f.write(line)
            print(f"wrote {file.file_name}")

def ignore_file(file, reg_ex_list) -> bool:
    for reg_ex in reg_ex_list:
        if reg_ex.match(file):
            return True
    return False



if __name__ == "__main__":
    print(f'level2 folder is: {sys.argv[1]}')
    print(f'target folder is: {sys.argv[2]}\n ------------')
    main(sys.argv[1], sys.argv[2])
