public cblib_cmpxchg64
public cblib_cmpxchg128
public cblib_cpuidex

.CODE

;-------------------------------------------------------------------------------------------

;int cdecl
;cblib_cmpxchg64
;( void*,	// rcx
;  void*,	// rdx
;  const uint64 ); // r8

align 8
cblib_cmpxchg64_old PROC 
;  // val* = rcx
;  // old* = rdx
;  // new = r8

  mov rax, [rdx]
  lock cmpxchg [rcx], r8
  jne cblib_cmpxchg64_fail
  mov rax, 1
  ret

align 8
cblib_cmpxchg64_fail:
  mov [rdx], rax
  mov rax, 0
  ret
align 8
cblib_cmpxchg64_old ENDP

align 8
cblib_cmpxchg64 PROC 
;  // val* = rcx
;  // old* = rdx
;  // new = r8

  mov rax, [rdx]
  lock cmpxchg [rcx], r8
  sete cl
  mov [rdx], rax
;  xor rax,rax
  movzx rax,cl
  ret
cblib_cmpxchg64 ENDP

;-------------------------------------------------------------------------------------------

;int __cdecl cblib_cmpxchg128( __m128* pVal, __m128* pOld, const __m128 * pNew);

align 8
cblib_cmpxchg128 PROC 
;  // val* = rcx
;  // old* = rdx
;  // new = r8

  push rbx
  push rdi
  push rsi

  mov rdi, rcx ; rdi = val
  mov rsi, rdx ; rsi = old

 ; rdx:rax = old
 ; rcx:rbx = new

  mov rdx, [rsi+8]
  mov rax, [rsi]
  mov rcx, [r8 +8]
  mov rbx, [r8 ]

  lock cmpxchg16b [rdi]
  jne cblib_cmpxchg128_fail
  mov rax, 1

  pop rsi
  pop rdi
  pop rbx
  ret

align 8
cblib_cmpxchg128_fail:
  mov [rsi+8] , rdx
  mov [rsi]   , rax
  mov rax, 0

  pop rsi
  pop rdi
  pop rbx
  ret
align 8
cblib_cmpxchg128 ENDP


;extern void __cdecl cblib_cpuidex( uint32 eax, uint32 ecx, const void * pInto);

align 8
cblib_cpuidex PROC
; eax = rcx
; ecx = rdx
; pInto = r8

  push rbx

  mov eax, ecx
  mov ecx, edx
  cpuid

	mov DWORD PTR [r8+0], eax
	mov DWORD PTR [r8+4], ebx
	mov DWORD PTR [r8+8], ecx
	mov DWORD PTR [r8+12], edx

  pop rbx
  ret
align 8
cblib_cpuidex ENDP

END




