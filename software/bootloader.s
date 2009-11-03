_start:
	jmp _test3

_test1:
	li $48
	str $2
	rot
	rot
	rot
	rot
	rot
	rot
	rot
	rot
	xor $2
	jz _test_success
	jmp _test_failure

_test2:
	li $30
	str $2
	li $-30
	add $2
	jz _test_success
	jmp _test_failure

_test3:
	li $-1
	str $2
	li $40
	xor $2
	str $3
	li $40
	not
	xor $3
	jz _test_success
	jmp _test_failure

_test_failure:
	li $2
	str $0
	jmp _test_death

_test_success:
	li $1
	str $0
	jmp _test_death

_test_death:
	jmp _test_death
