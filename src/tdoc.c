#include <tdoc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static TernDocNode* NewNode(TernDocPiece piece) {
    TernDocNode* node = (TernDocNode*)calloc(1, sizeof(TernDocNode));
    if (!node) return NULL;
    node->piece = piece;
    node->height = 1;
    node->subtreeSize = piece.length;
    return node;
}

static void NodeFree(TernDocNode* node) {
    if (!node) return;
    NodeFree(node->left);
    NodeFree(node->right);
    free(node);
}

static TernDocNode* NodeCopy(TernDocNode* node) {
    if (!node) return NULL;
    TernDocNode* copy = (TernDocNode*)malloc(sizeof(TernDocNode));
    if (!copy) return NULL;
    memcpy(copy, node, sizeof(TernDocNode));
    copy->left = NodeCopy(node->left);
    copy->right = NodeCopy(node->right);
    return copy;
}

static int NodeHeight(TernDocNode* n) {
    return n ? n->height : 0;
}

static size_t NodeSize(TernDocNode* n) {
    return n ? n->subtreeSize : 0;
}

static void NodeUpdate(TernDocNode* n) {
    if (!n) return;
    int hl = NodeHeight(n->left);
    int hr = NodeHeight(n->right);
    n->height = (hl > hr ? hl : hr) + 1;
    n->subtreeSize = NodeSize(n->left) + NodeSize(n->right) + n->piece.length;
}

static void NodeSlice(const TernDocument* doc, const TernDocNode* n, size_t pos, size_t len, TritState* out, size_t* written) {
    if (!n || len == 0) return;

    size_t leftSize = NodeSize(n->left);

    if (pos < leftSize) {
        size_t available = leftSize - pos;
        size_t toRead = (len < available) ? len : available;
        NodeSlice(doc, n->left, pos, toRead, out, written);
        len -= toRead;
        pos = 0;
    } else {
        pos -= leftSize;
    }

    if (len == 0) return;

    if (pos < n->piece.length) {
        size_t available = n->piece.length - pos;
        size_t toRead = (len < available) ? len : available;
        TritState* buf = (n->piece.buffer == TERNDOC_BUF_ORIGINAL) ? doc->original : doc->add;
        memcpy(out + *written, buf + n->piece.start + pos, toRead);
        *written += toRead;
        len -= toRead;
        pos = 0;
    } else {
        pos -= n->piece.length;
    }

    if (len == 0) return;

    NodeSlice(doc, n->right, pos, len, out, written);
}

static TernDocNode* RotateRight(TernDocNode* y) {
    TernDocNode* x = y->left;
    TernDocNode* T2 = x->right;
    x->right = y;
    y->left = T2;
    NodeUpdate(y);
    NodeUpdate(x);
    return x;
}

static TernDocNode* RotateLeft(TernDocNode* x) {
    TernDocNode* y = x->right;
    TernDocNode* T2 = y->left;
    y->left = x;
    x->right = T2;
    NodeUpdate(x);
    NodeUpdate(y);
    return y;
}

static int GetBalance(TernDocNode* n) {
    return n ? NodeHeight(n->left) - NodeHeight(n->right) : 0;
}

static TernDocNode* Rebalance(TernDocNode* n) {
    if (!n) return NULL;
    NodeUpdate(n);
    int balance = GetBalance(n);
    if (balance > 1) {
        if (GetBalance(n->left) < 0) {
            n->left = RotateLeft(n->left);
        }
        return RotateRight(n);
    }
    if (balance < -1) {
        if (GetBalance(n->right) > 0) {
            n->right = RotateRight(n->right);
        }
        return RotateLeft(n);
    }
    return n;
}

static TernDocNode* Merge(TernDocNode* l, TernDocNode* r) {
    if (!l) return r;
    if (!r) return l;
    if (NodeHeight(l) > NodeHeight(r)) {
        l->right = Merge(l->right, r);
        return Rebalance(l);
    } else {
        r->left = Merge(l, r->left);
        return Rebalance(r);
    }
}

typedef struct {
    TernDocNode* left;
    TernDocNode* right;
} NodePair;

static NodePair Split(TernDocNode* n, size_t k) {
    if (!n) return (NodePair) {NULL, NULL};

    size_t leftSize = NodeSize(n->left);
    if (k <= leftSize) {
        NodePair res = Split(n->left, k);
        n->left = res.right;
        return (NodePair) {res.left, Rebalance(n)};
    } else if (k >= leftSize + n->piece.length) {
        NodePair res = Split(n->right, k - leftSize - n->piece.length);
        n->right = res.left;
        return (NodePair) {Rebalance(n), res.right};
    } else {
        size_t offsetInPiece = k - leftSize;
        TernDocNode* leftPart = NewNode((TernDocPiece) {
            .buffer = n->piece.buffer,
            .start = n->piece.start,
            .length = offsetInPiece
        });
        TernDocNode* rightPart = NewNode((TernDocPiece) {
            .buffer = n->piece.buffer,
            .start = n->piece.start + offsetInPiece,
            .length = n->piece.length - offsetInPiece
        });

        TernDocNode* L = Merge(n->left, leftPart);
        TernDocNode* R = Merge(rightPart, n->right);

        n->left = NULL;
        n->right = NULL;
        NodeFree(n);

        return (NodePair) {L, R};
    }
}

// --- Undo/Redo Management ---

static void OpStackClear(TernDocOpStack* stack) {
    for (size_t i = 0; i < stack->size; ++i) {
        NodeFree(stack->items[i].savedRoot);
    }
    free(stack->items);
    stack->items = NULL;
    stack->size = 0;
    stack->capacity = 0;
}

static void OpStackPush(TernDocOpStack* stack, TernDocOp op) {
    if (stack->size >= stack->capacity) {
        stack->capacity = (stack->capacity == 0) ? 8 : stack->capacity * 2;
        stack->items = (TernDocOp*)realloc(stack->items, stack->capacity * sizeof(TernDocOp));
    }
    stack->items[stack->size++] = op;
}

static void SaveUndo(TernDocument* doc) {
    TernDocOp op = {
        .type = TERNDOC_OP_REPLACE,
        .savedRoot = NodeCopy(doc->root)
    };
    OpStackPush(&doc->undoStack, op);
    OpStackClear(&doc->redoStack);
}

static TritState NodeAt(const TernDocument* doc, const TernDocNode* n, size_t pos) {
    if (!n) return 0;
    size_t leftSize = NodeSize(n->left);
    if (pos < leftSize) {
        return NodeAt(doc, n->left, pos);
    } else if (pos < leftSize + n->piece.length) {
        size_t offset = pos - leftSize;
        TritState* buf = (n->piece.buffer == TERNDOC_BUF_ORIGINAL) ? doc->original : doc->add;
        return buf[n->piece.start + offset];
    } else {
        return NodeAt(doc, n->right, pos - leftSize - n->piece.length);
    }
}



TernDocument* TernDocNew(const TritState* initial, size_t size) {
    TernDocument* doc = (TernDocument*)calloc(1, sizeof(TernDocument));
    if (!doc) return NULL;

    if (initial && size > 0) {
        doc->original = (TritState*)malloc(size);
        memcpy(doc->original, initial, size);
        doc->originalSize = size;
        doc->root = NewNode((TernDocPiece) { TERNDOC_BUF_ORIGINAL, 0, size });
    }

    doc->addCapacity = 1024;
    doc->add = (TritState*)malloc(doc->addCapacity);
    doc->addSize = 0;

    return doc;
}

void TernDocFree(TernDocument* doc) {
    if (!doc) return;
    NodeFree(doc->root);
    free(doc->original);
    free(doc->add);
    OpStackClear(&doc->undoStack);
    OpStackClear(&doc->redoStack);
    free(doc);
}

static void AddBufEnsure(TernDocument* doc, size_t needed) {
    if (doc->addSize + needed > doc->addCapacity) {
        while (doc->addSize + needed > doc->addCapacity) {
            doc->addCapacity *= 2;
        }
        doc->add = (TritState*)realloc(doc->add, doc->addCapacity);
    }
}

void TernDocInsert(TernDocument* doc, size_t pos, StringView trits) {
    if (trits.len == 0) return;
    SaveUndo(doc);

    AddBufEnsure(doc, trits.len);
    size_t startInAdd = doc->addSize;
    memcpy(doc->add + doc->addSize, trits.data, trits.len);
    doc->addSize += trits.len;

    NodePair pair = Split(doc->root, pos);
    TernDocNode* newNode = NewNode((TernDocPiece) {TERNDOC_BUF_ADD, startInAdd, trits.len});
    doc->root = Merge(pair.left, Merge(newNode, pair.right));
}

void TernDocDelete(TernDocument* doc, size_t pos, size_t len) {
    if (len == 0) return;
    size_t docSize = TernDocSize(doc);
    if (pos >= docSize) return;
    if (pos + len > docSize) len = docSize - pos;

    SaveUndo(doc);

    NodePair pair1 = Split(doc->root, pos);
    NodePair pair2 = Split(pair1.right, len);
    
    NodeFree(pair2.left);
    doc->root = Merge(pair1.left, pair2.right);
}

void TernDocReplace(TernDocument* doc, size_t pos, size_t len, StringView trits) {
    size_t docSize = TernDocSize(doc);
    if (pos > docSize) pos = docSize;
    if (pos + len > docSize) len = docSize - pos;
    
    if (len == 0 && trits.len == 0) return;
    SaveUndo(doc);

    NodePair pair1 = Split(doc->root, pos);
    NodePair pair2 = Split(pair1.right, len);
    NodeFree(pair2.left);

    TernDocNode* newNode = NULL;
    if (trits.len > 0) {
        AddBufEnsure(doc, trits.len);
        size_t startInAdd = doc->addSize;
        memcpy(doc->add + doc->addSize, trits.data, trits.len);
        doc->addSize += trits.len;
        newNode = NewNode((TernDocPiece) { TERNDOC_BUF_ADD, startInAdd, trits.len });
    }

    doc->root = Merge(pair1.left, Merge(newNode, pair2.right));
}

size_t TernDocSize(const TernDocument* doc) {
    return NodeSize(doc->root);
}

TritState TernDocAt(const TernDocument* doc, size_t pos) {
    if (pos >= TernDocSize(doc)) return 0;
    return NodeAt(doc, doc->root, pos);
}

void TernDocSlice(const TernDocument* doc, size_t pos, size_t len, TritState* out) {
    size_t totalSize = TernDocSize(doc);
    if (pos >= totalSize) return;
    if (pos + len > totalSize) len = totalSize - pos;
    size_t written = 0;
    NodeSlice(doc, doc->root, pos, len, out, &written);
}

void TernDocUndo(TernDocument* doc) {
    if (doc->undoStack.size == 0) return;

    TernDocOp op = doc->undoStack.items[--doc->undoStack.size];
    
    TernDocOp redoOp = {
        .type = TERNDOC_OP_REPLACE,
        .savedRoot = doc->root
    };
    OpStackPush(&doc->redoStack, redoOp);

    doc->root = op.savedRoot;
}

void TernDocRedo(TernDocument* doc) {
    if (doc->redoStack.size == 0) return;

    TernDocOp op = doc->redoStack.items[--doc->redoStack.size];

    TernDocOp undoOp = {
        .type = TERNDOC_OP_REPLACE,
        .savedRoot = doc->root
    };
    OpStackPush(&doc->undoStack, undoOp);

    doc->root = op.savedRoot;
}

void TernDocSave(const TernDocument* doc, const char* path) {
    FILE* f = fopen(path, "wb");
    if (!f) return;

    size_t totalTrits = TernDocSize(doc);
    size_t tryteCount = (totalTrits + 5) / 6;

    fwrite("T48B", 1, 4, f);

    for (size_t i = 0; i < tryteCount; ++i) {
        uint16_t tryte = 0;
        uint16_t p3 = 243;
        for (int j = 0; j < 6; ++j) {
            size_t pos = i * 6 + j;
            if (pos < totalTrits) {
                tryte += (uint16_t)TernDocAt(doc, pos) * p3;
            }
            p3 /= 3;
        }
        fwrite(&tryte, 2, 1, f);
    }

    fclose(f);
}
