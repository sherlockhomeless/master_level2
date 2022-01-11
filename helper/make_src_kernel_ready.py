#!/usr/bin/env python3

"""
MUST BE GIVEN 2 PARAMETERS:

1. Parameter is path of level2 sources
2. Parameter is path of $KERNEL_SOURCES/kernel/sched/prediction_failure_handling/
"""

import sys
import shutil
import os
import re

IGNORE_FILES = ('test', '.*.txt', 'kernel_dummies.h', '.*.py', '.git', 'cmake-build-debug', 'main.c', '.idea',
                'parse_plan.c', 'userland_only.*', 'CMakeFiles', 'level2', 'helper', '.*cmake')


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
    print("### removing assert-statements ###")
    reg_ex = re.compile('.*assert(.*);')
    iterate_lines(target_files, lambda line: "" if reg_ex.match(line) else line)
    print('### printf -> printk ###')
    iterate_lines(target_files, lambda line: line.replace('printf', 'printk'), print_log=False)
    print('### add kernel headers ###')
    add_kernel_header(filter(lambda file: True if ".c" in file.file_name else False, target_files), ("#include <linux/kernel.h>", ""))
    write_to_file(target_files, dest)
    print('### generating makefile ###')
    create_makefile(target_files, dest)


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


def add_kernel_header(file_list, headers):
    for header in headers:
        for file in file_list:
            file_content = file.content
            file_content.insert(0,header + '\n')
            file.content = file_content
        print(f'added {header}')


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
                try:
                    target_files.append(SrcFile(file, f.readlines() ))
                except UnicodeDecodeError:
                    print(f'ERROR @ {file}')
                    assert(0)
    return target_files

def create_makefile(files, dest):
    object_list = "obj-y += "
    for f in files:
        if ".c" in f.file_name:
            object_list = f'{object_list} {f.file_name.replace(".c", ".o")} '
    with open(os.path.join(dest, "Makefile"), 'w') as f:
        f.write(object_list)
        print(f'write object_list')

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
