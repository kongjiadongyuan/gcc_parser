#!/usr/bin/env python3
import CppHeaderParser
import os
import sys

def parse_header(path):
    optionHeader = CppHeaderParser.CppHeader(path)
    opt_code_list = None
    for enum in optionHeader.enums:
        if enum['name'] == 'opt_code':
            opt_code_list = enum['values']
            break
    if opt_code_list is None:
        raise Exception("opt_code not found")
    return opt_code_list

def generate_rev_list(opt_code_list):
    max_code = 0
    for opt in opt_code_list:
        if opt['value'] > max_code:
            max_code = opt['value']
    rev_list = ['' for _ in range(max_code + 1)]
    for opt in opt_code_list:
        rev_list[opt['value']] = opt['name']
    return rev_list

def generate_rev_header(opt_code_list, path):
    res = ""
    res += "#ifndef REV_OPTIONS_H\n"
    res += "#define REV_OPTIONS_H\n"
    res += "\n" * 2
    res += "char const *rev_opt_code[] = {"
    for idx, opt_name in enumerate(opt_code_list):
        res += f"\"{opt_name}\""
        if idx < len(opt_code_list) - 1:
            res += ", "
    res += "};"
    res += "\n" * 2
    res += "#endif\n"
    with open(path, 'w') as f:
        f.write(res)


def main():
    opt_code_list = parse_header(sys.argv[1])
    rev_list = generate_rev_list(opt_code_list)
    generate_rev_header(rev_list, sys.argv[2])

if __name__ == '__main__':
    main()
