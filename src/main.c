#include <ternedit.h>

int main() {
    TerneditState state;
    TerneditInit(&state);
    int ec = TerneditRun(&state);
    TerneditFree(&state);
    return ec;
}
