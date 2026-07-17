// jit.c - see jit.h.
//
// Codegen is a stack machine: every node leaves its value on the machine stack,
// and a binary op pops two and pushes one. It wastes instructions that a
// register allocator would not, but it is correct at any expression depth and
// needs no allocation pass. Registers come later.
//
// Instructions are built in a scratch array and copied into executable memory
// in one shot, because the write toggle in os_exec_write_begin is per-thread
// and cannot be held open across a recursive walk.

#include "jit.h"

#if !ARCH_ARM64
#error "jit.c: arm64 is the only backend so far"
#endif

ArrayDef(InstArray, u32);

// x0 carries every value and is the return register. x1 holds the right operand
// while a binary op executes. x2 and x3 are scratch for reporting a trap.
// Register 31 reads as the zero register in the data-processing encodings below
// and as sp in the load and store ones.
#define REG_RESULT 0
#define REG_RHS    1
#define REG_TMP0   2
#define REG_TMP1   3
#define REG_ZERO   31

typedef struct Emitter Emitter;
struct Emitter
{
    InstArray code;
    i64 *trap; // baked into the code as a constant address
};

static u32 arm64_movz_(u32 rd, u16 imm16, u32 shift)
{
    return 0xD2800000 | ((shift / 16) << 21) | ((u32)imm16 << 5) | rd;
}

static u32 arm64_movk_(u32 rd, u16 imm16, u32 shift)
{
    return 0xF2800000 | ((shift / 16) << 21) | ((u32)imm16 << 5) | rd;
}

static u32 arm64_add_(u32 rd, u32 rn, u32 rm)
{
    return 0x8B000000 | (rm << 16) | (rn << 5) | rd;
}

static u32 arm64_sub_(u32 rd, u32 rn, u32 rm)
{
    return 0xCB000000 | (rm << 16) | (rn << 5) | rd;
}

// madd rd, rn, rm, xzr
static u32 arm64_mul_(u32 rd, u32 rn, u32 rm)
{
    return 0x9B007C00 | (rm << 16) | (rn << 5) | rd;
}

static u32 arm64_sdiv_(u32 rd, u32 rn, u32 rm)
{
    return 0x9AC00C00 | (rm << 16) | (rn << 5) | rd;
}

// str rt, [sp, #-16]! and ldr rt, [sp], #16. Sixteen rather than eight keeps sp
// 16-byte aligned, which arm64 requires of every sp update.
static u32 arm64_push_(u32 rt)
{
    return 0xF81F0FE0 | rt;
}

static u32 arm64_pop_(u32 rt)
{
    return 0xF84107E0 | rt;
}

static u32 arm64_ret_(void)
{
    return 0xD65F03C0;
}

// ldr rt, [rn] and str rt, [rn], both at a zero offset.
static u32 arm64_ldr_(u32 rt, u32 rn)
{
    return 0xF9400000 | (rn << 5) | rt;
}

static u32 arm64_str_(u32 rt, u32 rn)
{
    return 0xF9000000 | (rn << 5) | rt;
}

// Both branch encodings count in instructions from the branch itself, not in
// bytes, and both are emitted with a zero offset and patched once the target is
// known. cbz reaches 19 bits of signed instructions, b reaches 26.
static u32 arm64_cbz_(u32 rt)
{
    return 0xB4000000 | rt;
}

static u32 arm64_b_(void)
{
    return 0x14000000;
}

static void jit_patch_cbz_(InstArray *code, u64 at, u64 target)
{
    i64 delta = (i64)target - (i64)at;
    Assert(delta >= -(1 << 18) && delta < (1 << 18));
    u32 imm19   = (u32)((u64)delta & 0x7FFFF);
    code->v[at] = (code->v[at] & ~(0x7FFFFu << 5)) | (imm19 << 5);
}

static void jit_patch_b_(InstArray *code, u64 at, u64 target)
{
    i64 delta = (i64)target - (i64)at;
    Assert(delta >= -(1 << 25) && delta < (1 << 25));
    u32 imm26   = (u32)((u64)delta & 0x3FFFFFF);
    code->v[at] = (code->v[at] & ~0x3FFFFFFu) | imm26;
}

// movz of the low half, then movk for each nonzero half above it. A negative
// value fills all four halves, so it costs four instructions.
static void jit_emit_imm64_(InstArray *code, u32 rd, i64 value)
{
    u64 bits = (u64)value;
    ArrayPush(code, arm64_movz_(rd, (u16)(bits & 0xFFFF), 0));
    for (u32 shift = 16; shift < 64; shift += 16)
    {
        u16 part = (u16)((bits >> shift) & 0xFFFF);
        if (part != 0)
        {
            ArrayPush(code, arm64_movk_(rd, part, shift));
        }
    }
}

// sdiv yields 0 for a zero divisor and never traps, so the check is explicit.
// The trap path leaves a zero in `rd` and falls through rather than returning.
// Returning from here would strand whatever operands the surrounding expression
// had already pushed, and the caller discards the value anyway.
static void jit_emit_div_(Emitter *em, u32 rd, u32 rn, u32 rm)
{
    u64 to_fail = em->code.count;
    ArrayPush(&em->code, arm64_cbz_(rm));

    ArrayPush(&em->code, arm64_sdiv_(rd, rn, rm));
    u64 to_done = em->code.count;
    ArrayPush(&em->code, arm64_b_());

    jit_patch_cbz_(&em->code, to_fail, em->code.count);
    jit_emit_imm64_(&em->code, REG_TMP0, TrapKind_DivideByZero);
    jit_emit_imm64_(&em->code, REG_TMP1, (i64)(u64)(uintptr_t)em->trap);
    ArrayPush(&em->code, arm64_str_(REG_TMP0, REG_TMP1));
    jit_emit_imm64_(&em->code, rd, 0);

    jit_patch_b_(&em->code, to_done, em->code.count);
}

// Every node below leaves exactly one value on the machine stack. A block is
// handled here rather than in the switch because it reaches that state its own
// way, through its trailing expression, and so must skip the common tail push.
static void jit_emit_node_(Emitter *em, Node *node)
{
    if (node->kind == NodeKind_Block)
    {
        for (Node *stmt = node->lhs; stmt != 0; stmt = stmt->next)
        {
            jit_emit_node_(em, stmt);
            ArrayPush(&em->code, arm64_pop_(REG_RESULT)); // a statement's value is dropped
        }

        if (node->rhs != 0)
        {
            jit_emit_node_(em, node->rhs);
        }
        else
        {
            // A block with no trailing expression still has to leave something,
            // because the caller pops one value. Nothing reads it.
            jit_emit_imm64_(&em->code, REG_RESULT, 0);
            ArrayPush(&em->code, arm64_push_(REG_RESULT));
        }
        return;
    }

    switch (node->kind)
    {
    case NodeKind_Int:
    {
        jit_emit_imm64_(&em->code, REG_RESULT, node->value);
    }
    break;

    // A slot's address is a compile-time constant, because sym.c fixed it when
    // the name was declared. Baking it in is what lets code compiled now reach
    // a variable declared on an earlier line.
    case NodeKind_Var:
    {
        jit_emit_imm64_(&em->code, REG_RESULT, (i64)(u64)(uintptr_t)node->slot);
        ArrayPush(&em->code, arm64_ldr_(REG_RESULT, REG_RESULT));
    }
    break;

    // Both store to a slot check.c already resolved, so they only differ in
    // whether the name was new. Codegen cannot tell and does not need to.
    case NodeKind_Decl:
    case NodeKind_Assign:
    {
        jit_emit_node_(em, node->lhs);
        ArrayPush(&em->code, arm64_pop_(REG_RESULT));
        jit_emit_imm64_(&em->code, REG_RHS, (i64)(u64)(uintptr_t)node->slot);
        ArrayPush(&em->code, arm64_str_(REG_RESULT, REG_RHS));
    }
    break;

    case NodeKind_Neg:
    {
        jit_emit_node_(em, node->lhs);
        ArrayPush(&em->code, arm64_pop_(REG_RESULT));
        ArrayPush(&em->code, arm64_sub_(REG_RESULT, REG_ZERO, REG_RESULT));
    }
    break;

    case NodeKind_Add:
    case NodeKind_Sub:
    case NodeKind_Mul:
    case NodeKind_Div:
    {
        jit_emit_node_(em, node->lhs);
        jit_emit_node_(em, node->rhs);
        ArrayPush(&em->code, arm64_pop_(REG_RHS));    // rhs was pushed last
        ArrayPush(&em->code, arm64_pop_(REG_RESULT)); // lhs

        if (node->kind == NodeKind_Add)
        {
            ArrayPush(&em->code, arm64_add_(REG_RESULT, REG_RESULT, REG_RHS));
        }
        else if (node->kind == NodeKind_Sub)
        {
            ArrayPush(&em->code, arm64_sub_(REG_RESULT, REG_RESULT, REG_RHS));
        }
        else if (node->kind == NodeKind_Mul)
        {
            ArrayPush(&em->code, arm64_mul_(REG_RESULT, REG_RESULT, REG_RHS));
        }
        else
        {
            jit_emit_div_(em, REG_RESULT, REG_RESULT, REG_RHS);
        }
    }
    break;

    default:
    {
        Assert(!"jit_emit_node_: unhandled NodeKind");
    }
    break;
    }
    ArrayPush(&em->code, arm64_push_(REG_RESULT));
}

static char *jit_trap_messages_[] = {
    "no trap",
    "division by zero",
};
StaticAssert(ArrayCount(jit_trap_messages_) == TrapKind_COUNT, jit_trap_messages_complete);

char *jit_trap_message(TrapKind kind)
{
    if ((u32)kind >= TrapKind_COUNT)
    {
        return "an unknown trap";
    }
    return jit_trap_messages_[kind];
}

static b32 jit_install_(Jit *jit, u32 *code, u64 count, JitFunc *out)
{
    u64 size        = count * sizeof(u32);
    u64 pos_aligned = AlignPow2(jit->pos, 16);
    u64 new_pos     = pos_aligned + size;

    if (new_pos > jit->reserved)
    {
        return 0;
    }

    if (new_pos > jit->committed)
    {
        u64 commit_target = AlignPow2(new_pos, JIT_COMMIT_GRANULARITY);
        commit_target     = Min(commit_target, jit->reserved);
        if (!os_commit_exec(jit->base + jit->committed, commit_target - jit->committed))
        {
            return 0;
        }
        jit->committed = commit_target;
    }

    u8 *dst = jit->base + pos_aligned;
    os_exec_write_begin();
    MemoryCopy(dst, code, size);
    os_exec_write_end(dst, size);

    jit->pos = new_pos;
    *out     = (JitFunc)(void *)dst;
    return 1;
}

b32 jit_init(Jit *jit, u64 reserve_size)
{
    MemoryZeroStruct(jit);

    reserve_size = AlignPow2(reserve_size, os_page_size());
    jit->base    = (u8 *)os_reserve_exec(reserve_size);
    if (jit->base == 0)
    {
        return 0;
    }
    jit->reserved = reserve_size;
    return 1;
}

void jit_shutdown(Jit *jit)
{
    if (jit->base != 0)
    {
        os_release(jit->base, jit->reserved);
    }
    MemoryZeroStruct(jit);
}

b32 jit_compile(Jit *jit, Arena *scratch, Node *ast, i64 *trap, JitFunc *out)
{
    *out = 0;

    Temp temp = temp_begin(scratch);

    Emitter em = {0};
    em.trap    = trap;
    ArrayInit(&em.code, scratch);

    jit_emit_node_(&em, ast);
    ArrayPush(&em.code, arm64_pop_(REG_RESULT)); // the whole expression's value
    ArrayPush(&em.code, arm64_ret_());

    b32 result = 0;
    if (em.code.v != 0)
    {
        result = jit_install_(jit, em.code.v, em.code.count, out);
    }

    temp_end(temp);
    return result;
}
