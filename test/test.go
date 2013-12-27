package main

/*
#cgo LDFLAGS: -lvlmap
#include <vlmap.h>

uint8_t*
helper(char* s) {
	return (uint8_t*)s;
}

char*
helper2(uint8_t* s) {
	return (char*)s;
}
*/
import "C"

import (
	"log"
	"unsafe"
)

type Map struct {
	vlm *C.vlmap
}

func NewVlmap() *Map {
	return &Map{
		vlm: C.vlmap_create(),
	}
}

func (m *Map) Destroy() {
	C.vlmap_destroy(m.vlm)
}

func (m *Map) Version() uint64 {
	return uint64(C.vlmap_version(m.vlm))
}

func (m *Map) VersionInc() {
	C.vlmap_version_increment(m.vlm)
}

func (m *Map) Set(key string, value string) {
	C.vlmap_insert(m.vlm, C.vlmap_version(m.vlm), C.helper(C.CString(key)), C.int(len(key)),
		C.helper(C.CString(value)), C.int(len(value)))
}

func (m *Map) Get(key string, version uint64) (string, bool) {
	var val *C.uint8_t
	var vallen C.int
	getErr := C.vlmap_get(m.vlm, C.uint64_t(version), C.helper(C.CString(key)), C.int(len(key)), &val, &vallen)
	if getErr == 0 {
		value := C.GoStringN(C.helper2(val), vallen)
		C.free(unsafe.Pointer(val))
		return value, true
	} else {
		return "", false
	}
}

func (m *Map) Remove(key string) {
	C.vlmap_remove(m.vlm, 1, C.helper(C.CString(key)), C.int(len(key)))
}

func main() {
	m := NewVlmap()
	if version := m.Version(); version != 1 {
		log.Fatalf("Expected version to be %v, got %v\n", 1, version)
	}
	m.VersionInc()
	if version := m.Version(); version != 2 {
		log.Fatalf("Expected version to be %v, got %v\n", 2, version)
	}
	m.Destroy()

	m = NewVlmap()
	m.Set("foo", "bar")
	if val, ok := m.Get("foo", 1); val != "bar" || !ok {
		log.Fatalf("Expected `foo' => `%v', got `%v'. Okay: %v", "bar", val, ok)
	}
	m.Destroy()

	m = NewVlmap()
	m.Set("b", "b")
	m.VersionInc()
	m.Set("d", "d")
	m.Set("a", "a")
	m.VersionInc()
	m.Set("f", "f")

	if _, ok := m.Get("d", 1); ok {
		log.Fatalf("Got a value when I shouldn't have")
	}
	if _, ok := m.Get("a", 1); ok {
		log.Fatalf("Got a value when I shouldn't have")
	}
	if _, ok := m.Get("f", 1); ok {
		log.Fatalf("Got a value when I shouldn't have")
	}
	if _, ok := m.Get("f", 2); ok {
		log.Fatalf("Got a value when I shouldn't have")
	}
	if val, ok := m.Get("d", 2); val != "d" || !ok {
		log.Fatalf("Expected `d' => `%v', got `%v'. Okay: %v", "d", val, ok)
	}
	if val, ok := m.Get("d", 3); val != "d" || !ok {
		log.Fatalf("Expected `d' => `%v', got `%v'. Okay: %v", "d", val, ok)
	}
	if val, ok := m.Get("b", 3); val != "b" || !ok {
		log.Fatalf("Expected `b' => `%v', got `%v'. Okay: %v", "b", val, ok)
	}
}
