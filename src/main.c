#include <ternedit.h>

int main() {
    TerneditState state;
    ternedit_init(&state);
    int ec = ternedit_run(&state);
    ternedit_free(&state);
    return ec;
}
