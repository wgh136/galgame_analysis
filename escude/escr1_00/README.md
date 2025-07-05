# ESCR1_00 脚本

适用于对`script.bin`文件解包后的bin文件,
且文件以`45 53 43 52 31 5F 30 30`(ESCR1_00)开头

## 文件构成

```
struct ESCRFile {
    char magic[8];           // "ESCR1_00"
    uint32_t str_count;      // 字符串数量
    
    // 索引表（字符串起始位置相对文本段的偏移）
    uint32_t text_offsets[]; // 变长数组, 长度等于str_count*4
    
    uint32_t script_size;    // 脚本字节码占用的字节数量
    
    // 脚本字节码
    uint8_t script_data[];   // 游戏逻辑指令
    
    uint32_t text_segament_size; // 文本数据段占用的字节数量
    
    // 文本数据段
    char texts[];         // SJIS编码文本，以\0结尾
};
```

已确认第一个字符串为空字符串, 提取文本时需要忽略, 重新构建文本段时需要添加一个空字符串
