#!/usr/bin/env python3

import os
import argparse

import ii_test

def lookup(words, idx_dir):
    """lookup a word given the given idx_dir containing the inverted index."""
    files, alias_fname = ii_test.list_ii_files(idx_dir)
    if not files or not alias_fname:
        print('Could not find ii files in {}'.format(idx_dir))
        return
    # locate the file in which the words belogs
    word_files = dict()
    for f in files:
        shards, start_end = f.rstrip().rstrip('.idx').split('_')
        start, end = start_end.split('-')
        for word in words:
            if word >= start and word <= end:
                word_files[word] = f
    if len(word_files) < 1:
        print('Could not find word list in {}'.format(files))
        return
    # build alias dict
    aliases = dict()
    f = open(os.path.join(idx_dir, alias_fname), 'r')
    for line in f:
        file,alias = line.rstrip().split(';')
        aliases[alias] = file
    f.close()
    for word in words:
        print('> {}'.format(word))
        if word not in word_files:
            continue
        f = open(os.path.join(idx_dir, word_files[word]))
        for line in f:
            if not line.startswith(word+':'):
                continue
            _, appearances = line.rstrip().split(':')
            for v in appearances.split(';'):
                if not v:
                    continue
                alias, counts = v.rstrip(')').split('(')
                print('\t {} - lines {}'.format(aliases[alias], counts))


def main():
    parser = argparse.ArgumentParser(prog='inverted index query tool',
            description='tool to query inverted index ouput by apps/ii-main')
    parser.add_argument('-i', '--index', dest='index',
            help='path to output of ii tool', required=True)
    parser.add_argument('-w', '--words', dest='words',
            help='one or more words to look up, separated by commas', required=True)
    args = parser.parse_args()
    args.words = args.words.split(',')

    lookup(args.words, args.index)

if __name__ == '__main__':
    main()
