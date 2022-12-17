#!/usr/bin/env python3

import os
import argparse
import chardet
import os.path
import zipfile
import re

detector = chardet.universaldetector.UniversalDetector()

def find_files(path, ext='.zip'):
    """Find files in path with the given extension.

    Args:
      path: the path to search
      ext: the extension to match, optional (defaults to zip)

    Returns: a 2-tuple of parallel lists (directories, filenames)
    """
    files = []
    dirs = []
    for dirpath, dirnames, filenames in os.walk(path):
        files.extend([f for f in filenames if f.endswith(ext)])
        dirs.extend([dirpath for f in filenames if f.endswith(ext)])
    return (dirs, files)

def unzip(path, file, target_path):
    """Unzip the given path/file to the target_path."""
    full = os.path.join(path, file)
    with zipfile.ZipFile(full, 'r') as zip_ref:
        zip_ref.extractall(target_path)

def find_and_unzip(path, ext, target):
    """Finds all files with a given extension and unzips them.

    Args:
      path: path to search for files
      ext: an extension to filter files by
      target: the directory to unzip all files to

    Returns: none
    """
    (dirs, files) = find_files(path, ext)
    for i in range(len(files)):
        print(os.path.join(dirs[i], files[i]))
        unzip(dirs[i], files[i], target)

def clean(path, file):
    """Renames a Project Gutenberg file.

    Assumes there is a line with the format "TITLE: <title>\n" in the file, or
    the line "The Project Gutenberg Etext of <title>\n" The file will be renamed
    from its current name to the <title> after removing all non-alphanumeric
    characters from <title> and trimming the title to 200 characters.
    """
    global detector
    title = ''
    src = os.path.join(path, file)
    detector.reset()
    # TODO: only read the file once; buffer the bytes read and decode them to
    #       try to find the title within the buffer.
    with open(src, 'rb') as bf:
        for row in bf:
            detector.feed(row)
            if detector.done:
                break
    detector.close()
    enc = detector.result['encoding']
    print('{} - {}'.format(src, detector.result))
    title_lines = [
            "Project Gutenberg's Etext of ",
            "**Project Gutenberg's Etext of ",
            "*Project Gutenberg's Etext of ",
            "Title: ",
            "The Project Gutenberg Etext of ",
            "**The Project Gutenberg Etext of ",
            "*The Project Gutenberg Etext of ",
            "***Project Gutenberg Etext:",
            "**Project Gutenberg Etext:",
            "*Project Gutenberg Etext:",
            "Project Gutenberg Etext:",
            ]
    try:
        with open(src, 'r', encoding=enc) as f:
            for line in f:
                for tl in title_lines:
                    if line.startswith(tl):
                        title = line[len(tl):].rstrip()
                        break
                if title:
                    break
        if not title:
            print('NO TITLE FOR {}'.format(src))
            return
        title = ''.join(x for x in title if x.isalnum())[:200] + '.txt'
        dest = os.path.join(path, title)
        os.rename(src, dest)
    except Exception as e:
        print('ERROR PROCESSING {}: {}'.format(src, e))

def clean_output(target, file_pattern="\d*-?\d+\.txt"):
    """Find all text files and rename them to their Gutenberg title."""
    for dirpath, dirnames, filenames in os.walk(target):
        for f in filenames:
            if re.match(file_pattern, f):
                clean(dirpath, f)
            else:
                print('skipping {}...'.format(f))


def main():
    parser = argparse.ArgumentParser(
            prog='Lab 6 Library Extractor',
            description='Extracts zip files from the Gutenberg library')
    parser.add_argument('-t', '--target', dest='target', required=True,
            help='target directory for unzipped files')
    parser.add_argument('-e', '--extension', dest='ext', required=False,
            default='.zip', help='zip file extension')
    parser.add_argument('-p', '--path', dest='path', required=True,
            help='path to search for zip files')
    parser.add_argument('-s', '--skip', dest='skip', action='store_true',
            help='skip the unzipping of files')
    args = parser.parse_args()
    if not args.skip:
        find_and_unzip(args.path, args.ext, args.target)
    clean_output(args.target)

if __name__ == '__main__':
    main()

