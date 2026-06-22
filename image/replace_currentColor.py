#!/usr/bin/env python3
import os
import sys

#这个文件用来把当前目录的svg内部的currentColor替换为#000000,
# currentColor是从lucide网站上面下载下来的svg字段格式，这个字段在juce中显示为透明，所以需要替换

def replace_in_file(filepath, old_text, new_text):
    """读取文件内容并替换文本，写回原文件"""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
        
        if old_text not in content:
            return False  # 无需替换
        
        new_content = content.replace(old_text, new_text)
        
        # 写入备份（可选）
        # with open(filepath + '.bak', 'w', encoding='utf-8') as f:
        #     f.write(content)
        
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(new_content)
        return True
    except Exception as e:
        print(f"处理 {filepath} 时出错: {e}")
        return False

def main():
    # 默认处理 image 文件夹，如果不存在则处理当前目录
    target_dir = "."
    
    svg_files = []
    for root, dirs, files in os.walk(target_dir):
        for file in files:
            if file.lower().endswith(".svg"):
                svg_files.append(os.path.join(root, file))
    
    if not svg_files:
        print("未找到任何 .svg 文件。")
        return
    
    print(f"找到 {len(svg_files)} 个 SVG 文件，正在替换...")
    count = 0
    for path in svg_files:
        if replace_in_file(path, "currentColor", "#000000"):
            count += 1
            print(f"已处理: {path}")
    
    print(f"完成！共修改了 {count} 个文件。")

if __name__ == "__main__":
    main()