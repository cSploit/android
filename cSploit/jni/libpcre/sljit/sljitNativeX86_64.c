/*
 *    Stack-less Just-In-Time compiler
 *
 *    Copyright 2009-2012 Zoltan Herczeg (hzmester@freemail.hu). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this list of
 *      conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice, this list
 *      of conditions and the following disclaimer in the documentation and/or other materials
 *      provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDER(S) OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* x86 64-bit arch dependent functions. */

static sljit_si emit_load_imm64(struct sljit_compiler *compiler, sljit_si reg, sljit_sw imm)
{
	sljit_ub *inst;

	inst = (sljit_ub*)ensure_buf(compiler, 1 + 2 + sizeof(sljit_sw));
	FAIL_IF(!inst);
	INC_SIZE(2 + sizeof(sljit_sw));
	*inst++ = REX_W | ((reg_map[reg] <= 7) ? 0 : REX_B);
	*inst++ = MOV_r_i32 + (reg_map[reg] & 0x7);
	*(sljit_sw*)inst = imm;
	return SLJIT_SUCCESS;
}

static sljit_ub* generate_far_jump_code(struct sljit_jump *jump, sljit_ub *code_ptr, sljit_si type)
{
	if (type < SLJIT_JUMP) {
		/* Invert type. */
		*code_ptr++ = get_jump_code(type ^ 0x1) - 0x10;
		*code_ptr++ = 10 + 3;
	}

	SLJIT_COMPILE_ASSERT(reg_map[TMP_REG3] == 9, tmp3_is_9_first);
	*code_ptr++ = REX_W | REX_B;
	*code_ptr++ = MOV_r_i32 + 1;
	jump->addr = (sljit_uw)code_ptr;

	if (jump->flags & JUMP_LABEL)
		jump->flags |= PATCH_MD;
	else
		*(sljit_sw*)code_ptr = jump->u.target;

	code_ptr += sizeof(sljit_sw);
	*code_ptr++ = REX_B;
	*code_ptr++ = GROUP_FF;
	*code_ptr++ = (type >= SLJIT_FAST_CALL) ? (MOD_REG | CALL_rm | 1) : (MOD_REG | JMP_rm | 1);

	return code_ptr;
}

static sljit_ub* generate_fixed_jump(sljit_ub *code_ptr, sljit_sw addr, sljit_si type)
{
	sljit_sw delta = addr - ((sljit_sw)code_ptr + 1 + sizeof(sljit_si));

	if (delta <= SLJIT_W(0x7fffffff) && delta >= SLJIT_W(-0x80000000)) {
		*code_ptr++ = (type == 2) ? CALL_i32 : JMP_i32;
		*(sljit_sw*)code_ptr = delta;
	}
	else {
		SLJIT_COMPILE_ASSERT(reg_map[TMP_REG3] == 9, tmp3_is_9_second);
		*code_ptr++ = REX_W | REX_B;
		*code_ptr++ = MOV_r_i32 + 1;
		*(sljit_sw*)code_ptr = addr;
		code_ptr += sizeof(sljit_sw);
		*code_ptr++ = REX_B;
		*code_ptr++ = GROUP_FF;
		*code_ptr++ = (type == 2) ? (MOD_REG | CALL_rm | 1) : (MOD_REG | JMP_rm | 1);
	}

	return code_ptr;
}

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_emit_enter(struct sljit_compiler *compiler, sljit_si args, sljit_si scratches, sljit_si saveds, sljit_si local_size)
{
	sljit_si size, pushed_size;
	sljit_ub *inst;

	CHECK_ERROR();
	check_sljit_emit_enter(compiler, args, scratches, saveds, local_size);

	compiler->scratches = scratches;
	compiler->saveds = saveds;
	compiler->flags_saved = 0;
#if (defined SLJIT_DEBUG && SLJIT_DEBUG)
	compiler->logical_local_size = local_size;
#endif

	size = saveds;
	/* Including the return address saved by the call instruction. */
	pushed_size = (saveds + 1) * sizeof(sljit_sw);
#ifndef _WIN64
	if (saveds >= 2)
		size += saveds - 1;
#else
	if (saveds >= 4)
		size += saveds - 3;
	if (scratches >= 5) {
		size += (5 - 4) * 2;
		pushed_size += sizeof(sljit_sw);
	}
#endif
	size += args * 3;
	if (size > 0) {
		inst = (sljit_ub*)ensure_buf(compiler, 1 + size);
		FAIL_IF(!inst);

		INC_SIZE(size);
		if (saveds >= 5) {
			SLJIT_COMPILE_ASSERT(reg_map[SLJIT_SAVED_EREG2] >= 8, saved_ereg2_is_hireg);
			*inst++ = REX_B;
			PUSH_REG(reg_lmap[SLJIT_SAVED_EREG2]);
		}
		if (saveds >= 4) {
			SLJIT_COMPILE_ASSERT(reg_map[SLJIT_SAVED_EREG1] >= 8, saved_ereg1_is_hireg);
			*inst++ = REX_B;
			PUSH_REG(reg_lmap[SLJIT_SAVED_EREG1]);
		}
		if (saveds >= 3) {
#ifndef _WIN64
			SLJIT_COMPILE_ASSERT(reg_map[SLJIT_SAVED_REG3] >= 8, saved_reg3_is_hireg);
			*inst++ = REX_B;
#else
			SLJIT_COMPILE_ASSERT(reg_map[SLJIT_SAVED_REG3] < 8, saved_reg3_is_loreg);
#endif
			PUSH_REG(reg_lmap[SLJIT_SAVED_REG3]);
		}
		if (saveds >= 2) {
#ifndef _WIN64
			SLJIT_COMPILE_ASSERT(reg_map[SLJIT_SAVED_REG2] >= 8, saved_reg2_is_hireg);
			*inst++ = REX_B;
#else
			SLJIT_COMPILE_ASSERT(reg_map[SLJIT_SAVED_REG2] < 8, saved_reg2_is_loreg);
#endif
			PUSH_REG(reg_lmap[SLJIT_SAVED_REG2]);
		}
		if (saveds >= 1) {
			SLJIT_COMPILE_ASSERT(reg_map[SLJIT_SAVED_REG1] < 8, saved_reg1_is_loreg);
			PUSH_REG(reg_lmap[SLJIT_SAVED_REG1]);
		}
#ifdef _WIN64
		if (scratches >= 5) {
			SLJIT_COMPILE_ASSERT(reg_map[SLJIT_TEMPORARY_EREG2] >= 8, temporary_ereg2_is_hireg);
			*inst++ = REX_B;
			PUSH_REG(reg_lmap[SLJIT_TEMPORARY_EREG2]);
		}
#endif

#ifndef _WIN64
		if (args > 0) {
			*inst++ = REX_W;
			*inst++ = MOV_r_rm;
			*inst++ = MOD_REG | (reg_map[SLJIT_SAVED_REG1] << 3) | 0x7 /* rdi */;
		}
		if (args > 1) {
			*inst++ = REX_W | REX_R;
			*inst++ = MOV_r_rm;
			*inst++ = MOD_REG | (reg_lmap[SLJIT_SAVED_REG2] << 3) | 0x6 /* rsi */;
		}
		if (args > 2) {
			*inst++ = REX_W | REX_R;
			*inst++ = MOV_r_rm;
			*inst++ = MOD_REG | (reg_lmap[SLJIT_SAVED_REG3] << 3) | 0x2 /* rdx */;
		}
#else
		if (args > 0) {
			*inst++ = REX_W;
			*inst++ = MOV_r_rm;
			*inst++ = MOD_REG | (reg_map[SLJIT_SAVED_REG1] << 3) | 0x1 /* rcx */;
		}
		if (args > 1) {
			*inst++ = REX_W;
			*inst++ = MOV_r_rm;
			*inst++ = MOD_REG | (reg_map[SLJIT_SAVED_REG2] << 3) | 0x2 /* rdx */;
		}
		if (args > 2) {
			*inst++ = REX_W | REX_B;
			*inst++ = MOV_r_rm;
			*inst++ = MOD_REG | (reg_map[SLJIT_SAVED_REG3] << 3) | 0x0 /* r8 */;
		}
#endif
	}

	local_size = ((local_size + FIXED_LOCALS_OFFSET + pushed_size + 16 - 1) & ~(16 - 1)) - pushed_size;
	compiler->local_size = local_size;
#ifdef _WIN64
	if (local_size > 1024) {
		/* Allocate stack for the callback, which grows the stack. */
		inst = (sljit_ub*)ensure_buf(compiler, 1 + 4 + (3 + sizeof(sljit_si)));
		FAIL_IF(!inst);
		INC_SIZE(4 + (3 + sizeof(sljit_si)));
		*inst++ = REX_W;
		*inst++ = GROUP_BINARY_83;
		*inst++ = MOD_REG | SUB | 4;
		/* Pushed size must be divisible by 8. */
		SLJIT_ASSERT(!(pushed_size & 0x7));
		if (pushed_size & 0x8) {
			*inst++ = 5 * sizeof(sljit_sw);
			local_size -= 5 * sizeof(sljit_sw);
		} else {
			*inst++ = 4 * sizeof(sljit_sw);
			local_size -= 4 * sizeof(sljit_sw);
		}
		/* Second instruction */
		SLJIT_COMPILE_ASSERT(reg_map[SLJIT_SCRATCH_REG1] < 8, temporary_reg1_is_loreg);
		*inst++ = REX_W;
		*inst++ = MOV_rm_i32;
		*inst++ = MOD_REG | reg_lmap[SLJIT_SCRATCH_REG1];
		*(sljit_si*)inst = local_size;
#if (defined SLJIT_VERBOSE && SLJIT_VERBOSE) || (defined SLJIT_DEBUG && SLJIT_DEBUG)
		compiler->skip_checks = 1;
#endif
		FAIL_IF(sljit_emit_ijump(compiler, SLJIT_CALL1, SLJIT_IMM, SLJIT_FUNC_OFFSET(sljit_grow_stack)));
	}
#endif
	SLJIT_ASSERT(local_size > 0);
	if (local_size <= 127) {
		inst = (sljit_ub*)ensure_buf(compiler, 1 + 4);
		FAIL_IF(!inst);
		INC_SIZE(4);
		*inst++ = REX_W;
		*inst++ = GROUP_BINARY_83;
		*inst++ = MOD_REG | SUB | 4;
		*inst++ = local_size;
	}
	else {
		inst = (sljit_ub*)ensure_buf(compiler, 1 + 7);
		FAIL_IF(!inst);
		INC_SIZE(7);
		*inst++ = REX_W;
		*inst++ = GROUP_BINARY_81;
		*inst++ = MOD_REG | SUB | 4;
		*(sljit_si*)inst = local_size;
		inst += sizeof(sljit_si);
	}
#ifdef _WIN64
	/* Save xmm6 with MOVAPS instruction. */
	inst = (sljit_ub*)ensure_buf(compiler, 1 + 5);
	FAIL_IF(!inst);
	INC_SIZE(5);
	*inst++ = GROUP_0F;
	*(sljit_si*)inst = 0x20247429;
#endif

	return SLJIT_SUCCESS;
}

SLJIT_API_FUNC_ATTRIBUTE void sljit_set_context(struct sljit_compiler *compiler, sljit_si args, sljit_si scratches, sljit_si saveds, sljit_si local_size)
{
	sljit_si pushed_size;

	CHECK_ERROR_VOID();
	check_sljit_set_context(compiler, args, scratches, saveds, local_size);

	compiler->scratches = scratches;
	compiler->saveds = saveds;
#if (defined SLJIT_DEBUG && SLJIT_DEBUG)
	compiler->logical_local_size = local_size;
#endif

	/* Including the return address saved by the call instruction. */
	pushed_size = (saveds + 1) * sizeof(sljit_sw);
#ifdef _WIN64
	if (scratches >= 5)
		pushed_size += sizeof(sljit_sw);
#endif
	compiler->local_size = ((local_size + FIXED_LOCALS_OFFSET + pushed_size + 16 - 1) & ~(16 - 1)) - pushed_size;
}

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_emit_return(struct sljit_compiler *compiler, sljit_si op, sljit_si src, sljit_sw srcw)
{
	sljit_si size;
	sljit_ub *inst;

	CHECK_ERROR();
	check_sljit_emit_return(compiler, op, src, srcw);

	compiler->flags_saved = 0;
	FAIL_IF(emit_mov_before_return(compiler, op, src, srcw));

#ifdef _WIN64
	/* Restore xmm6 with MOVAPS instruction. */
	inst = (sljit_ub*)ensure_buf(compiler, 1 + 5);
	FAIL_IF(!inst);
	INC_SIZE(5);
	*inst++ = GROUP_0F;
	*(sljit_si*)inst = 0x20247428;
#endif
	SLJIT_ASSERT(compiler->local_size > 0);
	if (compiler->local_size <= 127) {
		inst = (sljit_ub*)ensure_buf(compiler, 1 + 4);
		FAIL_IF(!inst);
		INC_SIZE(4);
		*inst++ = REX_W;
		*inst++ = GROUP_BINARY_83;
		*inst++ = MOD_REG | ADD | 4;
		*inst = compiler->local_size;
	}
	else {
		inst = (sljit_ub*)ensure_buf(compiler, 1 + 7);
		FAIL_IF(!inst);
		INC_SIZE(7);
		*inst++ = REX_W;
		*inst++ = GROUP_BINARY_81;
		*inst++ = MOD_REG | ADD | 4;
		*(sljit_si*)inst = compiler->local_size;
	}

	size = 1 + compiler->saveds;
#ifndef _WIN64
	if (compiler->saveds >= 2)
		size += compiler->saveds - 1;
#else
	if (compiler->saveds >= 4)
		size += compiler->saveds - 3;
	if (compiler->scratches >= 5)
		size += (5 - 4) * 2;
#endif
	inst = (sljit_ub*)ensure_buf(compiler, 1 + size);
	FAIL_IF(!inst);

	INC_SIZE(size);

#ifdef _WIN64
	if (compiler->scratches >= 5) {
		*inst++ = REX_B;
		POP_REG(reg_lmap[SLJIT_TEMPORARY_EREG2]);
	}
#endif
	if (compiler->saveds >= 1)
		POP_REG(reg_map[SLJIT_SAVED_REG1]);
	if (compiler->saveds >= 2) {
#ifndef _WIN64
		*inst++ = REX_B;
#endif
		POP_REG(reg_lmap[SLJIT_SAVED_REG2]);
	}
	if (compiler->saveds >= 3) {
#ifndef _WIN64
		*inst++ = REX_B;
#endif
		POP_REG(reg_lmap[SLJIT_SAVED_REG3]);
	}
	if (compiler->saveds >= 4) {
		*inst++ = REX_B;
		POP_REG(reg_lmap[SLJIT_SAVED_EREG1]);
	}
	if (compiler->saveds >= 5) {
		*inst++ = REX_B;
		POP_REG(reg_lmap[SLJIT_SAVED_EREG2]);
	}

	RET();
	return SLJIT_SUCCESS;
}

/* --------------------------------------------------------------------- */
/*  Operators                                                            */
/* --------------------------------------------------------------------- */

static sljit_si emit_do_imm32(struct sljit_compiler *compiler, sljit_ub rex, sljit_ub opcode, sljit_sw imm)
{
	sljit_ub *inst;
	sljit_si length = 1 + (rex ? 1 : 0) + sizeof(sljit_si);

	inst = (sljit_ub*)ensure_buf(compiler, 1 + length);
	FAIL_IF(!inst);
	INC_SIZE(length);
	if (rex)
		*inst++ = rex;
	*inst++ = opcode;
	*(sljit_si*)inst = imm;
	return SLJIT_SUCCESS;
}

static sljit_ub* emit_x86_instruction(struct sljit_compiler *compiler, sljit_si size,
	/* The register or immediate operand. */
	sljit_si a, sljit_sw imma,
	/* The general operand (not immediate). */
	sljit_si b, sljit_sw immb)
{
	sljit_ub *inst;
	sljit_ub *buf_ptr;
	sljit_ub rex = 0;
	sljit_si flags = size & ~0xf;
	sljit_si inst_size;

	/* The immediate operand must be 32 bit. */
	SLJIT_ASSERT(!(a & SLJIT_IMM) || compiler->mode32 || IS_HALFWORD(imma));
	/* Both cannot be switched on. */
	SLJIT_ASSERT((flags & (EX86_BIN_INS | EX86_SHIFT_INS)) != (EX86_BIN_INS | EX86_SHIFT_INS));
	/* Size flags not allowed for typed instructions. */
	SLJIT_ASSERT(!(flags & (EX86_BIN_INS | EX86_SHIFT_INS)) || (flags & (EX86_BYTE_ARG | EX86_HALF_ARG)) == 0);
	/* Both size flags cannot be switched on. */
	SLJIT_ASSERT((flags & (EX86_BYTE_ARG | EX86_HALF_ARG)) != (EX86_BYTE_ARG | EX86_HALF_ARG));
#if (defined SLJIT_SSE2 && SLJIT_SSE2)
	/* SSE2 and immediate is not possible. */
	SLJIT_ASSERT(!(a & SLJIT_IMM) || !(flags & EX86_SSE2));
	SLJIT_ASSERT((flags & (EX86_PREF_F2 | EX86_PREF_F3)) != (EX86_PREF_F2 | EX86_PREF_F3)
		&& (flags & (EX86_PREF_F2 | EX86_PREF_66)) != (EX86_PREF_F2 | EX86_PREF_66)
		&& (flags & (EX86_PREF_F3 | EX86_PREF_66)) != (EX86_PREF_F3 | EX86_PREF_66));
#endif

	size &= 0xf;
	inst_size = size;

	if ((b & SLJIT_MEM) && !(b & 0xf0) && NOT_HALFWORD(immb)) {
		if (emit_load_imm64(compiler, TMP_REG3, immb))
			return NULL;
		immb = 0;
		if (b & 0xf)
			b |= TMP_REG3 << 4;
		else
			b |= TMP_REG3;
	}

	if (!compiler->mode32 && !(flags & EX86_NO_REXW))
		rex |= REX_W;
	else if (flags & EX86_REX)
		rex |= REX;

#if (defined SLJIT_SSE2 && SLJIT_SSE2)
	if (flags & (EX86_PREF_F2 | EX86_PREF_F3))
		inst_size++;
#endif
	if (flags & EX86_PREF_66)
		inst_size++;

	/* Calculate size of b. */
	inst_size += 1; /* mod r/m byte. */
	if (b & SLJIT_MEM) {
		if ((b & 0x0f) == SLJIT_UNUSED)
			inst_size += 1 + sizeof(sljit_si); /* SIB byte required to avoid RIP based addressing. */
		else {
			if (reg_map[b & 0x0f] >= 8)
				rex |= REX_B;
			if (immb != 0 && !(b & 0xf0)) {
				/* Immediate operand. */
				if (immb <= 127 && immb >= -128)
					inst_size += sizeof(sljit_sb);
				else
					inst_size += sizeof(sljit_si);
			}
		}

		if ((b & 0xf) == SLJIT_LOCALS_REG && !(b & 0xf0))
			b |= SLJIT_LOCALS_REG << 4;

		if ((b & 0xf0) != SLJIT_UNUSED) {
			inst_size += 1; /* SIB byte. */
			if (reg_map[(b >> 4) & 0x0f] >= 8)
				rex |= REX_X;
		}
	}
#if (defined SLJIT_SSE2 && SLJIT_SSE2)
	else if (!(flags & EX86_SSE2) && reg_map[b] >= 8)
		rex |= REX_B;
#else
	else if (reg_map[b] >= 8)
		rex |= REX_B;
#endif

	if (a & SLJIT_IMM) {
		if (flags & EX86_BIN_INS) {
			if (imma <= 127 && imma >= -128) {
				inst_size += 1;
				flags |= EX86_BYTE_ARG;
			} else
				inst_size += 4;
		}
		else if (flags & EX86_SHIFT_INS) {
			imma &= compiler->mode32 ? 0x1f : 0x3f;
			if (imma != 1) {
				inst_size ++;
				flags |= EX86_BYTE_ARG;
			}
		} else if (flags & EX86_BYTE_ARG)
			inst_size++;
		else if (flags & EX86_HALF_ARG)
			inst_size += sizeof(short);
		else
			inst_size += sizeof(sljit_si);
	}
	else {
		SLJIT_ASSERT(!(flags & EX86_SHIFT_INS) || a == SLJIT_PREF_SHIFT_REG);
		/* reg_map[SLJIT_PREF_SHIFT_REG] is less than 8. */
#if (defined SLJIT_SSE2 && SLJIT_SSE2)
		if (!(flags & EX86_SSE2) && reg_map[a] >= 8)
			rex |= REX_R;
#else
		if (reg_map[a] >= 8)
			rex |= REX_R;
#endif
	}

	if (rex)
		inst_size++;

	inst = (sljit_ub*)ensure_buf(compiler, 1 + inst_size);
	PTR_FAIL_IF(!inst);

	/* Encoding the byte. */
	INC_SIZE(inst_size);
#if (defined SLJIT_SSE2 && SLJIT_SSE2)
	if (flags & EX86_PREF_F2)
		*inst++ = 0xf2;
	if (flags & EX86_PREF_F3)
		*inst++ = 0xf3;
#endif
	if (flags & EX86_PREF_66)
		*inst++ = 0x66;
	if (rex)
		*inst++ = rex;
	buf_ptr = inst + size;

	/* Encode mod/rm byte. */
	if (!(flags & EX86_SHIFT_INS)) {
		if ((flags & EX86_BIN_INS) && (a & SLJIT_IMM))
			*inst = (flags & EX86_BYTE_ARG) ? GROUP_BINARY_83 : GROUP_BINARY_81;

		if ((a & SLJIT_IMM) || (a == 0))
			*buf_ptr = 0;
#if (defined SLJIT_SSE2 && SLJIT_SSE2)
		else if (!(flags & EX86_SSE2))
			*buf_ptr = reg_lmap[a] << 3;
		else
			*buf_ptr = a << 3;
#else
		else
			*buf_ptr = reg_lmap[a] << 3;
#endif
	}
	else {
		if (a & SLJIT_IMM) {
			if (imma == 1)
				*inst = GROUP_SHIFT_1;
			else
				*inst = GROUP_SHIFT_N;
		} else
			*inst = GROUP_SHIFT_CL;
		*buf_ptr = 0;
	}

	if (!(b & SLJIT_MEM))
#if (defined SLJIT_SSE2 && SLJIT_SSE2)
		*buf_ptr++ |= MOD_REG + ((!(flags & EX86_SSE2)) ? reg_lmap[b] : b);
#else
		*buf_ptr++ |= MOD_REG + reg_lmap[b];
#endif
	else if ((b & 0x0f) != SLJIT_UNUSED) {
		if ((b & 0xf0) == SLJIT_UNUSED || (b & 0xf0) == (SLJIT_LOCALS_REG << 4)) {
			if (immb != 0) {
				if (immb <= 127 && immb >= -128)
					*buf_ptr |= 0x40;
				else
					*buf_ptr |= 0x80;
			}

			if ((b & 0xf0) == SLJIT_UNUSED)
				*buf_ptr++ |= reg_lmap[b & 0x0f];
			else {
				*buf_ptr++ |= 0x04;
				*buf_ptr++ = reg_lmap[b & 0x0f] | (reg_lmap[(b >> 4) & 0x0f] << 3);
			}

			if (immb != 0) {
				if (immb <= 127 && immb >= -128)
					*buf_ptr++ = immb; /* 8 bit displacement. */
				else {
					*(sljit_si*)buf_ptr = immb; /* 32 bit displacement. */
					buf_ptr += sizeof(sljit_si);
				}
			}
		}
		else {
			*buf_ptr++ |= 0x04;
			*buf_ptr++ = reg_lmap[b & 0x0f] | (reg_lmap[(b >> 4) & 0x0f] << 3) | (immb << 6);
		}
	}
	else {
		*buf_ptr++ |= 0x04;
		*buf_ptr++ = 0x25;
		*(sljit_si*)buf_ptr = immb; /* 32 bit displacement. */
		buf_ptr += sizeof(sljit_si);
	}

	if (a & SLJIT_IMM) {
		if (flags & EX86_BYTE_ARG)
			*buf_ptr = imma;
		else if (flags & EX86_HALF_ARG)
			*(short*)buf_ptr = imma;
		else if (!(flags & EX86_SHIFT_INS))
			*(sljit_si*)buf_ptr = imma;
	}

	return !(flags & EX86_SHIFT_INS) ? inst : (inst + 1);
}

/* --------------------------------------------------------------------- */
/*  Call / return instructions                                           */
/* --------------------------------------------------------------------- */

static SLJIT_INLINE sljit_si call_with_args(struct sljit_compiler *compiler, sljit_si type)
{
	sljit_ub *inst;

#ifndef _WIN64
	SLJIT_COMPILE_ASSERT(reg_map[SLJIT_SCRATCH_REG2] == 6 && reg_map[SLJIT_SCRATCH_REG1] < 8 && reg_map[SLJIT_SCRATCH_REG3] < 8, args_registers);

	inst = (sljit_ub*)ensure_buf(compiler, 1 + ((type < SLJIT_CALL3) ? 3 : 6));
	FAIL_IF(!inst);
	INC_SIZE((type < SLJIT_CALL3) ? 3 : 6);
	if (type >= SLJIT_CALL3) {
		*inst++ = REX_W;
		*inst++ = MOV_r_rm;
		*inst++ = MOD_REG | (0x2 /* rdx */ << 3) | reg_lmap[SLJIT_SCRATCH_REG3];
	}
	*inst++ = REX_W;
	*inst++ = MOV_r_rm;
	*inst++ = MOD_REG | (0x7 /* rdi */ << 3) | reg_lmap[SLJIT_SCRATCH_REG1];
#else
	SLJIT_COMPILE_ASSERT(reg_map[SLJIT_SCRATCH_REG2] == 2 && reg_map[SLJIT_SCRATCH_REG1] < 8 && reg_map[SLJIT_SCRATCH_REG3] < 8, args_registers);

	inst = (sljit_ub*)ensure_buf(compiler, 1 + ((type < SLJIT_CALL3) ? 3 : 6));
	FAIL_IF(!inst);
	INC_SIZE((type < SLJIT_CALL3) ? 3 : 6);
	if (type >= SLJIT_CALL3) {
		*inst++ = REX_W | REX_R;
		*inst++ = MOV_r_rm;
		*inst++ = MOD_REG | (0x0 /* r8 */ << 3) | reg_lmap[SLJIT_SCRATCH_REG3];
	}
	*inst++ = REX_W;
	*inst++ = MOV_r_rm;
	*inst++ = MOD_REG | (0x1 /* rcx */ << 3) | reg_lmap[SLJIT_SCRATCH_REG1];
#endif
	return SLJIT_SUCCESS;
}

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_emit_fast_enter(struct sljit_compiler *compiler, sljit_si dst, sljit_sw dstw)
{
	sljit_ub *inst;

	CHECK_ERROR();
	check_sljit_emit_fast_enter(compiler, dst, dstw);
	ADJUST_LOCAL_OFFSET(dst, dstw);

	/* For UNUSED dst. Uncommon, but possible. */
	if (dst == SLJIT_UNUSED)
		dst = TMP_REGISTER;

	if (dst <= TMP_REGISTER) {
		if (reg_map[dst] < 8) {
			inst = (sljit_ub*)ensure_buf(compiler, 1 + 1);
			FAIL_IF(!inst);
			INC_SIZE(1);
			POP_REG(reg_lmap[dst]);
			return SLJIT_SUCCESS;
		}

		inst = (sljit_ub*)ensure_buf(compiler, 1 + 2);
		FAIL_IF(!inst);
		INC_SIZE(2);
		*inst++ = REX_B;
		POP_REG(reg_lmap[dst]);
		return SLJIT_SUCCESS;
	}

	/* REX_W is not necessary (src is not immediate). */
	compiler->mode32 = 1;
	inst = emit_x86_instruction(compiler, 1, 0, 0, dst, dstw);
	FAIL_IF(!inst);
	*inst++ = POP_rm;
	return SLJIT_SUCCESS;
}

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_emit_fast_return(struct sljit_compiler *compiler, sljit_si src, sljit_sw srcw)
{
	sljit_ub *inst;

	CHECK_ERROR();
	check_sljit_emit_fast_return(compiler, src, srcw);
	ADJUST_LOCAL_OFFSET(src, srcw);

	if ((src & SLJIT_IMM) && NOT_HALFWORD(srcw)) {
		FAIL_IF(emit_load_imm64(compiler, TMP_REGISTER, srcw));
		src = TMP_REGISTER;
	}

	if (src <= TMP_REGISTER) {
		if (reg_map[src] < 8) {
			inst = (sljit_ub*)ensure_buf(compiler, 1 + 1 + 1);
			FAIL_IF(!inst);

			INC_SIZE(1 + 1);
			PUSH_REG(reg_lmap[src]);
		}
		else {
			inst = (sljit_ub*)ensure_buf(compiler, 1 + 2 + 1);
			FAIL_IF(!inst);

			INC_SIZE(2 + 1);
			*inst++ = REX_B;
			PUSH_REG(reg_lmap[src]);
		}
	}
	else if (src & SLJIT_MEM) {
		/* REX_W is not necessary (src is not immediate). */
		compiler->mode32 = 1;
		inst = emit_x86_instruction(compiler, 1, 0, 0, src, srcw);
		FAIL_IF(!inst);
		*inst++ = GROUP_FF;
		*inst |= PUSH_rm;

		inst = (sljit_ub*)ensure_buf(compiler, 1 + 1);
		FAIL_IF(!inst);
		INC_SIZE(1);
	}
	else {
		SLJIT_ASSERT(IS_HALFWORD(srcw));
		/* SLJIT_IMM. */
		inst = (sljit_ub*)ensure_buf(compiler, 1 + 5 + 1);
		FAIL_IF(!inst);

		INC_SIZE(5 + 1);
		*inst++ = PUSH_i32;
		*(sljit_si*)inst = srcw;
		inst += sizeof(sljit_si);
	}

	RET();
	return SLJIT_SUCCESS;
}


/* --------------------------------------------------------------------- */
/*  Extend input                                                         */
/* --------------------------------------------------------------------- */

static sljit_si emit_mov_int(struct sljit_compiler *compiler, sljit_si sign,
	sljit_si dst, sljit_sw dstw,
	sljit_si src, sljit_sw srcw)
{
	sljit_ub* inst;
	sljit_si dst_r;

	compiler->mode32 = 0;

	if (dst == SLJIT_UNUSED && !(src & SLJIT_MEM))
		return SLJIT_SUCCESS; /* Empty instruction. */

	if (src & SLJIT_IMM) {
		if (dst <= TMP_REGISTER) {
			if (sign || ((sljit_uw)srcw <= 0x7fffffff)) {
				inst = emit_x86_instruction(compiler, 1, SLJIT_IMM, (sljit_sw)(sljit_si)srcw, dst, dstw);
				FAIL_IF(!inst);
				*inst = MOV_rm_i32;
				return SLJIT_SUCCESS;
			}
			return emit_load_imm64(compiler, dst, srcw);
		}
		compiler->mode32 = 1;
		inst = emit_x86_instruction(compiler, 1, SLJIT_IMM, (sljit_sw)(sljit_si)srcw, dst, dstw);
		FAIL_IF(!inst);
		*inst = MOV_rm_i32;
		compiler->mode32 = 0;
		return SLJIT_SUCCESS;
	}

	dst_r = (dst <= TMP_REGISTER) ? dst : TMP_REGISTER;

	if ((dst & SLJIT_MEM) && (src <= TMP_REGISTER))
		dst_r = src;
	else {
		if (sign) {
			inst = emit_x86_instruction(compiler, 1, dst_r, 0, src, srcw);
			FAIL_IF(!inst);
			*inst++ = MOVSXD_r_rm;
		} else {
			compiler->mode32 = 1;
			FAIL_IF(emit_mov(compiler, dst_r, 0, src, srcw));
			compiler->mode32 = 0;
		}
	}

	if (dst & SLJIT_MEM) {
		compiler->mode32 = 1;
		inst = emit_x86_instruction(compiler, 1, dst_r, 0, dst, dstw);
		FAIL_IF(!inst);
		*inst = MOV_rm_r;
		compiler->mode32 = 0;
	}

	return SLJIT_SUCCESS;
}
