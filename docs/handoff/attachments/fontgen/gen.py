import os, sys
from pathlib import Path

repo, language, strings_dir, out_dir = sys.argv[1:5]
here = os.path.dirname(os.path.abspath(__file__))
sys.path[:0] = [os.path.join(here, 'shim'), os.path.join(repo, 'tools')]

import fontconv
import textproc

source = open(os.path.join(repo, 'tools', 'build-fonts.py'), encoding='utf-8').read()
head, rest = source.split('# Codepoint sets', 1)
languages = rest.split('# Languages', 1)[1]
languages = languages.replace('Path("../platform.io/src/system/strings")', 'Path(STRINGS_DIR)')
languages = languages.replace('    language = language_file.stem\n',
                              '    language = language_file.stem\n    if language != LANGUAGE:\n        continue\n')
assert 'STRINGS_DIR' in languages and 'LANGUAGE:' in languages

scope = {'__file__': os.path.join(repo, 'tools', 'build-fonts.py'), '__name__': 'fontgen'}
exec(compile(head, 'build-fonts-head', 'exec'), scope)


def run_fontconv(source, codepoint_set, dest, name, pixels=None, bpp=None,
                 ascent=None, descent=None, cap_height=None):
    if source.endswith('.bdf'):
        return
    args = ['-s', codepoint_set, '-n', name]
    if pixels:
        args += ['-p', str(pixels)]
    if bpp:
        args += ['-b', str(bpp)]
    if ascent:
        args += ['-a', str(ascent)]
    if descent:
        args += ['-d', str(descent)]
    if cap_height:
        args += ['-c', str(cap_height)]
    args += [os.path.join(repo, 'fonts', source), os.path.join(out_dir, dest)]
    sys.argv[1:] = args
    fontconv.main()


scope['run_fontconv'] = run_fontconv
scope['STRINGS_DIR'] = strings_dir
scope['LANGUAGE'] = language
exec(compile(languages, 'build-fonts-languages', 'exec'), scope)
