// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

#include "lookuptree.h"
#include "reb_mem.h"

void lookuptree_init(lookuptree_t * tree) {
    tree->entry = 0;
    for (int i = 0; i < 4; i++)
        tree->subtrees[i] = 0;
}

static lookuptree_t * lookuptree_navigate_du(lookuptree_t * tree, char c, int expand) {
    if (!(tree->subtrees[c])) {
        if (!expand)
            return 0;
        lookuptree_t * newnode = reb_mem_alloc(sizeof(lookuptree_t));
        if (!newnode)
            return 0;
        lookuptree_init(newnode);
        tree->subtrees[c] = newnode;
    }
    return tree->subtrees[c];
}

lookuptree_t * lookuptree_navigate(lookuptree_t * tree, char c, int expand) {
    tree = lookuptree_navigate_du(tree, (c & 0xC0) >> 6, expand);
    if (!tree)
        return 0;
    tree = lookuptree_navigate_du(tree, (c & 0x30) >> 4, expand);
    if (!tree)
        return 0;
    tree = lookuptree_navigate_du(tree, (c & 0x0C) >> 2, expand);
    if (!tree)
        return 0;
    tree = lookuptree_navigate_du(tree, c & 0x03, expand);
    return tree;
}

// Checks if a node should be freed, and frees it if so.
// if freethis == 0, then it never frees root.
// Returns 1 if this node was freed, else 0.
int lookuptree_prune(lookuptree_t * tree, int freethis) {
    
    // Do not free any tree with an entry in it.
    // (This tree remaining causes the parents to remain, etcetc.)
    if (tree->entry)
        freethis = 0;

    for (int i = 0; i < 4; i++) {
        if (tree->subtrees[i])
            if (lookuptree_prune(tree->subtrees[i], 1)) {
                tree->subtrees[i] = 0;
            } else {
                // Do not free any tree which has subtrees that refuse to free themselves.
                freethis = 0;
            }
    }
    if (freethis) {
        reb_mem_free(tree);
        return 1;
    }
    // There are still subtrees (which didn't get pruned because somewhere was an entry),
    // or this tree itself holds an entry.
    return 0;
}

// Clears the tree, but doesn't deallocate the tree root.
// Note that freeentr is only called on non-zero entries.
void lookuptree_clear(lookuptree_t * tree, void (*freeentr)(void * entry)) {
    
    // Clear entry
    
    if (tree->entry) {
        freeentr(tree->entry);
        tree->entry = 0;
    }
    
    // Clear subtrees
    
    for (int i = 0; i < 4; i++) {
        if (tree->subtrees[i]) {
            lookuptree_clear(tree->subtrees[i], freeentr);
            reb_mem_free(tree->subtrees[i]);
        }
    }
}
