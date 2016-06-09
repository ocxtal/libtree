# libtree

Libtree is an wrapper of red-black-tree implementation in the [nginx](https://nginx.org/) core library. It provides node allocation and automatic cleanup in addition to the original library, with simpler interface.

## Functions


### tree\_init

Initialize a red-black-tree object.

```
tree_t *tree_init(uint64_t object_size, tree_params_t const *params);
```


###  tree\_clean

Destroy the tree.

```
void tree_clean(tree_t *tree);
```

### tree\_create\_node

Create a new node. Note that the created node object is not yet inserted in the tree. The object is automatically freed when `tree_remove` or `tree_clean` is called.

```
tree_node_t *tree_create_node(tree_t *tree);
```

### tree\_insert

Insert a node to the tree.

```
void tree_insert(tree_t *tree, tree_node_t *node);
```

### tree\_remove

Remove a node, automatically freed if malloc'd with `tree_create_node`.

```
void tree_remove(tree_t *tree, tree_node_t *node);
```

### tree\_search\_key

Search a node by key, returning the leftmost node.

```
tree_node_t *tree_search_key(tree_t *tree, int64_t key);
```

### tree\_search\_key\_left

Search a node by key. Returns the nearest node in the left half of the tree if key was not found.

```
tree_node_t *tree_search_key_left(tree_t *tree, int64_t key);
```

### tree\_search\_key\_right

Search a node by key. Returns the nearest node in the right half of the tree if key was not found.

```
tree_node_t *tree_search_key_right(tree_t *tree, int64_t key);
```

### tree\_left

Returns the left next node.

```
tree_node_t *tree_left(tree_t *tree, tree_node_t const *node);
```

### tree\_right

Returns the right next node

```
tree_node_t *tree_right(tree_t *tree, tree_node_t const *node);
```


## License

MIT