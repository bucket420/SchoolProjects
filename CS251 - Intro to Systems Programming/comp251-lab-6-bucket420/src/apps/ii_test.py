#!/usr/bin/env python3

import os
import subprocess
import argparse
import tempfile
import shutil
import traceback

from random import randint

OUTDIR = 'ii-test-out'
INDIR = 'ii-test-in'

def word(args):
    return str(randint(0, args.words))

def write_file(file, fname, args, idx):
    """Write the ith output file to the give file handle."""
    for line in range(args.n):
        for _ in range(args.l):
            w = word(args)
            file.write(w + ' ')
            if w not in idx:
                idx[w] = dict()
            if fname not in idx[w]:
                idx[w][fname] = list()
            idx[w][fname].append(line)
        file.write('\n')
    return

def setup(args):
    """Create temporary directories and input files."""
    # create io dirs
    print('creating input directories in {}'.format(tempfile.gettempdir()))
    indir = tempfile.mkdtemp(prefix=args.indir)
    outdir = tempfile.mkdtemp(prefix=args.outdir)
    files = []
    print('input file directory: {}\noutput file directory: {}'.format(
        indir, outdir))
    idx = dict()
    # create input files
    for i in range(args.f):
        f = tempfile.mkstemp(suffix='.txt', prefix='in', dir=indir, text=True)
        write_file(os.fdopen(f[0], 'w'), f[1], args, idx)
    print('wrote {} input files'.format(args.f))
    return (idx, files, indir, outdir)

def list_ii_files(idx_dir):
    """Retrieve a listing of ii files from the index dir.

    Args:
      idx_dir: string path to find the index.

    Returns: a tuple ([ii], alias) of ii files and the alias file.
    """
    files = []
    alias_fname = None
    for dirpath, dirname, filenames in os.walk(idx_dir): 
        files.extend([f for f in filenames if f.endswith('.idx')])
        for f in filenames:
            if f.endswith('.aliases'):
                alias_fname = f
                break
    return (files, alias_fname)


def validate(args, idx, indir, outdir, result):
    """Validate program output."""
    # check output
    if not result:
        print('Failed to execute.')
        return False
    # validate
    files, alias_fname = list_ii_files(outdir)
    errs = []
    if not files:
        errs.append('Could not find ii files!')
        return errs
    if not alias_fname:
        errs.append('Could not find aliases!')
        return errs
    files = [os.path.join(outdir, f) for f in files]
    alias_fname = os.path.join(outdir, alias_fname)

    aliases = dict()
    try:
        f = open(alias_fname, 'r')
        for line in f:
            fname, alias = line.rstrip().split(';')
            aliases[alias] = fname
        f.close()
    except Exception as e:
        message = 'Failed to parse alias file {}: {}'.format(alias_fname, e)
        print(message)
        errs.append(message)
        return errs

    file = ''
    line = ''
    try:
        for file in files:
            f = open(file, 'r')
            for line in f:
                line = line.rstrip()
                word, flist = line.split(':')
                if word not in idx:
                    errs.append('word {}: does not appear in index'.format(wor))
                    continue
                flist = flist.split(';')
                for a in flist:
                    if not a:
                        continue
                    fname, nums = a.split('(')
                    if fname not in aliases:
                        errs.append('file alias {} not in aliases {}'.format(fname, aliases))
                        continue
                    fname = aliases[fname] # map to actual filename
                    nums = nums.rstrip(')').split(',')
                    if fname not in idx[word]:
                        errs.append(
                        'file {} word {}: listed as appearing in {} but is not in index'.format(file, word, fname))
                        continue
                    l = idx[word][fname]
                    for num in nums:
                        num = int(num)
                        if num not in l:
                            errs.append('file {} word {}: line number {} not in index {}'.format(file, word, num, l))
                        else:
                            l.remove(num)
                    if len(l) > 0:
                        errs.append('file {} word {}: lines {} missing from {}'.format(file, word, l, fname))
                    del idx[word][fname]
                if len(idx[word]) > 0:
                    errs.append('file {} word {}: missing files {}'.format(file, word, idx[word]))
                del idx[word]
            f.close()
        if len(idx):
            errs.append('missing words {}'.format([k for k in idx]))
    except Exception as e:
        message = 'Failed to parse {}:{}: {}\n{}'.format(file, line, e, traceback.format_exc())
        print(message)
        errs.append(message)
    return errs

def run(args, idx, indir, outdir):
    """Run the ii generator."""
    # run program
    cmd = [args.binary,
            '-d', indir,
            '-e', '.txt',
            '-p', str(args.parallelism),
            '-o', outdir,
            '-s', str(args.shards),
            '-m', str(args.mapsize)]
    print('Executing {}'.format(cmd))
    result = None
    try:
        result = subprocess.run(cmd, timeout=args.timeout)
    except subprocess.SubprocessError as e:
        print('Failed to execute {}: {}'.format(cmd, e))
    # validate
    return validate(args, idx, indir, outdir, result)

def teardown(args, errs, indir, outdir):
    """Optionally clean up temp files."""
    if args.keep_failed and len(errs) > 0:
        print('test failed, leaving output files...')
        return
    print('rm {}'.format(indir))
    shutil.rmtree(indir)
    print('rm {}'.format(outdir))
    shutil.rmtree(outdir)
    return


def main():
    parser = argparse.ArgumentParser(prog='inverted index test',
            description='test that creates synthetic data to process and validate output')
    parser.add_argument('-b', '--binary', dest='binary',
            default='./apps/ii-main', help='path to binary to run')
    parser.add_argument('-i', '--inputdir', dest='indir', default=INDIR,
            help='prefix for directory for input files')
    parser.add_argument('-o', '--outputdir', dest='outdir', default=OUTDIR,
            help='prefix for directory for output files')
    parser.add_argument('-l', dest='l', default=2, type=int,
            help='number of input words per line')
    parser.add_argument('-n', dest='n', default=100, type=int,
            help='number of input lines per file')
    parser.add_argument('-f', dest='f', default=2, type=int,
            help='number of input files')
    parser.add_argument('-w', '--words', dest='words', default=200, type=int,
            help='number of input words')
    parser.add_argument('-p', '--parallelism', dest='parallelism', default=1,
            type=int, help='parallelism of parsing')
    parser.add_argument('-s', '--shards', dest='shards', default=2, type=int,
            help='output shards')
    parser.add_argument('-m', '--mapsize', dest='mapsize', default=8192, type=int,
            help='map size')
    parser.add_argument('-k', '--keep-failed-files', dest='keep_failed',
            action='store_true', help='keep input/output files on failed runs')
    parser.add_argument('-t', '--timeout', dest='timeout', type=int,
            default=120, help='timeout in seconds for subprocess')
    args = parser.parse_args()
    print(args)
    errs = ['dummy']
    try:
        idx, files, indir, outdir = setup(args)
        errs = run(args, idx, indir, outdir)
        if errs:
            print('FAILED')
            for err in errs:
                print('\t> {}'.format(err))
        else:
            print('SUCCESS')
    except Exception as e:
        print('Error running test: {}'.format(e))
        traceback.print_exc()
    teardown(args, errs, indir, outdir)


if __name__ == '__main__':
    main()


