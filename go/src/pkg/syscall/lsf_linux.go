// Copyright 2011 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Linux socket filter

package syscall

import (
	"unsafe"
)

func LsfStmt(code, k int) *SockFilter {
	return &SockFilter{Code: uint16(code), K: uint32(k)}
}

func LsfJump(code, k, jt, jf int) *SockFilter {
	return &SockFilter{Code: uint16(code), Jt: uint8(jt), Jf: uint8(jf), K: uint32(k)}
}

func LsfSocket(ifindex, proto int) (int, int) {
	var lsall SockaddrLinklayer
	s, e := Socket(AF_PACKET, SOCK_RAW, proto)
	if e != 0 {
		return 0, e
	}
	p := (*[2]byte)(unsafe.Pointer(&lsall.Protocol))
	p[0] = byte(proto >> 8)
	p[1] = byte(proto)
	lsall.Ifindex = ifindex
	e = Bind(s, &lsall)
	if e != 0 {
		Close(s)
		return 0, e
	}
	return s, 0
}

type iflags struct {
	name  [IFNAMSIZ]byte
	flags uint16
}

func SetLsfPromisc(name string, m bool) int {
	s, e := Socket(AF_INET, SOCK_DGRAM, 0)
	if e != 0 {
		return e
	}
	defer Close(s)
	var ifl iflags
	copy(ifl.name[:], []byte(name))
	_, _, ep := Syscall(SYS_IOCTL, uintptr(s), SIOCGIFFLAGS, uintptr(unsafe.Pointer(&ifl)))
	if e := int(ep); e != 0 {
		return e
	}
	if m {
		ifl.flags |= uint16(IFF_PROMISC)
	} else {
		ifl.flags &= ^uint16(IFF_PROMISC)
	}
	_, _, ep = Syscall(SYS_IOCTL, uintptr(s), SIOCSIFFLAGS, uintptr(unsafe.Pointer(&ifl)))
	if e := int(ep); e != 0 {
		return e
	}
	return 0
}

func AttachLsf(fd int, i []SockFilter) int {
	var p SockFprog
	p.Len = uint16(len(i))
	p.Filter = (*SockFilter)(unsafe.Pointer(&i[0]))
	return setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, uintptr(unsafe.Pointer(&p)), unsafe.Sizeof(p))
}

func DetachLsf(fd int) int {
	var dummy int
	return setsockopt(fd, SOL_SOCKET, SO_DETACH_FILTER, uintptr(unsafe.Pointer(&dummy)), unsafe.Sizeof(dummy))
}