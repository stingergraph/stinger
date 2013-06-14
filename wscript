#!/bin/python

APPNAME = 'stinger'
#'git log -1 --format=%ci'
VERSION = '2013.06'

top = '.'
out = 'obj'

def recurse(ctx, where, out):
  """ helper function that """
  orig_out = ctx.out_dir
  ctx.out_dir += '/' + out
  ctx.recurse(where)
  ctx.out_dir = orig_out

def options(ctx):
  ctx.load('compiler_c compiler_cxx')

  # ctx.add_option('--foo', action='store', default=False, help='Silly test')
  # Get result through ctx.options.foo

  ctx.add_option('--debug', action='store', default=False, help='If true, set compile flags for debugging')

def configure(ctx):
  ctx.load('compiler_c compiler_cxx')
  ctx.check(features='cxx cxxprogram', lib=['m'], cflags=['-Wall'], defines=[], uselib_store='M')

def dist(ctx):
  ctx.algo	= 'tar.gz'
  ctx.excl	= '**/.gitignore **/.git **/.svn **/*.o **/*.dSYM **/*.pl'

def build(ctx):
  recurse(ctx, 'src_lib', 'lib')
  recurse(ctx, 'src_bin', 'bin')
