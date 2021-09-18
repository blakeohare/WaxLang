FILENAME = 'config-formatting.txt'

import array
import os
import sys

class FormatStyle:
  def __init__(self):
    self.newline_char = '\n'
    self.tab_char = '    '
    self.should_rtrim = True
    self.should_trim = True
    self.should_end_with_newline = True
    self.canonicalize_csproj_tools_version = False
    self.include_bom = True
    self.xcode_proj = False

  def tabs(self, tab_char):
    self.tab_char = tab_char
    return self

  def newline(self, newline_char):
    self.newline_char = newline_char
    return self

  def rtrim(self, should_rtrim):
    self.should_rtrim = should_rtrim
    return self

  def noBom(self):
    self.include_bom = False
    return self

  def disableEndNewline(self):
    self.should_end_with_newline = False
    return self

  def enableCanonicalizeCsprojToolsVersion(self):
    self.canonicalize_csproj_tools_version = True
    return self

  def xcodeProjNoDeveloperId(self):
    self.xcode_proj = True
    return self

  def apply(self, text):
    if self.should_trim:
      text = text.strip()

    if self.canonicalize_csproj_tools_version:
      if 'ToolsVersion="' in text and not 'ToolsVersion="14.0"' in text:
        tools_version_loc = text.find('ToolsVersion=')
        first_quote = text.find('"', tools_version_loc)
        close_quote = text.find('"', first_quote + 1)
        text = text[:first_quote] + '"14.0' + text[close_quote:]

    lines = text.replace('\r\n', '\n').split('\n')

    if self.should_rtrim:
      lines = map(lambda x: x.rstrip(), lines)

    if self.xcode_proj:
      # TODO: configure this
      lines = fix_pbxproj_file(lines, 'ID_GOES_HERE')

    new_lines = []
    for line in lines:
      tabs = 0
      trimming = True
      while trimming:
        if len(line) > 0:
          if line[0] == '\t':
            line = line[1:]
            tabs += 1
          elif line.startswith(self.tab_char):
            line = line[len(self.tab_char):]
            tabs += 1
          else:
            trimming = False
        else:
          trimming = False
      if tabs > 0:
        new_lines.append((self.tab_char * tabs) + line)
      else:
        new_lines.append(line)

    if self.should_end_with_newline:
      new_lines.append('')

    text = '\n'.join(new_lines)

    return text

styles = {
  'ACRYLIC': FormatStyle().tabs(' ' * 4).newline('\n').noBom(),
  'C': FormatStyle().tabs(' ' * 2).newline('\n').noBom(),
  'CSHARP': FormatStyle().tabs(' ' * 4).newline('\r\n'),
  'CRAYON': FormatStyle().tabs(' ' * 4).newline('\n'),
  'CSPROJ': FormatStyle().tabs(' ' * 2).newline('\r\n').disableEndNewline().enableCanonicalizeCsprojToolsVersion(),
  'PASTEL': FormatStyle().tabs(' ' * 4).newline('\n'),
  'PYTHON': FormatStyle().tabs(' ' * 2).newline('\n').noBom(),
  'PYTHON_4_SPACES': FormatStyle().tabs(' ' * 4).newline('\n').noBom(),
  'JAVA': FormatStyle().tabs(' ' * 4).newline('\n').noBom(),
  'JAVA_ANDROID': FormatStyle().tabs(' ' * 2).newline('\n'),
  'JAVASCRIPT': FormatStyle().tabs('\t').newline('\n'),
  'JSON': FormatStyle().tabs(' ' * 4).newline('\n').noBom(),
  'MARKDOWN': FormatStyle().tabs(' ' * 2).newline('\n').noBom(),
  'PHP': FormatStyle().tabs(' ' * 4).newline('\n').disableEndNewline().noBom(),
  'PBXPROJ': FormatStyle().tabs("\t").newline('\n').noBom().xcodeProjNoDeveloperId(),
  'SWIFT': FormatStyle().tabs(' ' * 4).newline('\n').noBom(),

  'FORMAT_CONFIG': FormatStyle().tabs('    ').newline('\n'),
}

def os_pathify(paths):
  return list(map(lambda f: f.replace('/', os.sep), paths))

def get_all_files():
  output = []
  get_all_files_impl('.', output)
  return output

BAD_PATH_MARKERS = os_pathify([
  '/obj/Debug',
  '/obj/Release',
  '/bin/Debug',
  '/bin/Release',
  '/node_modules',
  '/.vscode',
])

IGNORE_FILES = os_pathify([
  'package.json',
  '.min.js',
])

def get_all_files_impl(path, output):
  for file in os.listdir(path):
    full_path = path + os.sep + file
    if os.path.isdir(full_path):
      bad = False
      for bad_path_marker in BAD_PATH_MARKERS:
        if full_path.endswith(bad_path_marker):
          bad = True
          break
      if not bad:
        get_all_files_impl(full_path, output)
    else:
      bad = False
      for ignore_file in IGNORE_FILES:
        if full_path.endswith(ignore_file):
          bad = True
          break
      if not bad:
        output.append(full_path[2:])

def main():

  if sys.version_info.major < 3:
    print("Python 2 is not supported by this script.")
    return

  config_file = read_text(FILENAME) + "\n\nFORMAT_CONFIG: " + FILENAME + "\n"

  MATCHERS = []
  for raw_line in config_file.split('\n'):
    t = raw_line.split('#')[0].strip().split(':')
    if len(t) >= 2:
      style_name = t[0].strip()
      style = styles.get(style_name)
      pattern = ':'.join(t[1:]).strip()
      if style == None:
        if style_name == 'IGNORE':
          IGNORE_FILES.append(pattern)
        elif style_name == 'BAD_PATH':
          BAD_PATH_MARKERS.append(pattern)
        else:
          print("Unknown style: " + style_name)
      else:
        MATCHERS.append((pattern, style))

  all_files = get_all_files()
  for pattern, matcher in MATCHERS:
    if '*' in pattern:
      prefix, ext = pattern.split('*')
    else:
      parts = pattern.split('.')
      prefix = '.'.join(parts[:-1])
      ext = parts[-1]
    for file in all_files:
      canonical_file = file.replace('\\', '/')
      if canonical_file .startswith(prefix) and canonical_file .endswith(ext):
        text = read_text(file)
        text = matcher.apply(text)
        write_text(file, text, matcher.newline_char, matcher.include_bom)

def read_text(path):
  c = open(path, 'rb')
  raw_bytes = c.read()
  c.close()
  if len(raw_bytes) > 3 and raw_bytes[0] == 239 and raw_bytes[1] == 187 and raw_bytes[2] == 191:
    raw_bytes = raw_bytes[3:]
  try:
    return raw_bytes.decode('utf-8')
  except:
    pass
  ascii = []
  for c in raw_bytes:
    ascii.append(chr(c))
  return ''.join(ascii)

def read_bytes(path):
  c = open(path, 'rb')
  text = c.read()
  c.close()
  return bytearray(text)

def string_to_byte_array(s):
  if sys.version_info.major == 2:
    return array.array('B', s)
  else:
    return bytearray(s, 'utf-8')

def write_text(path, text, newline_char, include_bom):
  bytes = string_to_byte_array(text)
  if include_bom:
    new_bytes = [239, 187, 191]
  else:
    new_bytes = []
  if newline_char == '\n':
    for byte in bytes:
      new_bytes.append(byte)
  else:
    for byte in bytes:
      if byte == 10:
        new_bytes.append(13)
        new_bytes.append(10)
      else:
        new_bytes.append(byte)

  old_bytes = read_bytes(path)
  update = False
  if len(old_bytes) != len(new_bytes):
    update = True
  elif len(old_bytes) > 0:
    if old_bytes[-1] != new_bytes[-1]:
      update = True
    else:
      for old, new in zip(old_bytes, new_bytes):
        if old != new:
          update = True
          break

  if update:
    print("Updating: " + path)
    c = open(path, 'wb')
    c.write(bytearray(new_bytes))
    c.close()

def fix_pbxproj_file(lines, devId):
  output = []
  for line in lines:
    if '=' in line and ('DevelopmentTeam =' in line or 'DEVELOPMENT_TEAM =' in line):
      if devId == None:
        pass
      else:
        output.append(line.split('=')[0] + '= ' + devId + ';')
    else:
      output.append(line)
  return output

main()
