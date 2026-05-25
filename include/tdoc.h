#ifndef TDOC_H
#define TDOC_H

#include <stddef.h>
#include <stdint.h>
#include <tritstate.h>
#include <sv.h>

typedef enum {
    TERNDOC_BUF_ORIGINAL = 0,
    TERNDOC_BUF_ADD      = 1
} TernDocBufKind;

typedef struct {
    TernDocBufKind buffer;
    size_t start;
    size_t length;
} TernDocPiece;

typedef struct TernDocNode {
    struct TernDocNode *left;
    struct TernDocNode *right;

    int height;
    size_t subtreeSize;

    TernDocPiece piece;
} TernDocNode;

typedef enum {
    TERNDOC_OP_INSERT,
    TERNDOC_OP_REPLACE,
    TERNDOC_OP_DELETE,
} TernDocOpType;

typedef struct {
    TernDocOpType type;
    size_t pos;
    size_t length;
    TernDocNode* savedRoot;
} TernDocOp;

typedef struct {
    TernDocOp* items;
    size_t size;
    size_t capacity;
} TernDocOpStack;

typedef struct {
    TernDocNode* root;

    TritState* original;
    size_t originalSize;

    TritState* add;
    size_t addSize;
    size_t addCapacity;

    TernDocOpStack undoStack;
    TernDocOpStack redoStack;
} TernDocument;

TernDocument* TernDocNew(const TritState* initial, size_t size);
void TernDocFree(TernDocument* doc);

void TernDocInsert(TernDocument* doc, size_t pos, StringView trits);
void TernDocDelete(TernDocument* doc, size_t pos, size_t len);
void TernDocReplace(TernDocument* doc, size_t pos, size_t len, StringView trits);

TritState TernDocAt(const TernDocument* doc, size_t pos);
void TernDocSlice(const TernDocument* doc, size_t pos, size_t len, TritState* out);
size_t TernDocSize(const TernDocument* doc);

void TernDocSave(const TernDocument* doc, const char* path);

void TernDocUndo(TernDocument* doc);
void TernDocRedo(TernDocument* doc);

#endif // TDOC_H
