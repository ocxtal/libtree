
/**
 * @file tree.c
 *
 * @brief an wrapper of ngx_rbtree.c in nginx (https://nginx.org/) core library
 */

#define UNITTEST_UNIQUE_ID		59
#include "unittest.h"

#include <stdint.h>
#include <stdlib.h>
#include "ngx_rbtree.h"
#include "lmm.h"
#include "log.h"
#include "sassert.h"
#include "tree.h"


/* constants */
#define  TREE_INIT_ELEM_CNT			( 64 )

/* roundup */
#define _roundup(x, base)			( ((x) + (base) - 1) & ~((base) - 1) )

/**
 * @struct tree_s
 */
struct tree_s {
	lmm_t *lmm;
	uint32_t object_size;
	uint32_t pad;
	struct tree_params_s params;

	int64_t vsize;
	int64_t vrem;
	uint8_t *v, *vhead, *vroot;
	ngx_rbtree_t t;
	ngx_rbtree_node_t sentinel;
};
#define _next_v(v)					( *((uint8_t **)v) )
#define _tail(v)					( (struct tree_node_s *)(v) )

/**
 * @struct tree_node_s
 */
struct tree_node_s {
	ngx_rbtree_node_t h;
};
_static_assert(sizeof(struct tree_node_s) == 40);
_static_assert(offsetof(struct tree_node_s, h.key) == TREE_OBJECT_OFFSET);
_static_assert(tree_get_object(NULL) == (void *)TREE_OBJECT_OFFSET);

/**
 * @fn tree_clean
 */
void tree_clean(
	tree_t *_tree)
{
	struct tree_s *tree = (struct tree_s *)_tree;
	if(tree == NULL) { return; }

	lmm_t *lmm = tree->lmm;

	uint8_t *v = tree->vroot;
	while(v != NULL) {
		uint8_t *vnext = _next_v(v);
		lmm_free(lmm, v); v = vnext;
	}
	lmm_free(lmm, tree);
	return;
}

/**
 * @fn tree_init
 */
tree_t *tree_init(
	uint64_t object_size,
	tree_params_t const *params)
{
	struct tree_params_s const default_params = { 0 };
	params = (params == NULL) ? &default_params : params;

	/* malloc mem */
	lmm_t *lmm = (lmm_t *)params->lmm;
	struct tree_s *tree = (struct tree_s *)lmm_malloc(lmm, sizeof(struct tree_s));
	if(tree == NULL) {
		return(NULL);
	}
	memset(tree, 0, sizeof(struct tree_s));

	/* set params */
	tree->lmm = lmm;
	tree->object_size = _roundup(object_size + sizeof(ngx_rbtree_node_t), 16);
	tree->params = *params;

	/* init vector */
	tree->vsize = tree->vrem = TREE_INIT_ELEM_CNT;
	tree->v = tree->vhead = tree->vroot = lmm_malloc(lmm,
		2 * sizeof(uint8_t *) + tree->vsize * tree->object_size);
	_next_v(tree->v) = NULL;

	/* init free list */
	struct tree_node_s *tail = (struct tree_node_s *)(tree->v += 2 * sizeof(uint8_t *));
	tail->h.key = (int64_t)NULL;
	debug("vector inited, v(%p), head(%p), root(%p)", tree->v, tree->vhead, tree->vroot);

	/* init tree */
	ngx_rbtree_init(&tree->t, &tree->sentinel, ngx_rbtree_insert_value);
	return((tree_t *)tree);
}

/**
 * @fn tree_create_node
 *
 * @brief create a new node (not inserted in the tree)
 */
tree_node_t *tree_create_node(
	tree_t *_tree)
{
	struct tree_s *tree = (struct tree_s *)_tree;
	struct tree_node_s *tail = _tail(tree->v);
	struct tree_node_s *node = NULL;

	/* check the recycle list */
	if((struct tree_node_s *)tail->h.key != NULL) {
		/* recycle removed space */
		node = (struct tree_node_s *)tail->h.key;
		debug("node recycled, node(%p)", node);

		/* update root of the freed list */
		tail->h.key = node->h.key;
	} else {
		/* add new node */
		node = tail;
		tree->v += tree->object_size;

		if(--tree->vrem <= 0) {
			/* add new vector */
			tree->vrem = (tree->vsize *= 2);
			tree->vhead = tree->v = (_next_v(tree->vhead) = lmm_malloc(tree->lmm,
				2 * sizeof(uint8_t *) + tree->vsize * tree->object_size));
			_next_v(tree->v) = NULL;

			/* adjust v */
			tree->v += 2 * sizeof(uint8_t *);
			debug("added new vector, v(%p), vhead(%p), vroot(%p)", tree->v, tree->vhead, tree->vroot);
		}

		/* copy root of free list */
		_tail(tree->v)->h.key = node->h.key;
		debug("new node created, node(%p)", node);
	}

	/* mark node */
	node->h.data = 0xff;
	return((tree_node_t *)node);
}

/**
 * @fn tree_insert
 *
 * @brief insert a node
 */
void tree_insert(
	tree_t *_tree,
	tree_node_t *_node)
{
	struct tree_s *tree = (struct tree_s *)_tree;
	struct tree_node_s *node = (struct tree_node_s *)_node;
	debug("tree->root(%p), tree->sentinel(%p)", tree->t.root, tree->t.sentinel);
	ngx_rbtree_insert(&tree->t, (ngx_rbtree_node_t *)node);
	return;
}

/**
 * @fn tree_remove
 *
 * @brief remove a node, automatically freed if malloc'd with tree_reserve_node
 */
void tree_remove(
	tree_t *_tree,
	tree_node_t *_node)
{
	struct tree_s *tree = (struct tree_s *)_tree;
	struct tree_node_s *node = (struct tree_node_s *)_node;
	ngx_rbtree_delete(&tree->t, (ngx_rbtree_node_t *)node);

	if(node->h.data == 0xff) {
		/* append node to the head of freed list */
		struct tree_node_s *tail = _tail(tree->v);
		node->h.key = tail->h.key;
		tail->h.key = (int64_t)node;
	}
	return;
}

/**
 * @fn tree_search_key
 *
 * @brief search a node by key, returning the leftmost node
 */
tree_node_t *tree_search_key(
	tree_t *_tree,
	int64_t key)
{
	struct tree_s *tree = (struct tree_s *)_tree;
	return((tree_node_t *)ngx_rbtree_find_key(&tree->t, key));
}

/**
 * @fn tree_search_key_left
 *
 * @brief search a node by key. returns the nearest node in the left half of the tree if key was not found.
 */
tree_node_t *tree_search_key_left(
	tree_t *_tree,
	int64_t key)
{
	struct tree_s *tree = (struct tree_s *)_tree;
	return((tree_node_t *)ngx_rbtree_find_key_left(&tree->t, key));
}

/**
 * @fn tree_search_key_right
 *
 * @brief search a node by key. returns the nearest node in the right half of the tree if key was not found.
 */
tree_node_t *tree_search_key_right(
	tree_t *_tree,
	int64_t key)
{
	struct tree_s *tree = (struct tree_s *)_tree;
	return((tree_node_t *)ngx_rbtree_find_key_right(&tree->t, key));
}

/**
 * @fn tree_left
 *
 * @brief returns the left next node
 */
tree_node_t *tree_left(
	tree_t *_tree,
	tree_node_t const *node)
{
	struct tree_s *tree = (struct tree_s *)_tree;
	return((tree_node_t *)ngx_rbtree_find_left(&tree->t, (ngx_rbtree_node_t *)node));
}

/**
 * @fn tree_right
 *
 * @brief returns the right next node
 */
tree_node_t *tree_right(
	tree_t *_tree,
	tree_node_t const *node)
{
	struct tree_s *tree = (struct tree_s *)_tree;
	return((tree_node_t *)ngx_rbtree_find_right(&tree->t, (ngx_rbtree_node_t *)node));
}


/* unittests */
unittest_config(
	.name = "tree"
);

/* create tree object */
unittest()
{
	tree_t *tree = tree_init(8, NULL);

	assert(tree != NULL);

	tree_clean(tree);
}

/* create node */
unittest()
{
	tree_t *tree = tree_init(8, NULL);

	tree_node_t *node = tree_create_node(tree);
	assert(node != NULL);

	int64_t *obj = (int64_t *)tree_get_object(node);
	obj[0] = 0xcafebabe;
	obj[1] = 0x12345678;

	/* insert and search */
	tree_insert(tree, node);
	tree_node_t *found = tree_search_key(tree, 0xcafebabe);
	assert(found == node, "found(%p), node(%p)", found, node);
	assert(obj[0] == 0xcafebabe, "obj[0](%lld)", obj[0]);
	assert(obj[1] == 0x12345678, "obj[1](%lld)", obj[1]);

	/* remove */
	tree_remove(tree, node);
	found = tree_search_key(tree, 0xcafebabe);
	assert(found == NULL, "found(%p), node(%p)", found, node);

	tree_clean(tree);
}

/* create multiple nodes */
unittest()
{
	tree_t *tree = tree_init(8, NULL);

	debug("tree->root(%p), tree->sentinel(%p)", tree->t.root, tree->t.sentinel);

	/* insert */
	tree_node_t *narr[256];
	for(int64_t i = 0; i < 256; i++) {
		tree_node_t *n = narr[i] = tree_create_node(tree);
		assert(n != NULL);

		int64_t *o = (int64_t *)tree_get_object(n);
		o[0] = (0xff & ((i<<4) | (i>>4)))<<1;
		o[1] = i;

		// debug("insert elem o[0](%lld), o[1](%lld)", o[0], o[1]);
		// debug("tree->root(%p), tree->sentinel(%p)", tree->t.root, tree->t.sentinel);

		// debug("i(0), n->parent(%p), n->left(%p), n->right(%p)",
			// narr[0]->h.parent, narr[0]->h.left, narr[0]->h.right);
		tree_insert(tree, n);
		// debug("i(%lld), n->parent(%p), n->left(%p), n->right(%p)",
			// i, n->h.parent, n->h.left, n->h.right);
	}

	/* search */
	for(int64_t i = 0; i < 256; i++) {
		tree_node_t *n = tree_search_key(tree, i<<1);
		assert(n != NULL);

		int64_t *obj = (int64_t *)tree_get_object(n);
		assert(obj[0] == i<<1, "obj[0](%lld), key(%lld)", obj[0], i<<1);
		assert(obj[1] == (0xff & ((i<<4) | (i>>4))), "obj[1](%lld), val(%lld)", obj[1], 0xff & ((i<<4) | (i>>4)));
	}

	tree_clean(tree);
}

/**
 * end of tree.c
 */
