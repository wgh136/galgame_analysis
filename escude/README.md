# escu:de

对escu:de会社的游戏引擎研究

## 游戏文件结构

- exe文件
- configure.cfg
- bin文件

有一个bin文件名称与exe文件相同, 解包后可以发现是游戏的主程序代码

## 解包bin文件

使用GARBro即可

## 封包bin文件

使用[Visual-novel-archive-tools](https://github.com/Cosetto/Visual-novel-archive-tools)

## script.bin

此文件为游戏脚本的打包, 解包后可以发现有许多bin文件或者001文件, 若有001文件则为加密. 存在多种编码方式, 需要根据文件头识别.

## data.bin

此文件存储了人名, 场景名等内容, 由于内容不多, 直接替换字符串, 确保长度一致即可

## 修改主程序代码

似乎exe文件是一个c代码解释器, 主程序在与exe文件同名的bin文件中

解包该文件, 做出如下修改:

修改`misc/text.c`中的`init_default_font`函数, 使得程序调用`CreateFontIndirectA`:
```c
void init_default_font(int font_id, int weight)
{
	default_font_id = FT_USER;
	default_font_weight = 400;
	strcpy(user_font_name,"SimHei");
	
	//ini_gets("Font", "Face", "", user_font_name, sizeof(user_font_name), NULL);
	//if(user_font_name[0] != '\0'){
	//	default_font_id = FT_USER;
	//	default_font_weight = 400;
	//	if(ini_geti("Font", "Bold", 0, NULL)){
	//		default_font_weight = 700;
	//	}
	//}

}
```

修改`lib/string.h`, 修改边界校验:
```c
//#define ISKANJI(x)			((((x)^0x20)-0xa1) <= 0x3b)

#define ISKANJI(x)			((x) >= 0x81)
```

## 汉化步骤

1. 解包script.bin, 对其中文件再解包获得游戏文本
2. 翻译游戏文本并重新封包, 使用GBK编码
3. 修改data.bin, 翻译人名
4. 修改启动脚本
5. 修改exe程序, 找到`CreateFontIndirectA`的调用, 将参数0x80修改为0x86