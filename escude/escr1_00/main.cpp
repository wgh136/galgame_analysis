#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>
#include <cstdint>
#include <algorithm>
#include <locale>
#include <codecvt>

namespace fs = std::filesystem;

const std::string magic = "ESCR1_00";

// 读取小端序 uint32_t
uint32_t readLittleEndian32(const uint8_t* data) {
    return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

// 写入小端序 uint32_t
void writeLittleEndian32(uint8_t* data, uint32_t value) {
    data[0] = value & 0xFF;
    data[1] = (value >> 8) & 0xFF;
    data[2] = (value >> 16) & 0xFF;
    data[3] = (value >> 24) & 0xFF;
}

// 检查文件头是否符合ESCR1_00标识
bool isESCR1_00(const std::vector<uint8_t>& data) {
    if (data.size() < 8) return false;
    return std::memcmp(data.data(), magic.c_str(), 8) == 0;
}

// 从脚本文件中提取文本并保存到txt文件
void extractText(const std::string& inputFile, const std::string& outputFile) {
    // 读取输入文件
    std::ifstream file(inputFile, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open input file: " + inputFile);
    }

    // 读取整个文件到内存
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    file.close();

    // 验证文件头
    if (!isESCR1_00(data)) {
        throw std::runtime_error("Invalid ESCR1_00 file format");
    }

    // 解析文件结构
    if (data.size() < 12) {
        throw std::runtime_error("File too small");
    }

    uint32_t str_count = readLittleEndian32(data.data() + 8);

    // 检查索引表大小
    size_t index_table_size = str_count * 4;
    if (data.size() < 12 + index_table_size + 4) {
        throw std::runtime_error("Invalid file structure");
    }

    // 读取索引表
    std::vector<uint32_t> text_offsets(str_count);
    for (uint32_t i = 0; i < str_count; i++) {
        text_offsets[i] = readLittleEndian32(data.data() + 12 + i * 4);
    }

    // 读取脚本大小
    uint32_t script_size = readLittleEndian32(data.data() + 12 + index_table_size);

    // 计算文本段起始位置
    size_t text_segment_pos = 12 + index_table_size + 4 + script_size + 4;
    if (data.size() < text_segment_pos) {
        throw std::runtime_error("Invalid file structure");
    }

    uint32_t text_segment_size = readLittleEndian32(data.data() + text_segment_pos - 4);

    // 提取文本
    std::ofstream outFile(outputFile, std::ios::binary);
    if (!outFile) {
        throw std::runtime_error("Cannot create output file: " + outputFile);
    }

    // 跳过第一个空字符串（索引0）
    for (uint32_t i = 1; i < str_count; i++) {
        uint32_t offset = text_offsets[i];
        if (offset >= text_segment_size) {
            continue;
        }

        // 找到字符串结束位置
        size_t text_pos = text_segment_pos + offset;
        size_t text_end = text_pos;
        while (text_end < data.size() && data[text_end] != 0) {
            text_end++;
        }

        if (text_end > text_pos) {
            // 写入文本行
            outFile.write(reinterpret_cast<const char*>(data.data() + text_pos), text_end - text_pos);
            outFile.write("\r\n", 2);
        }
    }

    outFile.close();
}

// 从txt文件读取文本并修改脚本文件
void modifyText(const std::string& scriptFile, const std::string& txtFile) {
    // 读取原始脚本文件
    std::ifstream file(scriptFile, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open script file: " + scriptFile);
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    file.close();

    // 验证文件头
    if (!isESCR1_00(data)) {
        throw std::runtime_error("Invalid ESCR1_00 file format");
    }

    // 解析原始文件结构
    uint32_t str_count = readLittleEndian32(data.data() + 8);
    size_t index_table_size = str_count * 4;
    uint32_t script_size = readLittleEndian32(data.data() + 12 + index_table_size);

    // 读取新文本
    std::ifstream txtFileStream(txtFile, std::ios::binary);
    if (!txtFileStream) {
        throw std::runtime_error("Cannot open text file: " + txtFile);
    }

    std::vector<std::string> newTexts;
    std::string line;
    while (std::getline(txtFileStream, line)) {
        // 移除行尾的 \r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        newTexts.push_back(line);
    }
    txtFileStream.close();

    // 检查文本数量是否匹配（减去第一个空字符串）
    if (newTexts.size() != str_count - 1) {
        throw std::runtime_error("Text count mismatch. Expected: " + std::to_string(str_count - 1) +
                                ", Got: " + std::to_string(newTexts.size()));
    }

    // 构建新的文本段
    std::vector<uint8_t> newTextSegment;
    std::vector<uint32_t> newTextOffsets(str_count);

    // 第一个字符串是空字符串
    newTextOffsets[0] = 0;
    newTextSegment.push_back(0);

    // 添加其他文本
    for (size_t i = 0; i < newTexts.size(); i++) {
        newTextOffsets[i + 1] = newTextSegment.size();
        for (char c : newTexts[i]) {
            newTextSegment.push_back(static_cast<uint8_t>(c));
        }
        newTextSegment.push_back(0); // 字符串结束符
    }

    // 构建新文件
    std::vector<uint8_t> newFile;

    // 写入文件头
    newFile.insert(newFile.end(), magic.begin(), magic.end());

    // 写入字符串数量
    uint8_t strCountBytes[4];
    writeLittleEndian32(strCountBytes, str_count);
    newFile.insert(newFile.end(), strCountBytes, strCountBytes + 4);

    // 写入索引表
    for (uint32_t offset : newTextOffsets) {
        uint8_t offsetBytes[4];
        writeLittleEndian32(offsetBytes, offset);
        newFile.insert(newFile.end(), offsetBytes, offsetBytes + 4);
    }

    // 写入脚本大小
    uint8_t scriptSizeBytes[4];
    writeLittleEndian32(scriptSizeBytes, script_size);
    newFile.insert(newFile.end(), scriptSizeBytes, scriptSizeBytes + 4);

    // 写入脚本数据
    size_t scriptDataPos = 12 + index_table_size + 4;
    newFile.insert(newFile.end(), data.begin() + scriptDataPos, data.begin() + scriptDataPos + script_size);

    // 写入文本段大小
    uint8_t textSegmentSizeBytes[4];
    writeLittleEndian32(textSegmentSizeBytes, newTextSegment.size());
    newFile.insert(newFile.end(), textSegmentSizeBytes, textSegmentSizeBytes + 4);

    // 写入文本段
    newFile.insert(newFile.end(), newTextSegment.begin(), newTextSegment.end());

    // 写入修改后的文件
    std::ofstream outFile(scriptFile, std::ios::binary);
    if (!outFile) {
        throw std::runtime_error("Cannot write to script file: " + scriptFile);
    }

    outFile.write(reinterpret_cast<const char*>(newFile.data()), newFile.size());
    outFile.close();
}

// 批量提取目录中的所有bin文件文本
void batchExtractText(const std::string& inputDir, const std::string& outputDir) {
    // 确保输出目录存在
    fs::create_directories(outputDir);

    int processedCount = 0;
    int errorCount = 0;

    // 递归遍历输入目录中的所有.bin文件
    for (const auto& entry : fs::recursive_directory_iterator(inputDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".bin") {
            std::string inputPath = entry.path().string();

            // 计算相对路径
            fs::path relativePath = fs::relative(entry.path(), inputDir);
            fs::path outputPath = fs::path(outputDir) / relativePath.parent_path() / (relativePath.stem().string() + ".txt");

            // 确保输出文件的目录存在
            fs::create_directories(outputPath.parent_path());

            try {
                std::cout << "Processing: " << inputPath << " -> " << outputPath.string() << std::endl;
                extractText(inputPath, outputPath.string());
                processedCount++;
            } catch (const std::exception& e) {
                std::cerr << "Error processing " << inputPath << ": " << e.what() << std::endl;
                errorCount++;
            }
        }
    }

    std::cout << "Batch extraction completed. " << processedCount << " files processed, "
              << errorCount << " errors." << std::endl;
}

// 批量修改目录中的所有文本文件对应的bin文件
void batchModifyText(const std::string& inputDir, const std::string& outputDir) {
    // 确保输出目录存在
    fs::create_directories(outputDir);

    int processedCount = 0;
    int errorCount = 0;

    // 递归遍历输入目录中的所有.txt文件
    for (const auto& entry : fs::recursive_directory_iterator(inputDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            std::string inputPath = entry.path().string();

            // 计算相对路径
            fs::path relativePath = fs::relative(entry.path(), inputDir);
            fs::path outputPath = fs::path(outputDir) / relativePath.parent_path() / (relativePath.stem().string() + ".bin");

            // 确保输出文件的目录存在
            fs::create_directories(outputPath.parent_path());

            // 检查输出文件是否存在
            if (!fs::exists(outputPath)) {
                std::cerr << "Skip: Cannot find corresponding bin file: " << outputPath.string() << std::endl;
                errorCount++;
                continue;
            }

            try {
                std::cout << "Processing: " << inputPath << " -> " << outputPath.string() << std::endl;
                modifyText(outputPath.string(), inputPath);
                processedCount++;
            } catch (const std::exception& e) {
                std::cerr << "Error processing " << inputPath << ": " << e.what() << std::endl;
                errorCount++;
            }
        }
    }

    std::cout << "Batch modification completed. " << processedCount << " files processed, "
              << errorCount << " errors." << std::endl;
}

void printUsage() {
    std::cout << "Usage:" << std::endl;
    std::cout << "  Extract text: program -e <script file> <output text file>" << std::endl;
    std::cout << "  Modify text: program -m <script file> <input text file>" << std::endl;
    std::cout << "  Batch extract: program -be <input directory> <output directory>" << std::endl;
    std::cout << "  Batch modify: program -bm <input directory> <output directory>" << std::endl;
}

int main(int argc, char* argv[]) {
    try {
        if (argc < 4) {
            printUsage();
            return 1;
        }

        std::string mode = argv[1];
        std::string sourcePath = argv[2];
        std::string targetPath = argv[3];

        if (mode == "-e") {
            // 提取模式
            extractText(sourcePath, targetPath);
        } else if (mode == "-m") {
            // 修改模式
            modifyText(sourcePath, targetPath);
        } else if (mode == "-be") {
            // 批量提取模式
            batchExtractText(sourcePath, targetPath);
        } else if (mode == "-bm") {
            // 批量修改模式
            batchModifyText(sourcePath, targetPath);
        } else {
            std::cerr << "Invalid operation mode" << std::endl;
            printUsage();
            return 1;
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}