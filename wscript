#! /usr/bin/env python
# encoding: utf-8

def options(opt):
	opt.load('compiler_c')

def configure(conf):
	conf.load('ar')
	conf.load('compiler_c')

	conf.env.append_value('CFLAGS', '-O3')
	conf.env.append_value('CFLAGS', '-std=c99')
	conf.env.append_value('CFLAGS', '-march=native')

	conf.env.append_value('OBJ_TREE', ['tree.o', 'ngx_rbtree.o'])


def build(bld):
	bld.objects(source = 'tree.c', target = 'tree.o')
	bld.objects(source = 'ngx_rbtree.c', target = 'ngx_rbtree.o')

	bld.stlib(
		source = ['unittest.c'],
		target = 'tree',
		use = bld.env.OBJ_TREE,
		lib = bld.env.LIB_TREE,
		defines = bld.env.DEFINES_TREE)

	bld.program(
		source = ['unittest.c'],
		target = 'unittest',
		use = bld.env.OBJ_TREE,
		lib = bld.env.LIB_TREE,
		defines = ['TEST'] + bld.env.DEFINES_TREE)
