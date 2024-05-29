import re
import sys

def parse_header_with_regex(path):
    try:
        with open(path, 'r') as file:
            content = file.read()

        # 使用正则表达式匹配枚举
        enum_pattern = r'enum\s+opt_code\s*\{([^}]+)\}'
        matches = re.search(enum_pattern, content, re.DOTALL)
        if not matches:
            raise Exception("opt_code enum not found")

        # 分析枚举中的每个值
        enum_content = matches.group(1)
        item_pattern = r'(\w+)\s*=\s*(\d+),?'
        items = re.findall(item_pattern, enum_content)
        opt_code_list = sorted(items, key=lambda x: int(x[1]))

        # 返回名称和对应的整数值
        return [(name, int(value)) for name, value in opt_code_list]
    except FileNotFoundError:
        print("File not found.")
        sys.exit(1)

def generate_rev_header(opt_code_list, path):
    header_content = [
        "#ifndef REV_OPTIONS_H",
        "#define REV_OPTIONS_H",
        "",
        "const char *rev_opt_code[] = {"
    ]

    # 生成数组元素
    for name, value in opt_code_list:
        if value == max(opt_code_list, key=lambda x: x[1])[1]:  # 检查是否为最后一个元素
            header_content.append(f'    "{name}"')
        else:
            header_content.append(f'    "{name}",')

    header_content.extend([
        "};",
        "",
        "#endif"
    ])

    # 写入文件
    with open(path, 'w') as file:
        file.write('\n'.join(header_content))

def main():
    if len(sys.argv) < 3:
        print("Usage: python script.py <input_header.h> <output_header.h>")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    opt_code_list = parse_header_with_regex(input_path)
    generate_rev_header(opt_code_list, output_path)

if __name__ == '__main__':
    main()