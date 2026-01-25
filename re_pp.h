#include <stdbool.h>  // For bool type
#include <stddef.h>   // For size_t

#define MAX_REG 256

// Enum for character classification
enum CharClass {
    DIGIT,
    UPPER,
    LOWER,
    OTHER
};

// Function to classify a character
enum CharClass classify(unsigned char c);

// Function to expand a regex pattern
char* regex_prep(char* raw_reg);
