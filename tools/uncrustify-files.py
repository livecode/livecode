#!/usr/bin/env python
import sys
import os
import json
import re

def re_join_compile(regexp_list):
	return re.compile('^(' + '|'.join(regexp_list) + ')$')

if __name__ == '__main__':
	if len(sys.argv) < 3:
		print('Usage: uncrustify-gen.py ROOT PATTERNS_FILE')
		sys.exit(1)
	root = sys.argv[1]
	with open(sys.argv[2]) as fp:
		patterns = json.load(fp)

	include_re = re_join_compile(patterns['include'])
	exclude_re = re_join_compile(patterns['exclude'])

	for dir_path, dir_names, file_names in os.walk(root):
		for f in file_names:
			match_path = os.path.join(dir_path[len(root):], f)
			if not include_re.search(match_path):
				continue
			if exclude_re.search(match_path):
				continue
			print(os.path.join(dir_path, f))
