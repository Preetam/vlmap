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
	"fmt"
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
	C.vlmap_remove(m.vlm, C.vlmap_version(m.vlm), C.helper(C.CString(key)), C.int(len(key)))
}

func (m *Map) NewMapIterator(version uint64, startkey string, endkey string) *MapIterator {
	return &MapIterator{
		iter: C.vlmap_iterator_create(m.vlm, C.uint64_t(version),
			C.helper(C.CString(startkey)), C.int(len(startkey)),
			C.helper(C.CString(endkey)), C.int(len(endkey))),
	}
}

type MapIterator struct {
	iter *C.vlmap_iterator
}

func (i *MapIterator) Next() {
	next := C.vlmap_iterator_next(i.iter)
	if uintptr(unsafe.Pointer(next)) == 0 {
		C.vlmap_iterator_destroy(i.iter)
		i.iter = nil
		return
	}

	i.iter = next
}

func (i *MapIterator) Key() (string, bool) {
	var key *C.uint8_t
	var keylen C.int
	getErr := C.vlmap_iterator_get_key(i.iter, &key, &keylen)
	if getErr == 0 {
		keyStr := C.GoStringN(C.helper2(key), keylen)
		C.free(unsafe.Pointer(key))
		return keyStr, true
	} else {
		return "", false
	}
}

func (i *MapIterator) Value() (string, bool) {
	var val *C.uint8_t
	var vallen C.int
	getErr := C.vlmap_iterator_get_value(i.iter, &val, &vallen)
	if getErr == 0 {
		value := C.GoStringN(C.helper2(val), vallen)
		C.free(unsafe.Pointer(val))
		return value, true
	} else {
		return "", false
	}
}

func (m *Map) GetRange(version uint64, start string, end string) []string {
	ret := make([]string, 0, 10)

	i := m.NewMapIterator(version, start, end)
	for i.iter != nil {
		if key, ok := i.Key(); ok {
			ret = append(ret, key)
		}
		i.Next()
	}
	return ret
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

	m.VersionInc()
	m.Remove("a")
	m.VersionInc()

	if _, ok := m.Get("a", 1); ok {
		log.Fatalf("Got a value when I shouldn't have")
	}
	if _, ok := m.Get("a", 4); ok {
		log.Fatalf("Got a value when I shouldn't have")
	}
	if val, ok := m.Get("a", 3); val != "a" || !ok {
		log.Fatalf("Expected `a' => `%v', got `%v'. Okay: %v", "a", val, ok)
	}

	if r := m.GetRange(3, "\x00", "\xff"); fmt.Sprint(r) != "[a b d f]" {
		log.Fatalf("Expected range %v, got %v.", "[a b d f]", r)
	}
	if r := m.GetRange(4, "\x00", "\xff"); fmt.Sprint(r) != "[b d f]" {
		log.Fatalf("Expected range %v, got %v.", "[b d f]", r)
	}
}
