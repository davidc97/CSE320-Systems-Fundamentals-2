#include "hw1.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the program
 * and will return a unsigned short (2 bytes) that will contain the
 * information necessary for the proper execution of the program.
 *
 * IF -p is given but no (-r) ROWS or (-c) COLUMNS are specified this function
 * MUST set the lower bits to the default value of 10. If one or the other
 * (rows/columns) is specified then you MUST keep that value rather than assigning the default.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return Refer to homework document for the return value of this function.
 */
unsigned short validargs(int argc, char **argv) {
    unsigned short mode = 0x0000;
    //mode >> 15;
    char** argvPtr = argv;
    char *x = *(argvPtr);
    int i = 0;
    while(isEqual(x, "bin/hw1") != 1 && (x != '\0')){
        argvPtr++;
        x = *(argvPtr);
        i++;
    }
    argvPtr++;
    x = *(argvPtr);
    i++;
    if(x == '\0'){
        return 0;
    }
    if(isEqual(x,"-h") == 1){
        mode |= 1 << 15;
        return mode;
    }
    if(isEqual(x, "-p") ==1){
        mode |= 0 << 15;
        argvPtr++;
        x = *(argvPtr);
        i++;
        if(x == '\0'){
            return 0;
        }
        if(isEqual(x, "-e") == 1){
            mode |= 0 << 15;
            argvPtr++;
            x = *(argvPtr);
            i++;
        }
        else if(isEqual(x, "-d") == 1){
            argvPtr++;
            x = *(argvPtr);
            i++;
            mode |= 1 << 13;
        }
        int argsleft = argc - i;
        if (checkKey(argsleft, argvPtr, 0) != 1){
            return 0;
        }
        int r = checkRows(argsleft, argvPtr);
        int c = checkCols(argsleft, argvPtr);
        if(r == 0 || c == 0){
            return 0;
        }
        mode = (mode| (r << 4));
        mode = (mode| c);
        return mode;
    }
    if(isEqual(x, "-f") == 1){
        mode |= 1 << 14;
        argvPtr++;
        x = *(argvPtr);
        i++;
        if(x == '\0'){
            return 0;
        }
        if(isEqual(x, "-e") == 1){
            argvPtr++;
            x = *(argvPtr);
            i++;
        }
        else if(isEqual(x, "-d") == 1){
            argvPtr++;
            x = *(argvPtr);
            i++;
            mode |= 1 << 13;
        }
        int argsleft = argc - i;
        if (checkKey(argsleft, argvPtr, 1) != 1){
            return 0;
        }
        return mode;
    }
    return mode;
}

// string comparison function
// returns -1 if not equal, 1 if equal
int isEqual(char *str1, char *str2){
    int i = 0;
    while(*(str1 + i) == *(str2 + i)){
        if(*(str1 + i) == '\0'){
            return 1;
        }
        i++;
    }
    return -1;
}
int validPKey(char *k){
    char *polyPtr;
    char *keyPtr;
    int i = 0;
    for(keyPtr = k; *keyPtr != '\0'; keyPtr++){
        i++;
        for(polyPtr = polybius_alphabet; *polyPtr != '\0'; polyPtr++){
            if(*keyPtr == *polyPtr){
            i--;
            break;
        }
    }
        if(i != 0){
            return 0;
        }
    }
key = k;
return 1;
}
int validFKey(char *k){
    char *fmPtr;
    char *keyPtr;

    int i = 0;
    for(keyPtr = k; *keyPtr != '\0'; keyPtr++){
        i++;
        for(fmPtr = (char*) fm_alphabet; *fmPtr != '\0'; fmPtr++){
            if(*keyPtr == *fmPtr){
            i--;
            break;
        }
    }
        if(i != 0){
            return 0;
        }
    }
key = k;
return 1;
}
// 0 = p, 1 = f
int checkKey(int argc, char **argv, int forP){
    int kflag = 0;
    int i = 0;
    char *str = *(argv);
    while(i < argc){
        if(kflag == 0 && isEqual(str, "-k") == 1){
            char *k = *(argv + i + 1);
            kflag = 1;
            if(forP == 0){
                if(validPKey(k) == 0){
                return 0;
            }
        }
            if(validFKey(k) == 0){
                return 0;
            }
            return 1;
        }
        if(kflag == 1 && isEqual(str, "-k") == 1){
            return 0;
        }
        i++;
        str =*(argv + i);
    }

    return 1;
}

 int checkRows(int argc, char **argv){
    int rflag = 0;
    int i = 0;
    char *str = *(argv);
    while(i < argc){
        if(rflag == 0 && isEqual(str, "-r") == 1){
            char* rStr = *(argv + i + 1);
            int r = strToInt(rStr);
            rflag = 1;
            if(r < 16 && r >= 9){
                return r;
            } else {
                return 0;
            }
        }
        if(rflag == 1 && isEqual(str, "-r") == 1){
            return 0;
        }
        i++;
        str = *(argv + i);
    }
    return 10;
 }
  int checkCols(int argc, char **argv){
    int cflag = 0;
    int i = 0;
    char *str = *(argv);
    while(i < argc){
        if(cflag == 0 && isEqual(str, "-c") == 1){
            char* cStr = *(argv + i + 1);
            int c = strToInt(cStr);
            cflag = 1;
            if(c < 16 && c >= 9){
                return c;
            } else {
                return 0;
            }
        }
        if(cflag == 1 && isEqual(str, "-c") == 1){
            return 0;
        }
        i++;
        str = *(argv + i);
    }
    return 10;
 }

 int strToInt(char *str){
    char *strPtr;
    strPtr = str;
    int t = 0;
    int o = 0;
    int r = 0;
    t = *strPtr - '0';
    strPtr++;
    if(*strPtr == '\0'){
        return t;
    }
    o = *strPtr -'0';
    r = (t * 10) + o;
    return r;
 }