#include <string.h>

class String {
    private:
        int length;
        int bodySize;
        char* body;

    public:
        String() {
            length = 0;
            bodySize = 0;
            body = NULL;
        }

        String(String other) {
            this.length = other.length;
            this.bodySize = int(other.length*1.5) + 1;
            this.body = new char[this.bodySize];
            int i = 0;
            for (; other.body[i] != NULL; i++) {
                this.body[i] = other.body[i];
            }
            this.body[i] = NULL;
        }
}