/* File : engine.i */
%module audiox
%include "std_vector.i"

/* Anything in the following section is added verbatim to the .cxx wrapper file */
%{
#include "player.h"
%}

%typemap(jstype) std::string& OUTPUT "String[]"
%typemap(jtype) std::string& OUTPUT "String[]"
%typemap(jni) std::string& OUTPUT "jobjectArray"
%typemap(javain)  std::string& OUTPUT "$javainput"
%typemap(in) std::string& OUTPUT (std::string temp) {
  if (!$input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "array null");
    return $null;
  }
  if (JCALL1(GetArrayLength, jenv, $input) == 0) {
    SWIG_JavaThrowException(jenv, SWIG_JavaIndexOutOfBoundsException, "Array must contain at least 1 element");
  }
  $1 = &temp;
}
%typemap(argout) std::string& OUTPUT {
  jstring jvalue = JCALL1(NewStringUTF, jenv, temp$argnum.c_str()); 
  JCALL3(SetObjectArrayElement, jenv, $input, 0, jvalue);
}

%apply  std::string& OUTPUT { std::string & returnValue }
%apply  std::string& OUTPUT { std::string & returnType }
%apply const std::string& {std::string* foo};

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
