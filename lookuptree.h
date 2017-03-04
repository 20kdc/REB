// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

// A tree for looking up arbitrary byte strings.
// It's laid out in a quadtree.
// -- EDIT AND REPLACEMENT OF LONG SPEECH --
// Memory usage optimization logic is that subtrees should contain 4 elements.
// Or maybe even 2, but then the overhead cuts in.
// Turns out my 136 byte mark was right.
// Too right - there's not enough memory for: a = "Hello " on lower heap sizes.
#ifndef LOOKUPTREE_H
#define LOOKUPTREE_H

typedef struct {
    void * subtrees[4];
    // Note: Usually inaccessible on every second (of two) nodes down.
    void * entry;
} lookuptree_t;

// Initializes a lookuptree_t. (Yes, static allocation is in mind)
void lookuptree_init(lookuptree_t * tree);

// NOTE: This creates nodes as you move.
// OTHER NOTE: This is indexed by bytes despite being a "nibble-based" tree,
//             since bytes are what people actually use.
lookuptree_t * lookuptree_navigate(lookuptree_t * tree, char c, int expand);

// Checks if a node should be freed, and frees it if so.
// if freethis == 0, then it never frees root.
// Returns 1 if this node was freed, else 0.
int lookuptree_prune(lookuptree_t * tree, int freethis);

// Clears the tree, but doesn't deallocate the tree root.
// Note that freeentr is only called on non-zero entries.
void lookuptree_clear(lookuptree_t * tree, void (*freeentr)(void * entry));
#endif

