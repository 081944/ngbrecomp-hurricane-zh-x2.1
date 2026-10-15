"""Comparison snapshots must survive a join of different CMP operands."""
from tools.recomp import config
from tools.recomp.translator import FunctionTranslator, _merge_flag_states
from tools.recomp.disasm import Operand

BASE = 0x10000

def translate_join(consumer=bytes.fromhex('0f95c0c3')):
    # test ecx,ecx; jz alternate; cmp eax,edx; jmp join; nop;
    # alternate: cmp ebx,esi; join: consumer.
    image = bytes.fromhex('85c9740539d0eb039039f3') + consumer
    config._install([config.Section('.text', BASE, len(image), 0, len(image), True)],
                              entry_point=BASE, kernel_thunk_addr=BASE,
                              origin='flag-join-test')
    db = {BASE: {'start': hex(BASE), 'end': BASE + len(image),
                 '_addr': BASE, 'size': len(image)}}
    return FunctionTranslator(image, db).translate_function(BASE, db[BASE])

def test_different_cmp_operands_join_for_setne():
    code = translate_join()
    assert 'CMP_NE(_fa, _fb)' in code, code
    assert '_flags /* setne */' not in code

def test_different_cmp_operands_join_for_cmovne():
    code = translate_join(bytes.fromhex('0f45c7c3'))
    assert 'if (CMP_NE(_fa, _fb)) eax = edi;' in code, code

def test_unknown_path_is_not_guessed():
    assert _merge_flag_states([None, ('cmp', [])]) is None

def test_mixed_operations_are_not_guessed():
    a = Operand(type='reg', reg='eax')
    b = Operand(type='reg', reg='edx')
    assert _merge_flag_states([('cmp', [a, b]), ('test', [a, b])]) is None

def test_mixed_width_compares_answer_width_free_conditions_only():
    """cmp snapshots of different widths reaching one jcc (Ninja Gaiden
    Black's 0x0003B590): equality and ordering read the per-width snapshot,
    so they are answered; sign, overflow and parity are not."""
    from tools.recomp.lifter import _make_condition
    wide = [Operand(type='reg', reg='eax'), Operand(type='reg', reg='edx')]
    narrow = [Operand(type='reg', reg='al'), Operand(type='reg', reg='dl')]
    merged = _merge_flag_states([('cmp', wide), ('cmp', narrow)])
    assert merged is not None and merged[0] == '__cmp_mixed_width'
    assert _make_condition('je', *merged)[0] == 'CMP_EQ(_fa, _fb)'
    assert _make_condition('jb', *merged)[0] == 'CMP_B(_fa, _fb)'
    assert _make_condition('jl', *merged)[0] == 'CMP_L(_fas, _fbs)'
    assert _make_condition('js', *merged) is None
    assert _make_condition('jo', *merged) is None

def test_mixed_width_tests_are_not_guessed():
    wide = [Operand(type='reg', reg='eax'), Operand(type='reg', reg='edx')]
    narrow = [Operand(type='reg', reg='al'), Operand(type='reg', reg='dl')]
    assert _merge_flag_states([('test', wide), ('test', narrow)]) is None


def test_sse_compares_with_different_operands_merge():
    """Joins of comiss predecessors read the _fca/_fcb snapshot, whatever
    the operands were (dozens of such joins in Ninja Gaiden Black lifted to
    the never-taken fallback)."""
    a = [Operand(type='reg', reg='xmm0'), Operand(type='reg', reg='xmm1')]
    b = [Operand(type='reg', reg='xmm7'), Operand(type='reg', reg='xmm5')]
    merged = _merge_flag_states([('comiss', a), ('ucomiss', b)])
    assert merged is not None and merged[0] in ('comiss', 'ucomiss')
