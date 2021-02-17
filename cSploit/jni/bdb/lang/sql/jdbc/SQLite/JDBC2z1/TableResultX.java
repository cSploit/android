package SQLite.JDBC2z1;

import java.sql.Types;

public class TableResultX extends SQLite.TableResult {
    public int sql_type[];

    public TableResultX() {
	super();
	sql_type = new int[this.ncolumns];
	for (int i = 0; i < this.ncolumns; i++) {
	    sql_type[i] = Types.VARCHAR;
	}
    }

    public TableResultX(int maxrows) {
	super(maxrows);
	sql_type = new int[this.ncolumns];
	for (int i = 0; i < this.ncolumns; i++) {
	    sql_type[i] = Types.VARCHAR;
	}
    }

    public TableResultX(SQLite.TableResult tr) {
	this.column = tr.column;
	this.rows = tr.rows;
	this.ncolumns = tr.ncolumns;
	this.nrows = tr.nrows;
	this.types = tr.types;
	this.maxrows = tr.maxrows;
	sql_type = new int[tr.ncolumns];
	for (int i = 0; i < this.ncolumns; i++) {
	    sql_type[i] = Types.VARCHAR;
	}
	if (tr.types != null) {
	    for (int i = 0; i < tr.types.length; i++) {
		sql_type[i] = JDBCDatabaseMetaData.mapSqlType(tr.types[i]);
	    }
	}	
    }

    void sql_types(int types[]) {
	sql_type = types;
    } 
}
