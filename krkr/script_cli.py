# Extract text from krkr script files or patch files
import os
import json

def extractFile(path):
    lines = []
    with open(path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    texts = []

    talker = None

    for i in range(len(lines)):
        line = lines[i]
        if line.startswith('@Talk'):
            splits = line.split(' ')
            for j in range(1, len(splits)):
                if splits[j].startswith('name='):
                    talker = splits[j][5:].strip()
                    break
            if talker is None:
                talker = 'Unknown'
        elif line.startswith("@Hitret"):
            talker = None
        elif talker is not None:
            texts.append({
                "name": talker,
                "message": line,
                "line": i
            })

    return texts

def extractFolder(path, out):
    nameTable = {}
    for file in os.listdir(path):
        if file.endswith('.ks'):
            texts = extractFile(os.path.join(path, file))
            for entry in texts:
                if entry['name'] not in nameTable:
                    nameTable[entry['name']] = len(nameTable) + 1
                entry['name'] = nameTable[entry['name']]

            outContent = json.dumps(texts, ensure_ascii=False, indent=2)
            with open(os.path.join(out, file + '.json'), 'w', encoding='utf-8') as outFile:
                outFile.write(outContent)
    with open(os.path.join(out, 'name_table.json'), 'w', encoding='utf-8') as nameFile:
        nameTableContent = json.dumps(nameTable, ensure_ascii=False, indent=2)
        nameFile.write(nameTableContent)

def patchFile(patchPath, scriptPath):
    patch = {}
    with open(patchPath, 'r', encoding='utf-8') as f:
        patch = json.load(f)
    scriptLines = []
    with open(scriptPath, 'r', encoding='utf-8') as f:
        scriptLines = f.readlines()
    for entry in patch:
        line = entry['line']
        if 0 <= line < len(scriptLines):
            scriptLines[line] = entry['message']
    with open(scriptPath, 'w', encoding='utf-8') as f:
        f.writelines(scriptLines)

def patchFiles(patchDir, scriptDir):
    for file in os.listdir(patchDir):
        if file.endswith('.json'):
            patchPath = os.path.join(patchDir, file)
            scriptPath = os.path.join(scriptDir, file[:-5])
            if os.path.exists(scriptPath):
                patchFile(patchPath, scriptPath)
            else:
                print(f"Warning: Script file '{scriptPath}' does not exist. Skipping patching for '{file}'.")

def main():
    import argparse

    parser = argparse.ArgumentParser(description='Extract text from krkr script files or patch files.')
    parser.add_argument("mode", choices=['extract', 'patch'], help="Mode of operation: 'extract' to extract text, 'patch' to patch files.")
    parser.add_argument("path", help="Path to the script files.")
    parser.add_argument("path2", help="Output directory for extracted text files. Or patch file for 'patch' mode.")

    args = parser.parse_args()
    if args.mode == 'extract':
        if not os.path.exists(args.path):
            print(f"Error: The path '{args.path}' does not exist.")
            return
        if not os.path.isdir(args.path):
            print(f"Error: The path '{args.path}' is not a directory.")
            return
        if not os.path.exists(args.path2):
            os.makedirs(args.path2)
        extractFolder(args.path, args.path2)
    elif args.mode == 'patch':
        if not os.path.exists(args.path):
            print(f"Error: The patch file '{args.path}' does not exist.")
            return
        if not os.path.exists(args.path2):
            print(f"Error: The script directory '{args.path2}' does not exist.")
            return
        patchFiles(args.path2, args.path)
    else:
        print("Invalid mode. Use 'extract' or 'patch'.")

if __name__ == "__main__":
    main()