/* File : engine.i */
%module audiox

/* Anything in the following section is added verbatim to the .cxx wrapper file */
%{
#include "player.h"
%}

// Enable the JNI class to load the required native library.
%pragma(java) jniclasscode=%{
    static {
        try {
            java.lang.System.loadLibrary("player");
        } catch (UnsatisfiedLinkError e) {
            java.lang.System.err.println("native code library failed to load.\n" + e);
            java.lang.System.exit(1);
        }
    }
%}

%include "player.h"
