#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "dynarray.h"
#include "re_pp.h"


enum CharClass classify(unsigned char c) {
    if (c >= '0' && c <= '9') return DIGIT;
    if (c >= 'A' && c <= 'Z') return UPPER;
    if (c >= 'a' && c <= 'z') return LOWER;
    return OTHER;
}

char* regex_prep(char* raw_reg){
    char* expanded_reg = dynarray_create(char);
    int raw_reg_len = strnlen(raw_reg, MAX_REG);
    bool should_expand = false;

    char or_sign = '|';
    for(int i = 0;i<raw_reg_len;i++){
        enum CharClass c1 = classify(raw_reg[i-1]);
        enum CharClass c2 = classify(raw_reg[i+1]);

        bool lookahead_expand = false;

        if((i+1)<raw_reg_len && raw_reg[i+1] == ']'){
            lookahead_expand = true;
        }

        if(raw_reg[i] == '/'){
            dynarray_push(expanded_reg, raw_reg[i]);
            dynarray_push(expanded_reg, raw_reg[i+1]);
            if(should_expand == true && ((i+2)<raw_reg_len && raw_reg[i+2] != ']')){
                dynarray_push(expanded_reg, or_sign);
            }
            i++;
            continue;
        }
        
        if(raw_reg[i] == ']'){
            should_expand = false;
        }
        
        if(should_expand == true){
            if(raw_reg[i] == '-'){
                assert((i-1)>=0);
                assert((i+1)<raw_reg_len);
                assert(c1 == c2);
                assert(raw_reg[i-1] < raw_reg[i+1]);
                assert(c1!=OTHER);
                for (unsigned char c = (unsigned char) raw_reg[i-1]+1; c < (unsigned char) raw_reg[i+1]; c++) {
                    char c_sign = (char) c;
                    dynarray_push(expanded_reg, c_sign);
                    if(lookahead_expand == false){
                        dynarray_push(expanded_reg, or_sign);
                    }
                    
                }
            }
            else{
                dynarray_push(expanded_reg, raw_reg[i]);
                if(lookahead_expand == false){
                    dynarray_push(expanded_reg, or_sign);
                }
            }
        }
        else{
            char add_char;
            if(raw_reg[i] == '['){
                add_char = '(';
            }
            else if(raw_reg[i] == ']'){
                add_char = ')';
            }
            else{
                add_char = raw_reg[i];
            }
            dynarray_push(expanded_reg, add_char);
        }

        if(raw_reg[i] == '['){
            should_expand = true;
        }
        
    }

    char null_char = '\0';
    dynarray_push(expanded_reg, null_char);
    return expanded_reg;
}