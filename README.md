# libtree

Libtree is an wrapper of red-black-tree implementation in the [nginx](https://nginx.org/) core library and interval tree implementation based on it. It provides node allocation and automatic cleanup in addition to the original library, with simpler interface.



## Struct and Functions

### Red-black-tree

#### rbtree\_node\_t

Node object of the red-black tree, must be placed at the head of the object passed to the rbtree functions.

```
struct rbtree_node_s {
	uint8_t pad[24];
	int64_t zero;				/* must be zeroed if external memory is used */
	int64_t key;
};
typedef struct rbtree_node_s rbtree_node_t;
```

#### rbtree\_init

Initialize a red-black-tree object.

```
rbtree_t *rbtree_init(uint64_t object_size, rbtree_params_t const *params);
```


####  rbtree\_clean

Destroy the tree.

```
void rbtree_clean(tree_t *tree);
```

#### rbtree\_flush

Flush contents of the tree without destroying the tree object.

```
void rbtree_flush(rbtree_t *tree);
```

#### rbtree\_create\_node

Create a new node. Note that the created node object is not yet inserted in the tree. The object is automatically freed when `rbtree_remove` or `rbtree_clean` is called.

```
rbtree_node_t *rbtree_create_node(rbtree_t *tree);
```

#### rbtree\_insert

Insert a node to the tree.

```
void rbtree_insert(rbtree_t *tree, rbtree_node_t *node);
```

#### rbtree\_remove

Remove a node, automatically freed if malloc'd with `rbtree_create_node`.

```
void rbtree_remove(rbtree_t *tree, rbtree_node_t *node);
```

#### rbtree\_search\_key

Search a node by key, returning the leftmost node.

```
rbtree_node_t *rbtree_search_key(rbtree_t *tree, int64_t key);
```

#### rbtree\_search\_key\_left

Search a node by key. Returns the nearest node in the left half of the tree if key was not found.

```
rbtree_node_t *rbtree_search_key_left(rbtree_t *tree, int64_t key);
```

#### rbtree\_search\_key\_right

Search a node by key. Returns the nearest node in the right half of the tree if key was not found.

```
rbtree_node_t *rbtree_search_key_right(rbtree_t *tree, int64_t key);
```

#### rbtree\_left

Returns the left next node.

```
rbtree_node_t *tree_left(rbtree_t *tree, rbtree_node_t const *node);
```

#### rbtree\_right

Returns the right next node

```
rbtree_node_t *rbtree_right(rbtree_t *tree, rbtree_node_t const *node);
```

#### rbtree\_walk

Apply a function (`rbtree_walk_t`) to nodes in a leaf-to-root order.

```
typedef void (*rbtree_walk_t)(rbtree_node_t *node, void *ctx);
void rbtree_walk(rbtree_t *tree, rbtree_walk_t fn, void *ctx);
```

### Interval tree

#### ivtree\_node\_t

Node object of the interval tree, must be placed at the head of the object passed to the ivtree functions.

```
struct ivtree_node_s {
	uint8_t pad[24];
	int64_t zero;				/* must be zeroed if external memory is used */
	int64_t lkey;
	int64_t rkey;
	int64_t reserved;
};
typedef struct ivtree_node_s ivtree_node_t;
```

#### ivtree\_init

Initialize a interval tree object.

```
ivtree_t *ivtree_init(uint64_t object_size, ivtree_params_t const *params);
```

#### ivtree\_clean

Destroy the tree.

```
void ivtree_clean(ivtree_t *tree);
```

#### ivtree\_flush

Flush contents of the tree without destroying the tree object.

```
void ivtree_flush(ivtree_t *tree);
```

#### ivtree\_create\_node

Create a new node. Note that the created node object is not yet inserted in the tree. The object is automatically freed when `ivtree_remove` or `ivtree_clean` is called.

```
ivtree_node_t *ivtree_create_node(ivtree_t *tree);
```

#### ivtree\_insert

Insert a node to the tree.

```
void ivtree_insert(ivtree_t *tree, ivtree_node_t *node);
```

#### ivtree\_remove

Remove a node, automatically freed if malloc'd with `ivtree_create_node`.

```
void ivtree_remove(ivtree_t *tree, ivtree_node_t *node);
```

#### ivtree\_contained

Return an iterator of a set of sections contained in [lkey, rkey)

```
ivtree_iter_t *ivtree_contained(ivtree_t *tree, int64_t lkey, int64_t rkey);
``

#### ivtree\_containing

Return an iterator of a set of sections containing [lkey, rkey)

```
ivtree_iter_t *ivtree_containing(ivtree_t *tree, int64_t lkey, int64_t rkey);
```

#### ivtree\_intersect

Return an iterator of a set of sections intersect with [lkey, rkey)

```
ivtree_iter_t *ivtree_intersect(ivtree_t *tree, int64_t lkey, int64_t rkey);
```

#### ivtree\_next

Get the next element from the iterater.

```
ivtree_node_t *ivtree_next(ivtree_iter_t *iter);
```

#### ivtree\_iter\_clean

Destroy an iterator.

```
void ivtree_iter_clean(ivtree_iter_t *iter);
```

#### ivtree\_update\_node

Update the tree, must be called when `rkey` element is modified.

```
void ivtree_update_node(ivtree_t *tree, ivtree_node_t *node);
```

#### ivtree\_walk

Apply a function (`rbtree_walk_t`) to nodes in a leaf-to-root order.

```
typedef void (*ivtree_walk_t)(ivtree_node_t *node, void *ctx);
void ivtree_walk(ivtree_t *tree, ivtree_walk_t fn, void *ctx);
```

## License

MIT