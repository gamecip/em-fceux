#!/usr/bin/env python
import sys, os, time, subprocess, random, re, shutil, gzip


dstdir = 'deploytmp'


class Meta:
        def __init__(self, **vargs):
                self.__dict__ = vargs

	def getPath(self):
		return os.path.join(self.dir, self.file)


def createRecords(path):
	result = []
	for dirname, dirnames, filenames in os.walk(path):
		#for subdirname in dirnames:
		#	print(os.path.join(dirname, subdirname))
		
		for filename in filenames:
			result.append(Meta(dir=dirname, file=filename))
		
		# Exclude .git directory:
		if '.git' in dirnames:
			dirnames.remove('.git')

	return result


def makeHashed(records, exclude, fallback_hash):
	for r in records:
		path = r.getPath()
		if r.file in exclude:
			hsh = ''
		else:
			hsh = subprocess.check_output([ 'git', 'log', '-1', "--pretty=format:-%h", '--', path ])
			if not hsh:
				hsh = fallback_hash
				print 'WARNING: using fallback hash, no file commit found: %s' % (path)

		#spl = os.path.splitext(r.file)
		#r.hashed = spl[0] + hsh + spl[1]
		spl = r.file.split('.', 1)
		spl[0] += hsh
		r.hashed = '.'.join(spl)
		print 'INFO: hashed: %s -> %s' % (r.file, r.hashed)

	return records

def setupOutput(records, srcdir, outdir):
	for r in records:
		relative = os.path.relpath(r.dir, srcdir)
		r.outdir = os.path.normpath(os.path.join(outdir, relative))
		r.outpath = os.path.join(r.outdir, r.hashed)

	return records

def createMapping(records):
	result = {}
	for r in records:
		if r.file in result:
			print 'ERROR: duplicate filename found: %s' % (r.file)
			exit(2)
		result[r.file] = r.hashed

	return result

def _doRegexpReplace(r, re, func):
	f = open(r.getPath(), 'r')
	tmp = f.read()
	f.close()
	result = re.sub(func, tmp)
	ensureDir(r.outpath)
	f = open(r.outpath, 'w')
	f.write(result)
	f.close()

# Process template file with {{filename}} markups
re_template = re.compile('{{([^}]*)}}')
def doTemplate(r, mapping):
	_doRegexpReplace(r, re_template, lambda x: mapping[x.group(1)])

# Process fceux.js file by replacing references to data and mem files
re_special = re.compile('(?<=\Wfceux)()(?=\.(data|js\.mem))')
def doSpecial(r, fallback_hash):
	_doRegexpReplace(r, re_special, lambda x: fallback_hash)

# Process file by copying
def doCopy(r):
	ensureDir(r.outpath)
	shutil.copy(r.getPath(), r.outpath)

def processRecords(records, mapping, templated, special, fallback_hash):
	
	for r in records:
		if r.file in templated:
			doTemplate(r, mapping)
			print 'INFO: template: %s -> %s' % (r.getPath(), r.outpath)
		elif r.file in special:
			doSpecial(r, fallback_hash)
			print 'INFO: special: %s -> %s' % (r.getPath(), r.outpath)
		else:
			doCopy(r)
			print 'INFO: copy: %s -> %s' % (r.getPath(), r.outpath)

def ensureDir(path):
	path = os.path.dirname(path)
	if not os.path.exists(path):
		print 'INFO: created dir(s): %s' % (path)
		os.makedirs(path)

def compressRecords(records, mapping, compress):
	for r in records:
		if r.file not in compress:
			continue
		dst = r.outpath + '.gz'
		with open(r.outpath, 'rb') as fi, gzip.open(dst, 'wb') as fo:
			shutil.copyfileobj(fi, fo)

def build(srcdir, outdir):
	srcdir = os.path.join(os.path.normpath(srcdir), '')
	outdir = os.path.join(os.path.normpath(outdir), '')

	fallback_hash = subprocess.check_output([ 'git', 'log', '-1', "--pretty=format:-%h" ])
	if not fallback_hash:
		print 'WARNING: no git commits found, using seconds since epoch'
		fallback_hash = '-%07x' % (int(time.time()) & 0xfffffff)
	print 'INFO: fallback hash: %s' % (fallback_hash)

	records = createRecords(srcdir)
	records = makeHashed(records, [ 'index.html', 'gpl-2.0.txt' ], fallback_hash)
	records = setupOutput(records, srcdir, outdir)
	mapping = createMapping(records)
	processRecords(records, mapping, [ 'index.html', 'style.css', 'loader.js' ], [ 'fceux.js' ], fallback_hash)
	compressRecords(records, mapping, [ 'fceux.js', 'fceux.js.mem', 'fceux.data', 'style.css', 'loader.js' ])

if __name__ == '__main__':
	build(sys.argv[1], sys.argv[2])
