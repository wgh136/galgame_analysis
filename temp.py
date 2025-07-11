import os
import json
import argparse


def add_missing_line_fields(original_dir, translated_dir):
    # 确保原始目录和翻译目录存在
    if not os.path.exists(original_dir):
        print(f"原始目录不存在: {original_dir}")
        return
    if not os.path.exists(translated_dir):
        print(f"翻译目录不存在: {translated_dir}")
        return

    # 遍历原始目录中的所有 JSON 文件
    for filename in os.listdir(original_dir):
        if filename.endswith(".json"):
            original_file_path = os.path.join(original_dir, filename)
            translated_file_path = os.path.join(translated_dir, filename)

            # 检查翻译后的文件是否存在
            if not os.path.exists(translated_file_path):
                print(f"翻译后的文件不存在，跳过: {translated_file_path}")
                continue

            # 读取原始文件和翻译后的文件
            with open(original_file_path, "r", encoding="utf-8") as original_file:
                original_data = json.load(original_file)

            with open(translated_file_path, "r", encoding="utf-8") as translated_file:
                translated_data = json.load(translated_file)

            # 检查并补充缺失的 line 字段
            for original_item, translated_item in zip(original_data, translated_data):
                if "line" in original_item and "line" not in translated_item:
                    translated_item["line"] = original_item["line"]

            # 将补充后的数据写回翻译后的文件
            with open(translated_file_path, "w", encoding="utf-8") as translated_file:
                json.dump(translated_data, translated_file, ensure_ascii=False, indent=4)

            print(f"已更新文件: {translated_file_path}")


if __name__ == "__main__":
    # 解析命令行参数
    parser = argparse.ArgumentParser(description="补充翻译后 JSON 文件中缺失的 line 字段")
    parser.add_argument("original_dir", help="原始 JSON 文件所在目录")
    parser.add_argument("translated_dir", help="翻译后 JSON 文件所在目录")
    args = parser.parse_args()

    # 调用函数处理文件
    add_missing_line_fields(args.original_dir, args.translated_dir)