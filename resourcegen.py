import os

BANNED_FILES = set(['.DS_Store'])
OK_CHARS = list(map(ord, list('!@#$%^&*()-_+=`~[]{}|;:\',.<>/? ')))
for i in range(26):
    if i < 10: OK_CHARS.append(ord('0') + i)
    c = ord('a') + i
    OK_CHARS.append(c)
    OK_CHARS.append(c - ord('a') + ord('A'))
OK_CHARS = set(OK_CHARS)
escape_lookup = {
    "\n": "\\n",
    "\r": "\\r",
    "\t": "\\t",
    '"': '\\"',
    "\\": "\\\\",
}
ESCAPE_CHARS = {}
for ec in escape_lookup.keys():
    ESCAPE_CHARS[ord(ec)] = escape_lookup[ec]

def get_string_chunks(text):
    
    text = text.replace("\r\n", "\n").replace("\r", "\n")

    chars = list(map(ord, list(text)))
    if str(chars[:3]) == '[239, 187, 191]':
        chars = chars[3:]
    
    chunks = []
    current_chunk = []
    current_length = 0
    for c in chars:
        if c == 0: 
            print("Invalid char code: 0")
            return None
        
        if current_length >= 100:
            chunks.append(current_chunk)
            current_chunk = []
            current_length = 0

        if c in OK_CHARS:
            current_chunk.append(c)
            current_length += 1
        elif ESCAPE_CHARS.get(c) != None:
            current_chunk.append(c)
            current_length += 2
        else:
            print("Invalid char code:", c)
            return None
    if current_length > 0:
        
        if current_length < 20 and len(chunks) > 0:
            chunks[-1] += current_chunk
        else:
            chunks.append(current_chunk)
    
    output = []
    for chunk in chunks:
        sb = ['"']
        for char in chunk:
            if char in OK_CHARS:
                sb.append(chr(char))
            else:
                sb.append(ESCAPE_CHARS[char])
        sb.append('"')
        output.append(''.join(sb))

    return output


def main():
    files = {}
    dir = os.path.join('src', 'resources', 'files')
    for file in os.listdir(dir):
        if file not in BANNED_FILES:
            full_path = os.path.join(dir, file)
            c = open(full_path, 'rt')
            text = c.read()
            c.close()
            files[file] = text
    
    file_names = list(files.keys())
    file_names.sort()

    generated_code = []

    for file_name in file_names:
        chunks = get_string_chunks(files[file_name])
        generated_code.append('  name = new_string("' + file_name + '");')
        generated_code.append('  sb = new_string_builder();')
        for chunk in chunks:
            generated_code.append('  string_builder_append_chars(sb, ' + chunk + ');')
        generated_code.append('  dictionary_set(dict, name, string_builder_to_string_and_free(sb));')
        generated_code.append('')
    generated_code.pop()

    resource_file_path = os.path.join('src', 'resources', 'resources.h')
    c = open(resource_file_path, 'rt')
    src = c.read()
    c.close()
    lines = src.split("\n")
    start_index = -1
    end_index = -1
    for i in range(len(lines)):
        line = lines[i].strip()
        if line.startswith('//'):
            if 'GEN_BEGIN' in line:
                start_index = i
            elif 'GEN_END' in line:
                end_index = i
    
    header = lines[:start_index + 1]
    footer = lines[end_index:]

    new_src = "\n".join(header + generated_code + footer)
    c = open(resource_file_path, 'wt')
    c.write(new_src)
    c.close()



if __name__ == "__main__":
    main()