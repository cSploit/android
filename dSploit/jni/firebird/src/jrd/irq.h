/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		irq.h
 *	DESCRIPTION:	Registry of persistent internal requests
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * 2001.6.25 Claudio Valderrama: add irq_r_gen_id_num to support
 *	new MET_lookup_generator_id().
 */

#ifndef JRD_IRQ_H
#define JRD_IRQ_H

#define REQUEST(irt) dbb->dbb_internal[irt]

enum irq_type_t
{
	irq_s_pages,			// store PAGES
	irq_r_pages,			// read PAGES
	irq_l_field,			// lookup field name
	irq_l_relation,			// lookup relation name
	irq_c_relation,			// create new relation
	irq_format1,			// make a new format for a record
	irq_format2,			// make a new format for a record
	irq_format3,			// make a new format for a record
	irq_r_format,			// read a format
	//irq_r_relation,		// pick up view definition
	irq_c_index,			// create index
	irq_c_index_m,			// create index - modify ID.NULL
	irq_m_fields,			// process a modification of RDB$FIELDS
	irq_m_index,			// modify index id
	irq_r_fields,			// pick up field expressions
	irq_l_security,			// lookup security_classes
	irq_v_security,			// verify security for relation
	irq_l_index,			// lookup index id
	irq_l_functions,		// lookup function
	irq_l_args,				// lookup function arguments
	irq_s_triggers,			// scan triggers
	irq_s_triggers2,		// scan triggers
	irq_s_msgs,				// search for message
	irq_grant1,				// process grant option
	irq_grant2,				// process grant option
	irq_grant3,				// process grant option
	irq_grant4,				// process grant option
	irq_format4,			// make a new format for a record
	irq_s_trans,			// store RDB$TRANSACTIONS
	irq_m_trans,			// erase RDB$TRANSACTIONS
	//irq_e_trans,			// store RDB$TRANSACTIONS
	irq_revoke1,			// check revokee for residual privileges
	irq_revoke2,			// recursive revoke
	irq_grant5,				// check for "PUBLIC" in processing grant options
	irq_r_filters,			// read RDB$BLOB_FILTERS
	irq_c_trigger,			// check for dependencies when creating a trigger
	irq_s_deps,				// store RDB$DEPENDENCIES
	irq_d_deps,				// delete RDB$DEPENDENCIES
	irq_r_fld_dim,			// read RDB$FIELD_DIMENSIONS
	irq_r_gen_id,			// read RDB$GENERATORS, lookup by name.
	//irq_s_gen_id,			// store RDB$GENERATORS
	irq_ch_f_dpd,			// check object field dependencies
	irq_ch_dpd,				// check object dependencies
	irq_ch_cmp_dpd,			// check computed field dependencies
	irq_grant6,				// get field-level security class
	irq_grant7,				// update field-level security classes
	irq_grant8,				// purge out field-level security classes
	irq_foreign1,			// purge out field-level security classes
	irq_foreign2,			// purge out field-level security classes
	irq_format5,			// make a new format for a record

	irq_c_exp_index,		// create expression index
	irq_l_exp_index,		// lookup expression index

	irq_l_rel_id,			// lookup relation id
	irq_l_procedure,		// lookup procedure name
	irq_l_proc_id,			// lookup procedure id
	irq_r_params,			// scan procedure parameters

	irq_r_procedure,		// scan procedure
	irq_p_security,			// verify security for procedure
	irq_c_prc_dpd,			// create procedure dependencies for delete

	irq_c_deps,				// check relation dependency
	irq_c_deps_f,			// check relation.field dependency
	irq_l_exception,		// lookup exception by number
	irq_l_except_no,		// lookup exception by name
	irq_grant9,				// process grant option
	irq_l_index_name,		// lookup index by name
	irq_view_context,		// lookup view context
	irq_l_cnstrt,			// lookup constraint for index
	irq_l_check,			// lookup check constraint for trigger
	irq_l_check2,			// lookup constraint for index
	irq_c_trg_perm,			// check if trig can ignore perm. checks
	irq_get_role_mem,		// get SQL role membership
	irq_get_role_name,		// get SQL role name
	irq_format6,			// make a new format for a record
	irq_r_gen_id_num,		// lookup generator by ID.
	irq_verify_role_name,	// ensure role exists in roles & user_privileges.
	irq_m_index_seg,		// modify per-segment index selectivity

	irq_l_subtype,			// lookup subtype (charset/collation)
	irq_c_relation2,		// create new relation
	irq_r_type,				// lookup relation type
	irq_p_type,				// lookup procedure type
	irq_prc_validate,		// procedure blr validate
	irq_trg_validate,		// trigger blr validate
	irq_l_domain,			// lookup a domain
	irq_m_fields2,			// process a modification of RDB$FIELDS for procedures (ODS 11.1)
	irq_m_fields3,			// process a modification of RDB$FIELDS for triggers (ODS 11.1)
	irq_m_fields4,			// process a modification of RDB$FIELDS for procedures (TYPE OF COLUMN)
	irq_m_fields5,			// process a modification of RDB$FIELDS for triggers (TYPE OF COLUMN)
	irq_r_params2,			// scan procedure parameters (ODS 11.1)
	irq_l_trg_dbg,			// lookup trigger debug_info (ODS 11.1)
	irq_l_colls,			// lookup collations
	irq_l_relfield,			// lookup a relation field
	irq_verify_trusted_role, // ensure trusted role exists

    irq_relation,			// DSQL/METD: lookup a relation
    irq_fields,				// DSQL/METD: lookup a relation's fields
    //irq_dimensions,		// lookup a field's dimensions
    irq_primary_key,		// DSQL/METD: lookup a primary key
    irq_view,				// DSQL/METD: lookup a view's base relations
    irq_view_base,			// DSQL/METD: lookup a view's base relations
    irq_view_base_flds,		// DSQL/METD: lookup a view's base fields
    irq_function,			// DSQL/METD: lookup a user defined function
    irq_func_return,		// DSQL/METD: lookup a function's return argument
    irq_procedure,			// DSQL/METD: lookup a stored procedure
    irq_parameters,			// DSQL/METD: lookup a procedure's parameters
    irq_parameters2,		// DSQL/METD: lookup a procedure's parameters (ODS 11.1)
    irq_parameters_description,	// DSQL/METD: lookup a procedure's parameters description
    irq_collation,			// DSQL/METD: lookup a collation name
    irq_charset,			// DSQL/METD: lookup a character set
    irq_trigger,			// DSQL/METD: lookup a trigger
    irq_domain,				// DSQL/METD: lookup a domain
    irq_type,				// DSQL/METD: lookup a symbolic name in RDB$TYPES
    irq_col_default,		// DSQL/METD: lookup default for a column
    irq_domain_2,			// DSQL/METD: lookup a domain
    irq_exception,			// DSQL/METD: lookup an exception
	irq_cs_name,			// DSQL/METD: lookup a charset name
	irq_default_cs,			// DSQL/METD: lookup the default charset
	irq_rel_ids,			// DSQL/METD: check relation/field ids
	irq_comp_circ_dpd,		// check computed circular dependencies

	irq_MAX
};

#endif // JRD_IRQ_H
