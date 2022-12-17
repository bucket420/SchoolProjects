#!/bin/bash

srcdir="/opt/gutenberg/txt"
limit=300000
outdir='books-input'

while getopts o:l:s: flag
do
  case "${flag}" in
    o) outdir=${OPTARG};;
    l) limit=${OPTARG};;
    s) srcdir=${OPTARG};;
  esac
done

mkdir -p $outdir
rm -f $outdir/*

files=($srcdir/*.txt)
while [ `du -s $outdir | awk '{print $1}'` -lt $limit ]; do
  file=${files[RANDOM % ${#files[@]}]}
  cp $file $outdir
done

