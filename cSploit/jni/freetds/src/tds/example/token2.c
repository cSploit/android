if ((ret = tds_process_row_tokens(dbproc->tds_socket, &rowtype))
    == TDS_SUCCESS) {
	if (rowtype == TDS_REG_ROW) {
		/* Add the row to the row buffer */
		resinfo = tds->current_results;
		buffer_add_row(&(dbproc->row_buf), resinfo->current_row,
					resinfo->row_size);
		result = REG_ROW;
	} else if (rowtype == TDS_COMP_ROW) {
		/* Add the row to the row buffer */
		resinfo = tds->current_results;
		buffer_add_row(&(dbproc->row_buf), resinfo->current_row,
					resinfo->row_size);
		result = tds->current_results->computeid;
	} else
		result = FAIL;
} else if (ret == TDS_NO_MORE_ROWS) {
	result = NO_MORE_ROWS;
} else
	result = FAIL;
