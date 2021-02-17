done = 0;

    /* see what "result" tokens we have. a "result" in ct-lib terms also  */
    /* includes row data. Some result types always get reported back  to  */
    /* the calling program, others are only reported back if the relevant */
    /* config flag is set.                                                */

while (!done) {

	tdsret = tds_process_tokens(tds, &res_type, NULL, TDS_TOKEN_RESULTS);

	switch (tdsret) {

	case TDS_SUCCESS:

		cmd->curr_result_type = res_type;

		switch (res_type) {
		case TDS_COMPUTEFMT_RESULT:
		case TDS_ROWFMT_RESULT:
			if (context->config.cs_expose_formats) {
				done = 1;
				retcode = CS_SUCCEED;
				*result_type = res_type;
			}
			break;

		case TDS_COMPUTE_RESULT:
			/* we've hit a compute data row. We have to get hold of this */
			/* data now, as it's necessary  to tie this data back to its */
			/* result format...the user may call ct_res_info() & friends */
			/* after getting back a compute "result".                    */

			tdsret = tds_process_row_tokens(tds, &rowtype);

			if (tdsret == TDS_SUCCESS) {
				if (rowtype == TDS_COMP_ROW) {
					cmd->row_prefetched = 1;
					retcode = CS_SUCCEED;
				} else {
					/* this couldn't really happen, but... */
					retcode = CS_FAIL;
				}
			} else
				retcode = CS_FAIL;
			done = 1;
			*result_type = res_type;
			break;

		case TDS_DONE_RESULT:
		case TDS_DONEPROC_RESULT:
		case TDS_DONEINPROC_RESULT:

			/* there's a distinction in ct-library     */
			/* depending on whether a command returned */
			/* results or not...                          */

			if (tds->res_info)
				*result_type = CS_CMD_DONE;
			else
				*result_type = CS_CMD_SUCCEED;

			retcode = CS_SUCCEED;
			done = 1;
			break;


		default:
			retcode = CS_SUCCEED;
			*result_type = res_type;
			done = 1;
			break;
		}
		break;

	case TDS_NO_MORE_RESULTS:
		done = 1;
		retcode = CS_END_RESULTS;
		break;
	case TDS_FAIL:
		done = 1;
		retcode = CS_FAIL;
		break;
	}
}
