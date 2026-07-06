import sys
import os

def replace_in_file(file_path, search, replace):
    """对单个文件进行字符串替换，返回是否实际修改"""
    try:
        # 尝试用 utf-8 读取
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except (UnicodeDecodeError, PermissionError, OSError) as e:
        print(f"  跳过 (无法读取): {file_path} - {e}")
        return False

    if search not in content:
        return False

    new_content = content.replace(search, replace)
    try:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        return True
    except (PermissionError, OSError) as e:
        print(f"  跳过 (无法写入): {file_path} - {e}")
        return False

def main():
    if len(sys.argv) != 4:
        print("用法: python replace_in_file.py <目标文件> <查找字符串> <替换字符串>")
        sys.exit(1)

    target_file = sys.argv[1]
    search_str = sys.argv[2]
    replace_str = sys.argv[3]

    if not os.path.isfile(target_file):
        print(f"错误: 文件不存在 - {target_file}")
        sys.exit(1)

    print(f"目标文件: {target_file}")
    print(f"将 '{search_str}' 替换为 '{replace_str}'")

    if replace_in_file(target_file, search_str, replace_str):
        print("替换完成。")
    else:
        print("未找到匹配字符串，或处理出错。")

if __name__ == "__main__":
    main()