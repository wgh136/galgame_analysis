#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>
#include <cstdint>
#include <filesystem>
#include <algorithm>

// 检查文件头是否符合escude标识
bool isEscudeScript(const std::vector<uint8_t>& data) {
    const uint8_t signature[] = {0x40, 0x65, 0x73, 0x63, 0x75, 0x3A, 0x64, 0x65}; // @escu:de
    if (data.size() < 8) return false;
    return std::memcmp(data.data(), signature, 8) == 0;
}

// 从脚本文件中提取文本并保存到txt文件
void extractText(const std::string& inputFile, const std::string& outputFile) {
    // 读取输入文件
    std::ifstream input(inputFile, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Cannot open input file: " + inputFile);
    }
    
    // 读取整个文件到内存
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    input.close();
    
    // 验证文件头
    if (!isEscudeScript(data)) {
        throw std::runtime_error("Invalid escude script file");
    }
    
    // 获取控制部分长度
    uint32_t controlLength = *reinterpret_cast<uint32_t*>(&data[0x08]);
    
    // 检查是否为两个文本段
    uint32_t hasTwoSegments = *reinterpret_cast<uint32_t*>(&data[0x0C]);

    if (!hasTwoSegments) {
        return;
    }
    
    // 获取第一个文本段数据区域长度（如果有两个文本段）
    uint32_t firstSegmentDataLength = *reinterpret_cast<uint32_t*>(&data[0x10]);
    
    // 获取最后一个文本段的字符串数量
    uint32_t lastSegmentStringCount = *reinterpret_cast<uint32_t*>(&data[0x14]);
    
    // 获取最后一个文本段数据区域长度
    uint32_t lastSegmentDataLength = *reinterpret_cast<uint32_t*>(&data[0x18]);
    
    // 计算文本部分的起始偏移
    uint32_t textSectionOffset = 0x1C + controlLength;
    
    // 打开输出文件
    std::ofstream output(outputFile);
    if (!output) {
        throw std::runtime_error("Cannot open output file: " + outputFile);
    }
    
    if (hasTwoSegments == 1) {
        // 有两个文本段
        // 计算第二个文本段的索引表和数据区域长度
        uint32_t secondSegmentIndexLength = lastSegmentStringCount * 4;
        uint32_t secondSegmentTotalLength = secondSegmentIndexLength + lastSegmentDataLength;
        
        // 第一个文本段索引表结束位置
        uint32_t firstIndexTableEnd = data.size() - secondSegmentTotalLength - firstSegmentDataLength;
        
        // 处理第一个文本段
        std::vector<uint32_t> firstOffsets;
        for (uint32_t i = textSectionOffset; i < firstIndexTableEnd; i += 4) {
            if (i + 4 > data.size()) break;
            uint32_t offset = *reinterpret_cast<uint32_t*>(&data[i]);
            firstOffsets.push_back(offset);
        }
        
        // 第一个文本段数据区域起始位置
        uint32_t firstDataStart = firstIndexTableEnd;
        
        // 提取第一个文本段的文本
        for (size_t i = 0; i < firstOffsets.size(); ++i) {
            uint32_t currentOffset = firstOffsets[i];
            uint32_t nextOffset = (i < firstOffsets.size() - 1) ? firstOffsets[i + 1] : firstSegmentDataLength;
            
            if (currentOffset >= firstSegmentDataLength) continue;
            
            uint32_t absoluteOffset = firstDataStart + currentOffset;
            std::string text;
            
            for (uint32_t j = absoluteOffset; j < absoluteOffset + (nextOffset - currentOffset); ++j) {
                if (j >= data.size()) break;
                if (data[j] == 0) break;
                text.push_back(static_cast<char>(data[j]));
            }
            
            if (!text.empty()) {
                output << text << std::endl;
            }
        }
        
        // 处理第二个文本段
        uint32_t secondIndexTableStart = data.size() - secondSegmentTotalLength;
        uint32_t secondDataStart = secondIndexTableStart + secondSegmentIndexLength;
        
        std::vector<uint32_t> secondOffsets;
        for (uint32_t i = secondIndexTableStart; i < secondDataStart; i += 4) {
            if (i + 4 > data.size()) break;
            uint32_t offset = *reinterpret_cast<uint32_t*>(&data[i]);
            secondOffsets.push_back(offset);
        }
        
        // 提取第二个文本段的文本
        for (size_t i = 0; i < secondOffsets.size(); ++i) {
            uint32_t currentOffset = secondOffsets[i];
            uint32_t nextOffset = (i < secondOffsets.size() - 1) ? secondOffsets[i + 1] : lastSegmentDataLength;
            
            if (currentOffset >= lastSegmentDataLength) continue;
            
            uint32_t absoluteOffset = secondDataStart + currentOffset;
            std::string text;
            
            for (uint32_t j = absoluteOffset; j < absoluteOffset + (nextOffset - currentOffset); ++j) {
                if (j >= data.size()) break;
                if (data[j] == 0) break;
                text.push_back(static_cast<char>(data[j]));
            }
            
            if (!text.empty()) {
                output << text << std::endl;
            }
        }
    } else {
        // 只有一个文本段
        uint32_t textDataStart = data.size() - lastSegmentDataLength;
        
        // 获取文本索引表
        std::vector<uint32_t> textOffsets;
        for (uint32_t i = textSectionOffset; i < textDataStart; i += 4) {
            if (i + 4 > data.size()) break;
            uint32_t offset = *reinterpret_cast<uint32_t*>(&data[i]);
            textOffsets.push_back(offset);
        }

        // 提取并输出文本
        for (size_t i = 0; i < textOffsets.size(); ++i) {
            uint32_t currentOffset = textOffsets[i];
            uint32_t nextOffset = (i < textOffsets.size() - 1) ? textOffsets[i + 1] : lastSegmentDataLength;
            
            if (currentOffset >= lastSegmentDataLength) continue;
            
            uint32_t absoluteOffset = textDataStart + currentOffset;
            std::string text;
            
            for (uint32_t j = absoluteOffset; j < absoluteOffset + (nextOffset - currentOffset); ++j) {
                if (j >= data.size()) break;
                if (data[j] == 0) break;
                text.push_back(static_cast<char>(data[j]));
            }
            
            if (!text.empty()) {
                output << text << std::endl;
            }
        }
    }
    
    std::cout << "Text successfully extracted to: " << outputFile << std::endl;
}

// 从txt文件读取文本并修改脚本文件
void modifyText(const std::string& scriptFile, const std::string& txtFile) {
    // 读取脚本文件
    std::ifstream scriptInput(scriptFile, std::ios::binary);
    if (!scriptInput) {
        throw std::runtime_error("Cannot open script file: " + scriptFile);
    }
    
    // 读取整个脚本文件到内存
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(scriptInput)), std::istreambuf_iterator<char>());
    scriptInput.close();
    
    // 验证文件头
    if (!isEscudeScript(data)) {
        throw std::runtime_error("Invalid escude script file");
    }
    
    // 读取txt文件的所有文本行
    std::ifstream txtInput(txtFile);
    if (!txtInput) {
        throw std::runtime_error("Cannot open text file: " + txtFile);
    }
    
    std::vector<std::string> newTexts;
    std::string line;
    while (std::getline(txtInput, line)) {
        newTexts.push_back(line);
    }
    txtInput.close();
    
    // 获取控制部分长度
    uint32_t controlLength = *reinterpret_cast<uint32_t*>(&data[0x08]);
    
    // 检查是否为两个文本段
    uint32_t hasTwoSegments = *reinterpret_cast<uint32_t*>(&data[0x0C]);
    
    // 获取第一个文本段数据区域长度（如果有两个文本段）
    uint32_t firstSegmentDataLength = *reinterpret_cast<uint32_t*>(&data[0x10]);
    
    // 获取最后一个文本段的字符串数量
    uint32_t lastSegmentStringCount = *reinterpret_cast<uint32_t*>(&data[0x14]);
    
    // 获取最后一个文本段数据区域长度
    uint32_t lastSegmentDataLength = *reinterpret_cast<uint32_t*>(&data[0x18]);
    
    // 计算文本索引表的起始偏移
    uint32_t indexTableOffset = 0x1C + controlLength;
    
    std::vector<uint32_t> firstOffsets, secondOffsets;
    
    if (hasTwoSegments == 1) {
        // 有两个文本段
        uint32_t secondSegmentIndexLength = lastSegmentStringCount * 4;
        uint32_t secondSegmentTotalLength = secondSegmentIndexLength + lastSegmentDataLength;
        uint32_t firstIndexTableEnd = data.size() - secondSegmentTotalLength - firstSegmentDataLength;
        
        // 获取第一个文本段索引表
        for (uint32_t i = indexTableOffset; i < firstIndexTableEnd; i += 4) {
            if (i + 4 > data.size()) break;
            uint32_t offset = *reinterpret_cast<uint32_t*>(&data[i]);
            firstOffsets.push_back(offset);
        }
        
        // 获取第二个文本段索引表
        uint32_t secondIndexTableStart = data.size() - secondSegmentTotalLength;
        uint32_t secondDataStart = secondIndexTableStart + secondSegmentIndexLength;
        for (uint32_t i = secondIndexTableStart; i < secondDataStart; i += 4) {
            if (i + 4 > data.size()) break;
            uint32_t offset = *reinterpret_cast<uint32_t*>(&data[i]);
            secondOffsets.push_back(offset);
        }
    } else {
        // 只有一个文本段
        uint32_t textDataStart = data.size() - lastSegmentDataLength;
        for (uint32_t i = indexTableOffset; i < textDataStart; i += 4) {
            if (i + 4 > data.size()) break;
            uint32_t offset = *reinterpret_cast<uint32_t*>(&data[i]);
            firstOffsets.push_back(offset);
        }
    }
    
    // 检查文本行数是否与索引表匹配
    uint32_t totalStringCount = firstOffsets.size() + secondOffsets.size();
    if (newTexts.size() != totalStringCount) {
        std::cout << "New Texts Size: " << newTexts.size() << std::endl;
        std::cout << "Total String Count: " << totalStringCount << std::endl;
        throw std::runtime_error("Mismatch between number of text lines and index table entries");
    }
    
    std::vector<uint8_t> newData;
    
    // 保留原始文件头和控制部分
    newData.insert(newData.end(), data.begin(), data.begin() + indexTableOffset);
    
    if (hasTwoSegments == 1) {
        // 构建第一个文本段
        std::vector<std::string> firstTexts(newTexts.begin(), newTexts.begin() + firstOffsets.size());
        std::vector<std::string> secondTexts(newTexts.begin() + firstOffsets.size(), newTexts.end());
        
        // 构建第一个文本段的数据
        std::vector<uint8_t> firstTextData;
        std::vector<uint32_t> newFirstOffsets;
        uint32_t currentOffset = 0;
        
        for (const std::string& text : firstTexts) {
            newFirstOffsets.push_back(currentOffset);
            for (char c : text) {
                firstTextData.push_back(static_cast<uint8_t>(c));
            }
            firstTextData.push_back(0);
            currentOffset += text.length() + 1;
        }
        
        // 构建第一个文本段的索引表
        for (uint32_t offset : newFirstOffsets) {
            for (size_t i = 0; i < 4; ++i) {
                newData.push_back((offset >> (i * 8)) & 0xFF);
            }
        }
        
        // 插入第一个文本段数据
        newData.insert(newData.end(), firstTextData.begin(), firstTextData.end());
        
        // 构建第二个文本段的数据
        std::vector<uint8_t> secondTextData;
        std::vector<uint32_t> newSecondOffsets;
        currentOffset = 0;
        
        for (const std::string& text : secondTexts) {
            newSecondOffsets.push_back(currentOffset);
            for (char c : text) {
                secondTextData.push_back(static_cast<uint8_t>(c));
            }
            secondTextData.push_back(0);
            currentOffset += text.length() + 1;
        }
        
        // 构建第二个文本段的索引表
        for (uint32_t offset : newSecondOffsets) {
            for (size_t i = 0; i < 4; ++i) {
                newData.push_back((offset >> (i * 8)) & 0xFF);
            }
        }
        
        // 插入第二个文本段数据
        newData.insert(newData.end(), secondTextData.begin(), secondTextData.end());
        
        // 更新文件头信息
        uint32_t newFirstSegmentDataLength = firstTextData.size();
        uint32_t newLastSegmentDataLength = secondTextData.size();
        
        for (size_t i = 0; i < 4; ++i) {
            newData[0x10 + i] = (newFirstSegmentDataLength >> (i * 8)) & 0xFF;
            newData[0x18 + i] = (newLastSegmentDataLength >> (i * 8)) & 0xFF;
        }
    } else {
        // 只有一个文本段
        std::vector<uint8_t> newTextData;
        std::vector<uint32_t> newOffsets;
        uint32_t currentOffset = 0;
        
        for (const std::string& text : newTexts) {
            newOffsets.push_back(currentOffset);
            for (char c : text) {
                newTextData.push_back(static_cast<uint8_t>(c));
            }
            newTextData.push_back(0);
            currentOffset += text.length() + 1;
        }
        
        // 构建新的索引表
        for (uint32_t offset : newOffsets) {
            for (size_t i = 0; i < 4; ++i) {
                newData.push_back((offset >> (i * 8)) & 0xFF);
            }
        }
        
        // 插入新文本段
        newData.insert(newData.end(), newTextData.begin(), newTextData.end());
        
        // 更新文本长度字段
        uint32_t newTextLength = newTextData.size();
        for (size_t i = 0; i < 4; ++i) {
            newData[0x18 + i] = (newTextLength >> (i * 8)) & 0xFF;
        }
    }
    
    // 将修改后的数据写回文件
    std::ofstream output(scriptFile, std::ios::binary);
    if (!output) {
        throw std::runtime_error("Cannot write script file: " + scriptFile);
    }
    
    output.write(reinterpret_cast<const char*>(newData.data()), newData.size());
    output.close();
    
    std::cout << "Script file successfully modified: " << scriptFile << std::endl;
}

// 批量提取目录中的所有bin文件文本
void batchExtractText(const std::string& inputDir, const std::string& outputDir) {
    namespace fs = std::filesystem;
    
    // 确保输出目录存在
    fs::create_directories(outputDir);
    
    int processedCount = 0;
    int errorCount = 0;
    
    // 遍历输入目录中的所有.bin文件
    for (const auto& entry : fs::directory_iterator(inputDir)) {
        if (entry.path().extension() == ".bin") {
            std::string inputPath = entry.path().string();
            std::string filename = entry.path().stem().string();
            std::string outputPath = absolute((fs::path(outputDir) / (filename + ".txt"))).string();
            
            try {
                std::cout << "Processing: " << inputPath << " -> " << outputPath << std::endl;
                extractText(inputPath, outputPath);
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
    namespace fs = std::filesystem;
    
    // 确保输出目录存在
    fs::create_directories(outputDir);
    
    int processedCount = 0;
    int errorCount = 0;
    
    // 遍历输入目录中的所有.txt文件
    for (const auto& entry : fs::directory_iterator(inputDir)) {
        if (entry.path().extension() == ".txt") {
            std::string inputPath = entry.path().string();
            std::string filename = entry.path().stem().string();
            std::string outputPath = absolute((fs::path(outputDir) / (filename + ".bin"))).string();
            
            // 检查输出文件是否存在
            if (!fs::exists(outputPath)) {
                std::cerr << "Skip: Cannot find corresponding bin file: " << outputPath << std::endl;
                errorCount++;
                continue;
            }
            
            try {
                std::cout << "Processing: " << inputPath << " -> " << outputPath << std::endl;
                modifyText(outputPath, inputPath);
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
