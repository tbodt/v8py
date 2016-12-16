// Meme header file. Every project should have one of these. Kappa Kappa Kappa
#ifndef KAPPA_KAPPA_KAPPA
#define KAPPA_KAPPA_KAPPA

// I CAN HAZ HEADER FILES
#include <Python.h> // Kappa
#include <v8.h> // Kappa
using/*Kappa*/namespace/*Kappa*/v8;
// KTHXBYE

/*  ▄▀▀▀▀▀█▀▄▄▄▄          ▄▀▀▀▀▀█▀▄▄▄▄          ▄▀▀▀▀▀█▀▄▄▄▄          ▄▀▀▀▀▀█▀▄▄▄▄    
  ▄▀▒▓▒▓▓▒▓▒▒▓▒▓▀▄      ▄▀▒▓▒▓▓▒▓▒▒▓▒▓▀▄      ▄▀▒▓▒▓▓▒▓▒▒▓▒▓▀▄      ▄▀▒▓▒▓▓▒▓▒▒▓▒▓▀▄  
▄▀▒▒▓▒▓▒▒▓▒▓▒▓▓▒▒▓█   ▄▀▒▒▓▒▓▒▒▓▒▓▒▓▓▒▒▓█   ▄▀▒▒▓▒▓▒▒▓▒▓▒▓▓▒▒▓█   ▄▀▒▒▓▒▓▒▒▓▒▓▒▓▓▒▒▓█ 
█▓▒▓▒▓▒▓▓▓░░░░░░▓▓█   █▓▒▓▒▓▒▓▓▓░░░░░░▓▓█   █▓▒▓▒▓▒▓▓▓░░░░░░▓▓█   █▓▒▓▒▓▒▓▓▓░░░░░░▓▓█ 
█▓▓▓▓▓▒▓▒░░░░░░░░▓█   █▓▓▓▓▓▒▓▒░░░░░░░░▓█   █▓▓▓▓▓▒▓▒░░░░░░░░▓█   █▓▓▓▓▓▒▓▒░░░░░░░░▓█ 
▓▓▓▓▓▒░░░░░░░░░░░░█   ▓▓▓▓▓▒░░░░░░░░░░░░█   ▓▓▓▓▓▒░░░░░░░░░░░░█   ▓▓▓▓▓▒░░░░░░░░░░░░█ 
▓▓▓▓░░░░▄▄▄▄░░░▄█▄▀   ▓▓▓▓░░░░▄▄▄▄░░░▄█▄▀   ▓▓▓▓░░░░▄▄▄▄░░░▄█▄▀   ▓▓▓▓░░░░▄▄▄▄░░░▄█▄▀ 
░▀▄▓░░▒▀▓▓▒▒░░█▓▒▒░   ░▀▄▓░░▒▀▓▓▒▒░░█▓▒▒░   ░▀▄▓░░▒▀▓▓▒▒░░█▓▒▒░   ░▀▄▓░░▒▀▓▓▒▒░░█▓▒▒░ 
▀▄░░░░░░░░░░░░▀▄▒▒█   ▀▄░░░░░░░░░░░░▀▄▒▒█   ▀▄░░░░░░░░░░░░▀▄▒▒█   ▀▄░░░░░░░░░░░░▀▄▒▒█ 
 ▀░▀░░░░░▒▒▀▄▄▒▀▒▒█    ▀░▀░░░░░▒▒▀▄▄▒▀▒▒█    ▀░▀░░░░░▒▒▀▄▄▒▀▒▒█    ▀░▀░░░░░▒▒▀▄▄▒▀▒▒█ 
  ▀░░░░░░▒▄▄▒▄▄▄▒▒█     ▀░░░░░░▒▄▄▒▄▄▄▒▒█     ▀░░░░░░▒▄▄▒▄▄▄▒▒█     ▀░░░░░░▒▄▄▒▄▄▄▒▒█ 
   ▀▄▄▒▒░░░░▀▀▒▒▄▀       ▀▄▄▒▒░░░░▀▀▒▒▄▀       ▀▄▄▒▒░░░░▀▀▒▒▄▀       ▀▄▄▒▒░░░░▀▀▒▒▄▀  
     ▀█▄▒▒░░░░▒▄▀          ▀█▄▒▒░░░░▒▄▀          ▀█▄▒▒░░░░▒▄▀          ▀█▄▒▒░░░░▒▄▀   
        ▀▀█▄▄▄▄▀              ▀▀█▄▄▄▄▀              ▀▀█▄▄▄▄▀              ▀▀█▄▄▄▄▀ */  

#define MAGIC_CONSTANT_STRING_LIST_KAPPA(V) \
/*Kappa*/V(I_CAN_HAZ_ERROR_PROTOTYPE, "Error Prototype Will Appear Here FeelsGoodMan Kappa") \
/*Kappa*/V(IZ_DAT_OBJECT, "A wild object appeared! Kappa") \
/*Kappa*/V(IZ_DAT_DICTINARY, "A wild dictionary appeared! Kappa") \
    // really need more of these Kappa Kappa

#define DECLARE_MAGIC(name, string) extern Persistent<String> name##p;
MAGIC_CONSTANT_STRING_LIST_KAPPA(DECLARE_MAGIC)
#undef DECLARE_MAGIC // Kappa
// You can't go so far as to define a macro in a macro so Kappa
#define I_CAN_HAZ_ERROR_PROTOTYPE I_CAN_HAZ_ERROR_PROTOTYPEp.Get(isolate)
#define IZ_DAT_OBJECT IZ_DAT_OBJECTp.Get(isolate)
#define IZ_DAT_DICTINARY IZ_DAT_DICTINARYp.Get(isolate)

// boring function prototype Kappa
void create_memes_plz_thx();
// Kappa 

// They do nothing for us Kappa
// So we call them interns Kappa
#define KAPPA(name, value) \
    static PyObject *name = PyString_InternFromString(value)
#define KAPPA_PRIDE(name) KAPPA(name, #name)
KAPPA_PRIDE(__dict__);
// Kappa

// Kappa Kappa Kappa Kappa Kappa Kappa Kappa Kappa Kappa Kappa Kappa Kappa Kappa
#endif // Kappa Kappa Kappa Kappa Kappa Kappa Kappa Kappa Kappa Kappa Kappa
// Kappa Kappa Kappa Kappa Kappa Kappa Kappa Kappa Kappa Kappa Kappa Kappa Kappa
