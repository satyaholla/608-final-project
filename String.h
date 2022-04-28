#include <string.h>
#include <iostream>
#include <stdexcept>

// to fix: make count const somehow, fix naming of length
class String {
    protected:
        int length;
        int bodySize;
        char* body;

    public:
        int getLength() { return this->length; }
        int capacity() { return this->bodySize; }
        char* rawCharArray() { return this->body; } // avoid using this function if possible

        void clear() { this->body[0] = '\0'; }

        void resize(int newSize) {
            if (newSize < 1) throw std::invalid_argument( "received value less than 1" );
            if (newSize < bodySize) this->body[newSize - 1] = '\0';
            char* newBody = new char[newSize];
            strcpy(newBody, this->body);
            delete[] this->body;
            this->body = newBody;
        }
        
        // String& substr(int pos = 0, int len = 1) {
        //     // todo
        // }

        int count(String&& sub) {
          int count = 0;
          for (int i = 0; i < this->length - sub.length + 1; i++) {
            bool matches = true;
            for (int j = 0; j < sub.length && matches; j++) {
              if (this->body[i + j] != sub[j]) matches = false;
            }
            count += matches;
          }
          return count;
        }

        String() {
            length = 0;
            bodySize = 0;
            body = new char[1];
            body[0] = '\0';
        }

        String(const String &other) {
            this->length = other.length;
            this->bodySize = int(other.length*1.5) + 1;
            this->body = new char[this->bodySize];
            strcpy(this->body, other.body);
        }
        
        String(const char* other) {
            this->length = strlen(other);
            this->bodySize = int(this->length*1.5) + 1;
            this->body = new char[this->bodySize];
            strcpy(this->body, other);
        }
        
        String& operator=(const String &other) {
            this->length = other.length;
            this->bodySize = int(other.length*1.5) + 1;
            this->body = new char[this->bodySize];
            strcpy(this->body, other.body);
            return *this;
        }
        
        String& operator=(const char* other) {
            this->length = strlen(other);
            this->bodySize = int(this->length*1.5) + 1;
            this->body = new char[this->bodySize];
            strcpy(this->body, other);
            return *this;
        }
        
        String& operator+(const String& other) {
            this->length += other.length;
            if (this->length > this->bodySize - 1) {
                this->bodySize = int(this->length*1.5) + 1;
                char* newBody = new char[this->bodySize];
                strcpy(newBody, this->body);
                this->body = newBody;
            }
            strcat(this->body, other.body);
            return *this;
        }
        
        String& operator+(const char* other) {
            this->length += strlen(other);
            if (this->length > this->bodySize - 1) {
                this->bodySize = int(this->length*1.5) + 1;
                char* newBody = new char[this->bodySize];
                strcpy(newBody, this->body);
                delete[] this->body;
                this->body = newBody;
            }
            strcat(this->body, other);
            return *this;
        }

        char& operator[](int i) {
            if (i >= this->length || i < 0) throw std::range_error("index out of range");
            return this->body[i];
        }

        ~String() {
            delete[] this->body;
        }
        
        friend std::ostream& operator<<(std::ostream& os, const String& s);
        friend std::istream& operator>>(std::istream& is, const String& s);
};

std::ostream& operator<<(std::ostream& os, const String& s) {
    os << s.body;
    return os;
}

std::istream& operator>>(std::istream& is, const String& s) {
    is >> s.body;
    return is;
}