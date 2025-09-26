#include <iostream>
#include <cstddef>
#include <cassert>
#include <stdexcept>

using namespace std;

/*
SmallString:
A class where small strings are optimized.
*/

const size_t BUFFER_LIMIT = 22;
const size_t FALLBACK_INITIAL_CAP = 10;


class SmallString {

  // A helper class for the dynamically allocated fallback;
  // it's just a vector.
  struct Fallback {
    char* fallback;
    size_t size;
    size_t capacity;
    
    // Initializes the fallback with a default capacity.
    Fallback() {
      // If anything happens, we must make sure this is destroyed!
      fallback = new char[FALLBACK_INITIAL_CAP];
      size = 0;
      capacity = FALLBACK_INITIAL_CAP;
    }

    ~Fallback() noexcept {
      // We must delete the allocated chars manually since fallback is
      // a raw pointer.
      delete[] fallback;
      fallback = nullptr;
    }

    // Initializes the fallback with one character.
    Fallback(const char* c) {
      if (FALLBACK_INITIAL_CAP == 0) {
        throw std::out_of_range("Fallback initial capacity is non-positive.");
      }

      fallback = new char[FALLBACK_INITIAL_CAP];
      fallback[0] = *c;

      size = 1;
      capacity = FALLBACK_INITIAL_CAP;

    }

    static void copy_chars(size_t n, char* from, char* to) noexcept {
      for (std::size_t i = 0; i < n; ++i) {
        to[i] = from[i];
      }
    }

    // Handle with care: may throw!
    void double_capacity() {

      char* new_fallback = new char[capacity * 2];

      capacity *= 2;

      /*
      WRONG! If the following line were to throw — God forbid — we'd be left with the wrong capacity!
      char* new_fallback = new char[capacity]; // May throw std::bad_alloc in bad weather...
      */

      copy_chars(size, fallback, new_fallback);

      delete[] fallback;
      fallback = new_fallback; // The object now has ownership of the pointer, so we're safe.

    }

    // Given a pointer to a read-only char,
    // attempts to append at the end of the fallback.
    void append_char(const char* c) {

      if (size == capacity) {
        double_capacity();
      }

      fallback[size++] = *c;

    }

  };

  private:
  size_t _size;
  char _buffer[BUFFER_LIMIT];
  Fallback* _fb;

    // Given a pointer to a read-only char, 
    // appends it at the end of the string.
    void append_char(const char* c) {
      // If the buffer has been exhausted:
      if (_size == BUFFER_LIMIT) {

        _fb = new Fallback(c);
        ++_size;
      }
      else if (_size > BUFFER_LIMIT) {
        _fb->append_char(c);
        ++_size;
      }
      else {
        _buffer[_size++] = *c;
      }
    }

  public:
  // Default constructor: makes sure that _fb is nullptr (important)!
  SmallString() noexcept {
    _size = 0;
    _fb = nullptr;
  }

  // Destructor!
  ~SmallString() noexcept {
    delete _fb;
    _fb = nullptr;
  }

  const char& operator[](size_t i) const {

    if (i >= _size) {
      throw std::out_of_range("Index outside of the bounds!");
    }

    if (i < BUFFER_LIMIT) {
      return _buffer[i];
    }
    else {
      return _fb->fallback[i - BUFFER_LIMIT];
    }
  }

  // Empties the string.
  void empty() noexcept {
    delete _fb;
    _fb = nullptr;

    _size = 0;
  }

  // Appends the given literal at the end of the word.
  void append(const char* literal) {

    size_t length = strlen(literal);
    for (size_t i = 0; i < length; ++i) {
      append_char(literal + i);
    }

  }


  // To the constructor, we pass a pointer to the read-only literal.
  SmallString(const char* literal) : SmallString() {
    append(literal);
  }

  size_t length() const {
    return _size;
  }

  // Copy
  SmallString(const SmallString& other) : SmallString() {
    
    size_t length = other.length();
    char c;
    
    for (size_t i = 0; i < length; ++i) {
      c = other[i];
      append_char(&c);
    }

  }

  // Move
  SmallString(SmallString&& other) {
    _size = other._size;
    other._size = 0;

    Fallback::copy_chars(BUFFER_LIMIT, other._buffer, _buffer);
    _fb = other._fb;

    other._fb = nullptr;
  }

  /*
  Problem:
  At this point, if we were to do copy-assignment, such as:
  SmallString a("copy");
  SmallString b = a;
  Given that there's no copy-assignment operator, this would
  default to shallow-copy. This is dangerous, since now both
  b and a will have the pointer to the same Fallback (and you
  get double delete!)
  */

  // Copy-assignment operator

  SmallString& operator=(const SmallString& rhs) {

    empty();
    char c;

    size_t length = rhs.length();
    for (size_t i = 0; i < length; ++i) {
      c = rhs[i];
      append_char(&c);
    }

    return *this;

  }

  // Move-assignment operator (we must make sure that the
  // moved object is left in a graceful state!)

  SmallString& operator=(SmallString&& rhs) {

    _size = rhs._size;
    rhs._size = 0;

    Fallback::copy_chars(BUFFER_LIMIT, rhs._buffer, _buffer);
    _fb = rhs._fb;
    rhs._fb = nullptr;

    return *this;

  }

  friend SmallString operator+(SmallString, SmallString);

};

// Concatenation:
  // Which incidentally shows the need for move/copy
  // constructors.
  SmallString operator+(SmallString lhs, SmallString rhs) {

    size_t length = rhs.length();
    char c;
    
    for (size_t i = 0; i < length; ++i) {
      c = rhs[i];
      lhs.append_char(&c);
    }

    return lhs;
  }

// Equality for literals and SmallStrings
static bool operator==(const SmallString& lhs, const SmallString& rhs) {
  if (lhs.length() != rhs.length()) {
    return false;
  }

  size_t length = lhs.length();

  for (size_t i = 0; i < length; ++i) {
    if (lhs[i] != rhs[i]) {
      return false;
    }
  }

  return true;
}

static bool operator==(const SmallString& lhs, const char* rhs) {
  size_t length = lhs.length();

  for (size_t i = 0; i < length; ++i) {
    if (lhs[i] != rhs[i]) {
      return false;
    }
  }

  return true;
}

static bool operator==(const char* lhs, const SmallString& rhs) {
  return (rhs == lhs);
}

// TESTS

int main() {

  // Default constructor
  SmallString a;
  assert (a.length() == 0);

  // Indexing
  SmallString b1("small");
  assert (b1[3] == 'l');

  SmallString b2("abcdefghijklmnopqrstuvwxyz");
  assert (b2[25] == 'z');

  // Appending
  SmallString d1("Hello, ");
  SmallString d2("world!");

  d1.append("world!");

  SmallString d3(" Goodbye? Not really, it's just supposed to be long...");

  assert (d1 == "Hello, world!");

  // Concatenating
  assert (d1 + d2 == "Hello, world!world!");
  assert (!(d1 + d2 == "Hello, world!"));

  // Lengths
  SmallString e = b1 + b2;
  assert (e.length() == 31);

  // Equality
  SmallString newHello("Hello, world!");
  assert (d1 == newHello);
  assert ("Hello, world!" == d1);

  // Large strings
  SmallString large;
  for (size_t i = 0; i < 10000; ++i) {
    large.append("a");
  }

  assert(large.length() == 10000);

  // Copy assignment
  SmallString x("this will disappear");
  assert (x.length() == 19);

  SmallString y("and this will appear!");
  x = y;
  assert (x.length() == y.length());
  assert (y[2] == 'd');

  // Move assignment
  y = std::move(x);
  assert (y == "and this will appear!");
  assert (x.length() == 0); 

  return 0;

}