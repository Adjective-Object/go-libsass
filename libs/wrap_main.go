// +build !dev

package libs

// #cgo CFLAGS: -O2 -fPIC
// #cgo CPPFLAGS: -I../libsass-build -I../libsass-build/include
// #cgo CXXFLAGS: -g -std=c++0x -O2 -fPIC
// #cgo LDFLAGS: -lstdc++ -lm
// #cgo darwin linux LDFLAGS: -ldl
//
// // Opt into windows > XP so we have access to the
// // SRWLOCK type from the windows.h stdlib
// #cgo windows CFLAGS: -D_WIN32_WINNT=0x0600
//
// #include "./resolutioncache/rwmutex.c"
// #include "./resolutioncache/resolutioncache.c"
// #include "./resolutioncache/resolvercookie.c"
// #include "./resolutioncache/importerhandler.c"
import "C"
