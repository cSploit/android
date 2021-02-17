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

/* x86 32-bit arch dependent functions. */

static sljit_si emit_do_imm(struct sljit_compiler *compiler, sljit_ub opcode, sljit_sw imm)
{
	sljit_ub *inst;

	inst = (sljit_ub*)ensure_buf(compiler, 1 + 1 + sizeof(sljit_sw));
	FAIL_IF(!inst);
	INC_SIZE(1 + sizeof(sljit_sw));
	*inst++ = opcode;
	*(sljit_sw*)inst = imm;
	return SLJIT_SUCCESS;
}

static sljit_ub* generate_far_jump_code(struct sljit_jump *jump, sljit_ub *code_ptr, sljit_si type)
{
	if (type == SLJIT_JUMP) {
		*code_ptr++ = JMP_i32;
		jump->addr++;
	}
	else if (type >= SLJIT_FAST_CALL) {
		*code_ptr++ = CALL_i32;
		jump->addr++;
	}
	else {
		*code_ptr++ = GROUP_0F;
		*code_ptr++ = get_jump_code(type);
		jump->addr += 2;
	}

	if (jump->flags & JUMP_LABEL)
		jump->flags |= PATCH_MW;
	else
		*(sljit_sw*)code_ptr = jump->u.target - (jump->addr + 4);
	code_ptr += 4;

	return code_ptr;
}

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_emit_enter(struct sljit_compiler *compiler, sljit_si args, sljit_si scratches, sljit_si saveds, sljit_si local_size)
{
	sljit_si size;
	sljit_si locals_offset;
	sljit_ub *inst;

	CHECK_ERROR();
	check_sljit_emit_enter(compiler, args, scratches, saveds, local_size);

	compiler->scratches = scratches;
	compiler->saveds = saveds;
	compiler->args = args;
	compiler->flags_saved = 0;
#if (defined SLJIT_DEBUG && SLJIT_DEBUG)
	compiler->logical_local_size = local_size;
#endif

#if (defined SLJIT_X86_32_FASTCALL && SLJIT_X86_32_FASTCALL)
	size = 1 + (saveds <= 3 ? saveds : 3) + (args > 0 ? (args * 2) : 0) + (args > 2 ? 2 : 0);
#else
	size = 1 + (saveds <= 3 ? saveds : 3) + (args > 0 ? (2 + args * 3) : 0);
#endif
	inst = (sljit_ub*)ensure_buf(compiler, 1 + size);
	FAIL_IF(!inst);

	INC_SIZE(size);
	PUSH_REG(reg_map[TMP_REGISTER]);
#if !(defined SLJIT_X86_32_FASTCALL && SLJIT_X86_32_FASTCALL)
	if (args > 0) {
		*inst++ = MOV_r_rm;
		*inst++ = MOD_REG | (reg_map[TMP_REGISTER] << 3) | 0x4 /* esp */;
	}
#endif
	if (saveds > 2)
		PUSH_REG(reg_map[SLJIT_SAVED_REG3]);
	if (saveds > 1)
		PUSH_REG(reg_map[SLJIT_SAVED_REG2]);
	if (saveds > 0)
		PUSH_REG(reg_map[SLJIT_SAVED_REG1]);

#if (defined SLJIT_X86_32_FASTCALL && SLJIT_X86_32_FASTCALL)
	if (args > 0) {
		*inst++ = MOV_r_rm;
		*inst++ = MOD_REG | (reg_map[SLJIT_SAVED_REG1] << 3) | reg_map[SLJIT_SCRATCH_REG3];
	}
	if (args > 1) {
		*inst++ = MOV_r_rm;
		*inst++ = MOD_REG | (reg_map[SLJIT_SAVED_REG2] << 3) | reg_map[SLJIT_SCRATCH_REG2];
	}
	if (args > 2) {
		*inst++ = MOV_r_rm;
		*inst++ = MOD_DISP8 | (reg_map[SLJIT_SAVED_REG3] << 3) | 0x4 /* esp */;
		*inst++ = 0x24;
		*inst++ = sizeof(sljit_sw) * (3 + 2); /* saveds >= 3 as well. */
	}
#else
	if (args > 0) {
		*inst++ = MOV_r_rm;
		*inst++ = MOD_DISP8 | (reg_map[SLJIT_SAVED_REG1] << 3) | reg_map[TMP_REGISTER];
		*inst++ = sizeof(sljit_sw) * 2;
	}
	if (args > 1) {
		*inst++ = MOV_r_rm;
		*inst++ = MOD_DISP8 | (reg_map[SLJIT_SAVED_REG2] << 3) | reg_map[TMP_REGISTER];
		*inst++ = sizeof(sljit_sw) * 3;
	}
	if (args > 2) {
		*inst++ = MOV_r_rm;
		*inst++ = MOD_DISP8 | (reg_map[SLJIT_SAVED_REG3] << 3) | reg_map[TMP_REGISTER];
		*inst++ = sizeof(sljit_sw) * 4;
	}
#endif

#if (defined SLJIT_X86_32_FASTCALL && SLJIT_X86_32_FASTCALL)
	locals_offset = 2 * sizeof(sljit_uw);
#else
	SLJIT_COMPILE_ASSERT(FIXED_LOCALS_OFFSET >= 2 * sizeof(sljit_uw), require_at_least_two_words);
	locals_offset = FIXED_LOCALS_OFFSET;
#endif
	compiler->scratches_start = locals_offset;
	if (scratches > 3)
		locals_offset += (scratches - 3) * sizeof(sljit_uw);
	compiler->saveds_start = locals_offset;
	if (saveds > 3)
		locals_offset += (saveds - 3) * sizeof(sljit_uw);
	compiler->locals_offset = locals_offset;
#if defined(__APPLE__)
	saveds = (2 + (saveds <= 3 ? saveds : 3)) * sizeof(sljit_uw);
	local_size = ((locals_offset + saveds + local_size + 15) & ~15) - saveds;
#else
	local_size = locals_offset + ((local_size + sizeof(sljit_uw) - 1) & ~(sizeof(sljit_uw) - 1));
#endif

	compiler->local_size = local_size;
#ifdef _WIN32
	if (local_size > 1024) {
#if (defined SLJIT_X86_32_FASTCALL && SLJIT_X86_32_FASTCALL)
		FAIL_IF(emit_do_imm(compiler, MOV_r_i32 + reg_map[SLJIT_SCRATCH_REG1], local_size));
#else
		local_size -= FIXED_LOCALS_OFFSET;
		FAIL_IF(emit_do_imm(compiler, MOV_r_i32 + reg_map[SLJIT_SCRATCH_REG1], local_size));
		FAIL_IF(emit_non_cum_binary(compiler, SUB_r_rm, SUB_rm_r, SUB, SUB_EAX_i32,
			SLJIT_LOCALS_REG, 0, SLJIT_LOCALS_REG, 0, SLJIT_IMM, FIXED_LOCALS_OFFSET));
#endif
		FAIL_IF(sljit_emit_ijump(compiler, SLJIT_CALL1, SLJIT_IMM, SLJIT_FUNC_OFFSET(sljit_grow_stack)));
	}
#endif

	SLJIT_ASSERT(local_size > 0);
	return emit_non_cum_binary(compiler, SUB_r_rm, SUB_rm_r, SUB, SUB_EAX_i32,
		SLJIT_LOCALS_REG, 0, SLJIT_LOCALS_REG, 0, SLJIT_IMM, local_size);
}

SLJIT_API_FUNC_ATTRIBUTE void sljit_set_context(struct sljit_compiler *compiler, sljit_si args, sljit_si scratches, sljit_si saveds, sljit_si local_size)
{
	sljit_si locals_offset;

	CHECK_ERROR_VOID();
	check_sljit_set_context(compiler, args, scratches, saveds, local_size);

	compiler->scratches = scratches;
	compiler->saveds = saveds;
	compiler->args = args;
#if (defined SLJIT_DEBUG && SLJIT_DEBUG)
	compiler->logical_local_size = local_size;
#endif

#if (defined SLJIT_X86_32_FASTCALL && SLJIT_X86_32_FASTCALL)
	locals_offset = 2 * sizeof(sljit_uw);
#else
	locals_offset = FIXED_LOCALS_OFFSET;
#endif
	compiler->scratches_start = locals_offset;
	if (scratches > 3)
		locals_offset += (scratches - 3) * sizeof(sljit_uw);
	compiler->saveds_start = locals_offset;
	if (saveds > 3)
		locals_offset += (saveds - 3) * sizeof(sljit_uw);
	compiler->locals_offset = locals_offset;
#if defined(__APPLE__)
	saveds = (2 + (saveds <= 3 ? saveds : 3)) * sizeof(sljit_uw);
	compiler->local_size = ((locals_offset + saveds + local_size + 15) & ~15) - saveds;
#else
	compiler->local_size = locals_offset + ((local_size + sizeof(sljit_uw) - 1) & ~(sizeof(sljit_uw) - 1));
#endif
}

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_emit_return(struct sljit_compiler *compiler, sljit_si op, sljit_si src, sljit_sw srcw)
{
	sljit_si size;
	sljit_ub *inst;

	CHECK_ERROR();
	check_sljit_emit_return(compiler, op, src, srcw);
	SLJIT_ASSERT(compiler->args >= 0);

	compiler->flags_saved = 0;
	FAIL_IF(emit_mov_before_return(compiler, op, src, srcw));

	SLJIT_ASSERT(compiler->local_size > 0);
	FAIL_IF(emit_cum_binary(compiler, ADD_r_rm, ADD_rm_r, ADD, ADD_EAX_i32,
		SLJIT_LOCALS_REG, 0, SLJIT_LOCALS_REG, 0, SLJIT_IMM, compiler->local_size));

	size = 2 + (compiler->saveds <= 3 ? compiler->saveds : 3);
#if (defined SLJIT_X86_32_FASTCALL && SLJIT_X86_32_FASTCALL)
	if (compiler->args > 2)
		size += 2;
#else
	if (compiler->args > 0)
		size += 2;
#endif
	inst = (sljit_ub*)ensure_buf(compiler, 1 + size);
	FAIL_IF(!inst);

	INC_SIZE(size);

	if (compiler->saveds > 0)
		POP_REG(reg_map[SLJIT_SAVED_REG1]);
	if (compiler->saveds > 1)
		POP_REG(reg_map[SLJIT_SAVED_REG2]);
	if (compiler->saveds > 2)
		POP_REG(reg_map[SLJIT_SAVED_REG3]);
	POP_REG(reg_map[TMP_REGISTER]);
#if (defined SLJIT_X86_32_FASTCALL && SLJIT_X86_32_FASTCALL)
	if (compiler->args > 2)
		RET_I16(sizeof(sljit_sw));
	else
		RET();
#else
	RET();
#endif

	return SLJIT_SUCCESS;
}

/* --------------------------------------------------------------------- */
/*  Operators                                                            */
/* --------------------------------------------------------------------- */

/* Size contains the flags as well. */
static sljit_ub* emit_x86_instruction(struct sljit_compiler *compiler, sljit_si size,
	/* The register or immediate operand. */
	sljit_si a, sljit_sw imma,
	/* The general operand (not immediate). */
	sljit_si b, sljit_sw immb)
{
	sljit_ub *inst;
	sljit_ub *buf_ptr;
	sljit_si flags = size & ~0xf;
	sljit_si inst_size;

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
			inst_size += sizeof(sljit_sw);
		else if (immb != 0 && !(b & 0xf0)) {
			/* Immediate operand. */
			if (immb <= 127 && immb >= -128)
				inst_size += sizeof(sljit_sb);
			else
				inst_size += sizeof(sljit_sw);
		}

		if ((b & 0xf) == SLJIT_LOCALS_REG && !(b & 0xf0))
			b |= SLJIT_LOCALS_REG << 4;

		if ((b & 0xf0) != SLJIT_UNUSED)
			inst_size += 1; /* SIB byte. */
	}

	/* Calculate size of a. */
	if (a & SLJIT_IMM) {
		if (flags & EX86_BIN_INS) {
			if (imma <= 127 && imma >= -128) {
				inst_size += 1;
				flags |= EX86_BYTE_ARG;
			} else
				inst_size += 4;
		}
		else if (flags & EX86_SHIFT_INS) {
			imma &= 0x1f;
			if (imma != 1) {
				inst_size ++;
				flags |= EX86_BYTE_ARG;
			}
		} else if (flags & EX86_BYTE_ARG)
			inst_size++;
		else if (flags & EX86_HALF_ARG)
			inst_size += sizeof(short);
		else
			inst_size += sizeof(sljit_sw);
	}
	else
		SLJIT_ASSERT(!(flags & EX86_SHIFT_INS) || a == SLJIT_PREF_SHIFT_REG);

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

	buf_ptr = inst + size;

	/* Encode mod/rm byte. */
	if (!(flags & EX86_SHIFT_INS)) {
		if ((flags & EX86_BIN_INS) && (a & SLJIT_IMM))
			*inst = (flags & EX86_BYTE_ARG) ? GROUP_BINARY_83 : GROUP_BINARY_81;

		if ((a & SLJIT_IMM) || (a == 0))
			*buf_ptr = 0;
#if (defined SLJIT_SSE2 && SLJIT_SSE2)
		else if (!(flags & EX86_SSE2))
			*buf_ptr = reg_map[a] << 3;
		else
			*buf_ptr = a << 3;
#else
		else
			*buf_ptr = reg_map[a] << 3;
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
		*buf_ptr++ |= MOD_REG + ((!(flags & EX86_SSE2)) ? reg_map[b] : b);
#else
		*buf_ptr++ |= MOD_REG + reg_map[b];
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
				*buf_ptr++ |= reg_map[b & 0x0f];
			else {
				*buf_ptr++ |= 0x04;
				*buf_ptr++ = reg_map[b & 0x0f] | (reg_map[(b >> 4) & 0x0f] << 3);
			}

			if (immb != 0) {
				if (immb <= 127 && immb >= -128)
					*buf_ptr++ = immb; /* 8 bit displacement. */
				else {
					*(sljit_sw*)buf_ptr = immb; /* 32 bit displacement. */
					buf_ptr += sizeof(sljit_sw);
				}
			}
		}
		else {
			*buf_ptr++ |= 0x04;
			*buf_ptr++ = reg_map[b & 0x0f] | (reg_map[(b >> 4) & 0x0f] << 3) | (immb << 6);
		}
	}
	else {
		*buf_ptr++ |= 0x05;
		*(sljit_sw*)buf_ptr = immb; /* 32 bit displacement. */
		buf_ptr += sizeof(sljit_sw);
	}

	if (a & SLJIT_IMM) {
		if (flags & EX86_BYTE_ARG)
			*buf_ptr = imma;
		else if (flags & EX86_HALF_ARG)
			*(short*)buf_ptr = imma;
		else if (!(flags & EX86_SHIFT_INS))
			*(sljit_sw*)buf_ptr = imma;
	}

	return !(flags & EX86_SHIFT_INS) ? inst : (inst + 1);
}

/* --------------------------------------------------------------------- */
/*  Call / return instructions                                           */
/* --------------------------------------------------------------------- */

static SLJIT_INLINE sljit_si call_with_args(struct sljit_compiler *compiler, sljit_si type)
{
	sljit_ub *inst;

#if (defined SLJIT_X86_32_FASTCALL && SLJIT_X86_32_FASTCALL)
	inst = (sljit_ub*)ensure_buf(compiler, type >= SLJIT_CALL3 ? 1 + 2 + 1 : 1 + 2);
	FAIL_IF(!inst);
	INC_SIZE(type >= SLJIT_CALL3 ? 2 + 1 : 2);

	if (type >= SLJIT_CALL3)
		PUSH_REG(reg_map[SLJIT_SCRATCH_REG3]);
	*inst++ = MOV_r_rm;
	*inst++ = MOD_REG | (reg_map[SLJIT_SCRATCH_REG3] << 3) | reg_map[SLJIT_SCRATCH_REG1];
#else
	inst = (sljit_ub*)ensure_buf(compiler, 1 + 4 * (type - SLJIT_CALL0));
	FAIL_IF(!inst);
	INC_SIZE(4 * (type - SLJIT_CALL0));

	*inst++ = MOV_rm_r;
	*inst++ = MOD_DISP8 | (reg_map[SLJIT_SCRATCH_REG1] << 3) | 0x4 /* SIB */;
	*inst++ = (0x4 /* none*/ << 3) | reg_map[SLJIT_LOCALS_REG];
	*inst++ = 0;
	if (type >= SLJIT_CALL2) {
		*inst++ = MOV_rm_r;
		*inst++ = MOD_DISP8 | (reg_map[SLJIT_SCRATCH_REG2] << 3) | 0x4 /* SIB */;
		*inst++ = (0x4 /* none*/ << 3) | reg_map[SLJIT_LOCALS_REG];
		*inst++ = sizeof(sljit_sw);
	}
	if (type >= SLJIT_CALL3) {
		*inst++ = MOV_rm_r;
		*inst++ = MOD_DISP8 | (reg_map[SLJIT_SCRATCH_REG3] << 3) | 0x4 /* SIB */;
		*inst++ = (0x4 /* none*/ << 3) | reg_map[SLJIT_LOCALS_REG];
		*inst++ = 2 * sizeof(sljit_sw);
	}
#endif
	return SLJIT_SUCCESS;
}

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_emit_fast_enter(struct sljit_compiler *compiler, sljit_si dst, sljit_sw dstw)
{
	sljit_ub *inst;

	CHECK_ERROR();
	check_sljit_emit_fast_enter(compiler, dst, dstw);
	ADJUST_LOCAL_OFFSET(dst, dstw);

	CHECK_EXTRA_REGS(dst, dstw, (void)0);

	/* For UNUSED dst. Uncommon, but possible. */
	if (dst == SLJIT_UNUSED)
		dst = TMP_REGISTER;

	if (dst <= TMP_REGISTER) {
		/* Unused dest is possible here. */
		inst = (sljit_ub*)ensure_buf(compiler, 1 + 1);
		FAIL_IF(!inst);

		INC_SIZE(1);
		POP_REG(reg_map[dst]);
		return SLJIT_SUCCESS;
	}

	/* Memory. */
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

	CHECK_EXTRA_REGS(src, srcw, (void)0);

	if (src <= TMP_REGISTER) {
		inst = (sljit_ub*)ensure_buf(compiler, 1 + 1 + 1);
		FAIL_IF(!inst);

		INC_SIZE(1 + 1);
		PUSH_REG(reg_map[src]);
	}
	else if (src & SLJIT_MEM) {
		inst = emit_x86_instruction(compiler, 1, 0, 0, src, srcw);
		FAIL_IF(!inst);
		*inst++ = GROUP_FF;
		*inst |= PUSH_rm;

		inst = (sljit_ub*)ensure_buf(compiler, 1 + 1);
		FAIL_IF(!inst);
		INC_SIZE(1);
	}
	else {
		/* SLJIT_IMM. */
		inst = (sljit_ub*)ensure_buf(compiler, 1 + 5 + 1);
		FAIL_IF(!inst);

		INC_SIZE(5 + 1);
		*inst++ = PUSH_i32;
		*(sljit_sw*)inst = srcw;
		inst += sizeof(sljit_sw);
	}

	RET();
	return SLJIT_SUCCESS;
}
